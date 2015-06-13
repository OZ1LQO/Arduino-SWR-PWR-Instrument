/*
  OZ1LQO QRP/SWR Powermeter with AD8307 Log Amplifier / RSSI unit
  Project start July 30th, 2013.
  
  This is SW revision 0.4.
  
  Revision History:
  0.1 - 2014.07.30 First Startup: Use of 20x2 display and simple calculation of PWR, SWR and Return Loss
        based on fixed values. Also, some variables are defined, to be used in the future
        when the meter is calibrated
  0.2 - 2014.09.03 Moved things neatly into functions, added button read
  0.3 - 2014.09.04 Added raw mode to be selected during startup and averaging of input readings
  0.4 - 2014.09.05 Proof of concept! Added reading the real ports and added real (to be more accurate) calibration values
  1.0 - 2014.09.14 First fully funtional edition! Calibrated precisely, Buzzer, High Power Alarm, dBm/Watts switch
  1.1 - 2014.09.18 Added high SWR alarm, fixed set to 10:1.
  2.0 - 2014.09.26 Added menu to adjust SWR alarm level from 2:1 to 10:1. Stores the Value in the EEPROM
  2.1 - 2014.09.27 Added Max hold functionality
  2.2 - 2014.10.10 Added SWR Alarm OFF setting
*/

// include LCD Library and math
#include <LiquidCrystal.h>
#include <math.h>
#include <EEPROM.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

//Initialize variables

//Calibration values. Later a calibration routine should be added.
// Note that there are calibration values for forward AND reverse as the 
// SWR bridge may not be entirely symmetrical
//These calibration values are from using the FT817 at lowest power level and a HP8482 power sensor

//FWD
float Vcalfwd = 1.7535; //Output voltage from AD8307, at 27.6dBm
float Pcalfwd = 27.6; //Corresponding power at the input terminal
//REV
float Vcalrev = 1.8848; //Output voltage from AD8307, at 27.6dBm
float Pcalrev = 27.6; //Corresponding power at the input terminal

// Port pin definitions
int buttonPin = 2; // AD port to read buttons
int RevPort = 0; // AD port for reflected power
int FwdPort = 1; // AD port for forward power
int BackLight = 8;//Backlight Port
int Buzzer = 10;//Buzzer Port. Use 'tone(Buzzer,1000) to turn it on, noTone(Buzzer) to turn it off
int Mode = 7;//Port to read mode switch
int max_swr; //max swr alarm level variable declaration
int maxhold = 0; //Initialise max hold variable, defaults at '0', ie. OFF
float fwd;// Initialise fwd and rev variables
float rev;


//Calculates the measured power in dBm from the measured input values and the calibration
float dbmpwr(float Vcal, float Pcal, float Vpwr){
  float diff=Vpwr-Vcal;
  diff=diff/0.025;
  float pwr = Pcal+diff;
  return pwr;
}

//Calculates power in watts from dBm
double watts(float dbm){
  return pow(10.0,(dbm/10.0))*0.001;
}

//Calculates the SWR from forward and reverse power
double swr(double Pf, double Pr){
  return ((1+sqrt(Pr/Pf))/(1-sqrt(Pr/Pf)));
}
  
  
  //Function to calculate and print dBm and RL
  //print forward/reverse power in dBm and calculate return loss
void dBpower(float fwddbm, float revdbm, double wattfwd, double wattrev){
  lcd.clear();
  lcd.print("F:");
  lcd.print(fwddbm,1);
  lcd.print("dBm");
  lcd.setCursor(0, 1);
  lcd.print("R:");
  lcd.print(revdbm,1);
  lcd.print("dBm RL:");
  lcd.print(10.0*log10(wattfwd/wattrev),1);
  lcd.print("dB");
}
  
  
  //Function to print WATTS and SWR
  //Print forward power in watts
void wattswr(double wattfwd, double wattrev, int max_swr){
  lcd.clear();
  lcd.print("F:");
  // Check if it is sub 1 watt, if so print in milliwatts
  if (wattfwd<1){
    lcd.print(wattfwd*1000.0,1);
    lcd.print("mW");
  }
  else {
    lcd.print(wattfwd,1);
    lcd.print("W");
  }
  
  //Print reverse power in watts
  lcd.setCursor(0, 1);
  lcd.print("R:");
    // Check if it is sub 1 watt, if so print in milliwatts
  if (wattrev<1){
    lcd.print(wattrev*1000.0,1);
    lcd.print("mW");
  }
  else {
    lcd.print(wattrev,1);
    lcd.print("W");
  }
  
  //check for swr > max swr setting and sound alarm if so.
  
  float m_swr=swr(wattfwd,wattrev);
  if(m_swr>max_swr && max_swr!=11){ //checf if over the limit or if OFF
    lcd.clear();
    lcd.print("SWR ALARM");
    lcd.setCursor(0, 1);
    lcd.print("SWR>");
    lcd.print(max_swr);
    lcd.print(":1");
    Alarm(Buzzer);
    delay(1000);
  }
  else{
    lcd.print("  SWR:");
    lcd.print(m_swr,1);
  }
  
}
  
//Function to read pushbuttons
//Returns 0 if none are pressed, else 1,2,3,4 for the corresponding buttons, left to right
int buttons(int pin){
  int button;
  int keyVal = analogRead(pin);
  delay(40);//delay to compensate for bouncing
  
  if(keyVal==1023){
    button=1;
  }
  else if(keyVal >= 990 && keyVal <= 1010){
    button=2;
  }
  else if(keyVal >= 505 && keyVal <= 515){
   button=3;
  }
  else if(keyVal >= 5 && keyVal <= 10){
   button=4;
  }
  else{
    button=0;
  }
  return button;
}
   
  //Function to read an A/D port, averaging 100 samples
  float portRead(int Port){
    float result = 0.0;
    for(int i=0; i<100;i++){
      result+=(5.0/1024.0)*analogRead(Port);
    }
    result/= 100.0;
    return result;
  }

//Function to sound Alarm
void Alarm(int Buzzer){
  for(int i=0;i<10;i++){
    tone(Buzzer,4000);
    delay(100);
    noTone(Buzzer);
    delay(100);
  }
}
  
  
  
//SWR Alarm level Menu Function
int menu(int max_swr){
  
  int value = max_swr;  //store current max swr level
  lcd.clear();
  lcd.print("Set SWR Alarm level"); //intro
  lcd.setCursor(0,1);
  lcd.print("Use UP/DOWN/SEL");
  delay(3000);
    
  while(buttons(buttonPin)!=1){ //do this as long as select has not been pushed
    lcd.clear();
    lcd.print("SWR Alarm level:");
    lcd.setCursor(0,1);
    if(value==11){
    lcd.print("OFF");
  }
  else{
    lcd.print(value);
  }
    if(buttons(buttonPin)==2){ //if 'up', then add one, check for max value of 10
      value+=1;
      if(value>11){
        value=11; //11 means OFF
      }
    }
    if(buttons(buttonPin)==3){ //if 'down', then subtract one, check for min value of 2
      value-=1;
      if(value<2){
        value=2;
      }
    }
     delay(100);
  }
  lcd.clear();
  lcd.print("SWR Alarm level: "); //confirm the value or OFF setting
  if(value==11){
    lcd.print("OFF");
  }
  else{
    lcd.print(value);
  }
  EEPROM.write(0,value); //Store the value in the eeprom to save it for next power up
  delay(2000);
  return value; //return the new value
}  
  
  
  
void setup() {
  
  //setup Mode switch input
  pinMode(Mode, INPUT);
  
  //turn on backlight
  pinMode(BackLight, OUTPUT);
  digitalWrite(BackLight, HIGH);
  
  //Initialize Serial Port
  Serial.begin(9600);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(20, 2);
  // Greet the user.
  lcd.print("OZ1LQO SWR/PWR Meter");
  lcd.setCursor(0,1);
  lcd.print("Rev 2.2 - 2014.10.10");
  
  //Test Alarm Buzzer
  Alarm(Buzzer);
  
  //read stored maximum swr from eeprom adresse 0.
  //Make it 11, if it's the first time reading from the eeprom, as the values defaults to 255
  // if never used
  max_swr=EEPROM.read(0);
  if(max_swr>11){
    max_swr=11;
  }
  
  
  //Enable raw mode by holding key 'Select'
  lcd.clear();
  lcd.print("Press Select");
  lcd.setCursor(0,1);
  lcd.print("for raw mode..");
  delay(2000);
  if(buttons(buttonPin)==1){
    while(true){
      float f=portRead(FwdPort);//read the ports
      float r=portRead(RevPort);
      lcd.clear();
      lcd.print("F:");//print the voltages with 4 decimals
      lcd.print(f,4);
      lcd.print("V");
      lcd.setCursor(0, 1);
      lcd.print("R:");
      lcd.print(r,4);
      lcd.print("V");
      delay(500);
    }
  }
      
  //read FWD and REV ports first time
  fwd=portRead(FwdPort);
  rev=portRead(RevPort);
  lcd.clear();
  lcd.print("Max Hold Disabled");//inform max hold disabled
  lcd.setCursor(0,1);
  lcd.print("SWR Alarm level: ");//inform SWR Alarm Level
  if(max_swr==11){
    lcd.print("OFF");
  }
  else{
    lcd.print(max_swr);
  }
  delay(3000);
  
  
}


    

//Main Loop
void loop() {
  
  //check for menu selection
  if(buttons(buttonPin)==1){
    max_swr=menu(max_swr);
  }
  
  
 //check for max hold enabling
 if(buttons(buttonPin)==4){
    if(maxhold==0){
    lcd.clear();
    lcd.print("Max Hold Enabled");
    maxhold=1;
    delay(2000);
    }
    else{
      lcd.clear();
    lcd.print("Max Hold Disabled");
    maxhold=0;
    delay(2000);
    }
 }
      
  
  // Max Hold functionality  
  if(maxhold==1){
    float fwd_value=portRead(FwdPort);
    float rev_value=portRead(RevPort);
    if(fwd_value>fwd){ //compare with initial measurement and store new value if it's larger
      fwd=fwd_value;
      rev=rev_value;
    }
  }
  else{
    fwd=portRead(FwdPort);
    rev=portRead(RevPort);
    
  }

  
  //Send values to the serial port
  Serial.print("FWD: ");
  Serial.print(fwd,4);
  Serial.print("V   ");
  Serial.print("REF: ");
  Serial.print(rev,4);
  Serial.println("V   ");
  
  
  //Calculate forward and reverse power in dBm
  float fwddbm=dbmpwr(Vcalfwd,Pcalfwd,fwd);
  float revdbm=dbmpwr(Vcalrev,Pcalrev,rev);
  
  //Calculate forward and reverse power in watts
  double wattfwd = watts(fwddbm);
  double wattrev = watts(revdbm);
  
  //Sound High Power alarm if input power exceeds 50W
  if(wattfwd>50){
    lcd.clear();
    lcd.print("HIGH POWER!!");
    lcd.setCursor(0, 1);
    lcd.print("50W MAXIMUM");
    Alarm(Buzzer);
    delay(3000);
  }
  
  //Check mode selector switch
  if(digitalRead(Mode)==LOW){
  
  //Call the dBm power function
  dBpower(fwddbm,revdbm,wattfwd,wattrev);
  delay(100);
  }
  else{
  
  //Call the watts and swr function
  wattswr(wattfwd,wattrev,max_swr);
  delay(100);
  }
  
  
}

