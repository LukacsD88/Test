#include <Arduino.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "secrets.h"


char auth[] = API_KEY;  //from secrets.h
char ssid[] = SSID;     //from secrets.h
char pass[] = PASSWORD; //from secrets.h

BlynkTimer timer;

WidgetRTC rtc;

//WidgetRTC rtc;


//LCD
LiquidCrystal_I2C lcd(0x3F, 20,4);



#define Relay 4  //D2
#define Reed_Relay_Upper  4 //D2  //15//D8  --- 4 //D2
#define Reed_Relay_Lower  5 //D1
#define Reed_Relay_LED_Out  14 //D5//2 //D4
#define LDR_Sensor_Pin A0


//State Variables
int Waterlevel_State = 0;
int Waterlevel_State_prev = 22;  //22 to differentiate between active states and init
int val_upper = 0; //ReedRelay upper value
int val_lower = 0; //ReedRelay lower value

int Errorcounter = 0;
int reed_status = 0;

int LDR_Sensor_value = 0;


const char* Waterlevel_State_names[] = {

"Init          ",
"Tank Full     ",
"Tank middle   ",
"Tank level low",
"Error!        "

};

// Digital clock display of the time
void clockDisplay()
{
  // You can call hour(), minute(), ... at any time
  // Please see Time library examples for details
String currentTime = String(hour()) + ":" + minute() + ":" + second();

char dateBuffer[12];

sprintf(dateBuffer, "%02u:%02u:%02u ", hour(),minute(),second());


 String currentDate = String(day()) + " " + month() + " " + year();

  Serial.print("Current time: ");
  Serial.print(dateBuffer);
  Serial.print(" ");
  Serial.print(currentDate);
  Serial.println();

sprintf(dateBuffer, "%02u:%02u", hour(),minute());
  String currentTime_LCD = String(hour()) + ":" + minute();
lcd.setCursor(14,0);
lcd.print(dateBuffer);

  // Send time to the App
  Blynk.virtualWrite(V1, currentTime);
  // Send date to the App
  Blynk.virtualWrite(V2, currentDate);
}

void ButtonHandling()
{

}


//Add relay relevant code
BLYNK_WRITE(V0)
{
int state = param.asInt();

Serial.println(state);

if (state == 1)
{
digitalWrite(Relay, LOW);
}
else
{
  digitalWrite(Relay, HIGH);
}


}



BLYNK_CONNECTED()
{

  rtc.begin();
}

void Waterlevel_FCN()
{
  val_upper =! digitalRead(Reed_Relay_Upper);
  val_lower =! digitalRead(Reed_Relay_Lower);
  Serial.print("Upper relay value = ");
  Serial.println(val_upper);
  Serial.print("Lower relay value = ");
  Serial.println(val_lower);
  Serial.print("State: ");
  Serial.println(Waterlevel_State);
 // lcd.clear();
  const char * val = Waterlevel_State_names[Waterlevel_State];
//lcd.setCursor(0,0);
 //lcd.print(val);
lcd.setCursor(0,0);
 if(Waterlevel_State - Waterlevel_State_prev != 0)
 {
//lcd.clear();
lcd.print(val);


 }


switch (Waterlevel_State)
{
  

  
  case 0:   //Init
  //lcd.setCursor(0,0);
  
  val_upper =! digitalRead(Reed_Relay_Upper);
  val_lower =! digitalRead(Reed_Relay_Lower);

  if      ((val_lower == 0) &&(val_upper == 0)) { Waterlevel_State = 3;} //Tank level low
  else if ((val_lower == 0) &&(val_upper == 1)) { Waterlevel_State = 4;} //Error
  else if ((val_lower == 1) &&(val_upper == 0)) { Waterlevel_State = 2;} //between min /max;
  else if ((val_lower == 1) &&(val_upper == 1)) { Waterlevel_State = 1;} // Tank level full
  else
  {
    Serial.println("Error!  Not recognized state!!!");
  }
  Waterlevel_State_prev = 0;
    break;


  case 1:  //Tank Full
      Serial.println("Tank is full");
         Waterlevel_State = 1;
      if((val_upper == 0) && (val_lower == 1)) {Waterlevel_State = 2;} //Tank level goes below full level
      else if ((val_upper == 0) && (val_lower == 0)) 
        {
          Waterlevel_State = 3;  //Tank level is below minimum (Leakage ???)
        }
        else if ((val_lower == 0) &&(val_upper == 1)) { Waterlevel_State = 4;} // Error

        Waterlevel_State_prev = 1;
  break;

  case 2:  //Tank between Min - Max
    Serial.println("Tank is between min and full");
    
    if(Waterlevel_State_prev == 3)
    {
      while(val_upper != 1)
      {
        Serial.println("Pump filling the tank");

      }
      if ((val_lower == 1) &&(val_upper == 1))
      {
        Waterlevel_State = 1;
        Serial.println("Tank full, go to State 1!");
        
              }
    }
    else
    {

      if ((val_lower == 1) &&(val_upper == 0)) 
      { 
      Serial.println ("Water level decreasing!");
      Waterlevel_State = 2;
      }
      else if ((val_lower == 1) &&(val_upper == 1)) 
      {
        Serial.println ("Water filled up to max!");
        Waterlevel_State = 1; //Check changes

      }
      else if ((val_lower == 0) &&(val_upper == 0))
      {
        Serial.println("Water level too low , go to fill up state!");
        Waterlevel_State = 3;  //Water level low level

      }
      else if ((val_lower == 0) &&(val_upper == 1)) 
      {
        Serial.println ("Go to error state!");
        Waterlevel_State = 4;
      }

    }


    


    Waterlevel_State_prev = 2;

  break;

  case 3:  //Tank level low
  Serial.println("Tank level is low, PUMP start!");
  Waterlevel_State_prev = 3;
  if ((val_lower == 1) &&(val_upper == 1)) { Waterlevel_State = 1;}
  break;

  case 4:  //Error
  Serial.println (" Error!!!");
  Errorcounter++; //Errorcounter++;
  Serial.print ("Error counter: ");
  Serial.println(Errorcounter);
  Waterlevel_State_prev = 4;
  //Waterlevel_State = 0;
  //Healiing from failure:
   if ((val_lower == 1) &&(val_upper == 1)) { Waterlevel_State = 0;}
   else if ((val_lower == 0) &&(val_upper == 0)) { Waterlevel_State = 0;}
   else  if ((val_lower == 1) &&(val_upper == 0)) { Waterlevel_State = 0;}

  break;
  
}

}


void LDR_Light_Measure()
{

  LDR_Sensor_value = 1024 - analogRead(LDR_Sensor_Pin);
  Serial.print("Light: ");
  Serial.println(LDR_Sensor_value);
  lcd.setCursor(0,3);
  lcd.print("Light: ");
  lcd.print(LDR_Sensor_value);


}



void setup() {


  // put your setup code here, to run once:
Serial.begin(9600);

pinMode(Relay, OUTPUT);
pinMode(Reed_Relay_Lower, INPUT);
pinMode(Reed_Relay_Upper, INPUT);
pinMode(Reed_Relay_LED_Out, OUTPUT);

Wire.begin(D3, D4);

//LCD setup
lcd.init();
lcd.backlight();

 /* lcd.setCursor ( 0, 0 );            // go to the top left corner
  lcd.print("    Hello,world!    "); // write this string on the top row
  lcd.setCursor ( 0, 1 );            // go to the 2nd row
  lcd.print("   IIC/I2C LCD2004  "); // pad string with spaces for centering
  lcd.setCursor ( 0, 2 );            // go to the third row
  lcd.print("  20 cols, 4 rows   "); // pad with spaces for centering
  lcd.setCursor ( 0, 3 );            // go to the fourth row
  lcd.print(" www.sunfounder.com ");*/

  lcd.setCursor(0,0);
  lcd.print ("Welcome!");


/*
  //Nextion

  //nexInit();
 // pinMode(ledpin, OUTPUT);
  //digitalWrite(ledpin, LOW);

  p0_b0.attachPop(p0_b0_Release, &p0_b0);
  p0_b1.attachPush(p0_b1_Press, &p0_b1);
  p0_b1.attachPop(p0_b1_Release, &p0_b1);
  p1_b0.attachPop(p1_b0_Release, &p1_b0);
p0_t1.setText("ESPPP");
next = millis();
*/


Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 8080);

setSyncInterval(10*60); // Sync interval in seconds (10 minutes)

  // Display digital clock every 10 seconds
  
clockDisplay();


timer.setInterval(3000L, clockDisplay);
//timer.setInterval(1000L, ButtonHandling);
timer.setInterval(1000L, Waterlevel_FCN);
timer.setInterval(30000L, LDR_Light_Measure);
//timer.setInterval(500L, do_every_so_often);



Serial.print("IP cim: ");
Serial.println(WiFi.localIP());
//Serial.println(WiFi.);

}

void loop() {
  
 // nexLoop(nex_listen_list);
  /*if(millis() >= next) {
    next = millis()+500;
    do_every_so_often();
  }*/
  Blynk.run();
  timer.run();

//reed_status = digitalRead(Reed_Relay_Lower);


}