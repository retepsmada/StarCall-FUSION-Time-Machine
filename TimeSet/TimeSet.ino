#include <Ethernet.h> //Ethernet Client
#include <SPI.h> //For Interfacing With Ethernet Shield
#include <Wire.h>//For I2C
#include <Time.h>//Dependancy For DS1307
#include <TimeLib.h>//Dependancy For Time
#include <TimeAlarms.h>
#include <DS1307RTC.h>//For RTC

/* The sequence for dialing the clocks is this:
 *  Extension of system: 4000
 *  Select time set: 750
 *  Input time in military time: 7:00 would be 0700
 *  Set time: #
 *  Password: 1234
*/

//A4 and A5 are used for I2C

//Relay Pin
const byte phoneHook = A1;
//Relay Trigger Pin
const byte hookButton = 12;

//DTMF Variables
const byte tone1Pin=3; // pin for tone 1
const byte tone2Pin=4; // pin for tone 2
// for special characters: 10=*, 11=#, 12=1sec delay
//dialNumber(PhoneNumber,PhoneNumberLength);   Dial the number
const byte setTimePin=7; // for momentary switch
bool phoneHookOn = false; //For relay testing

//Ethernet Variables
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ip[] = {172, 19, 1, 521};
byte gateway[] = {172, 19, 1, 1};
byte subnet[] = {};

//Time Variables
tmElements_t tm; //DS1307 Get Time Object (tm.Second)


void setup()
{  
  //outputs
  pinMode(tone1Pin,OUTPUT); // Output for Tone 1
  pinMode(tone2Pin,OUTPUT); // Output for Tone 2
  pinMode(phoneHook,OUTPUT); // Output for picking/hanging up the phone
  digitalWrite(phoneHook, LOW);
  
  //inputs
  pinMode(setTimePin,INPUT_PULLUP); // Button for testing
  pinMode(hookButton,INPUT_PULLUP); // Button for taking phone off hook

  //ethernet and time
  Ethernet.begin(mac, ip, gateway, subnet); 
  EthernetClient client; //This is used to get the time over HTTP
  //Get the current time from the Internet:
  unsigned long unixTime = webUnixTime(client);
  Serial.begin(115200);

  setSyncProvider(RTC.get);
  
  //Set the time for the school clock at 7:04:00
  Alarm.alarmRepeat(7, 4, 0, setTimeAlarm);
  //Sync real-time clock with Internet at 6:30:00
  Alarm.alarmRepeat(6, 30, 0, getInternetTime);
}
 
void loop()
{
  //for testing
  Serial.print(hour());
  Serial.print(minute());
  Serial.println(second());
  
  //If hookButton is pressed, toggle whether the phone is picked up or not:
  if(digitalRead(hookButton) == LOW){
    phoneHookOn = !phoneHookOn;
    digitalWrite(phoneHook, phoneHookOn);
    delay(1000);
  }

  //If setTimePin is pressed, set the time for the school clock:
  if(digitalRead(setTimePin) == LOW){
    setTime();
  }
}

void setTimeAlarm(){
  /* Callback for TimeAlarms used to sync school clocks */
  Serial.println("ITS WORKING YEAH BOIIIIII");
  setTime();
}
