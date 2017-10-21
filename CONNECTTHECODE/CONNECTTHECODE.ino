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

//Ethernet Variables
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ip[] = {172, 19, 1, 521};
byte gateway[] = {172, 19, 1, 1};
byte subnet[] = {};

//Time Variables
tmElements_t tm; //DS1307 Get Time Object (tm.Second)

//DTMF Variables
const byte tone1Pin=3; // pin for tone 1
const byte tone2Pin=4; // pin for tone 2
// for special characters: 10=*, 11=#, 12=1sec delay
//dialNumber(PhoneNumber,PhoneNumberLength);   Dial the number
const byte setTimePin=7; // for momentary switch
bool phoneHookOn = false; //For relay testing

// frequencies adopted from: https://en.wikipedia.org/wiki/Dual-tone_multi-frequency_signaling
int DTMF[13][2]={
  {941,1336}, // frequencies for touch tone 0
  {697,1209}, // frequencies for touch tone 1
  {697,1336}, // frequencies for touch tone 2
  {697,1477}, // frequencies for touch tone 3
  {770,1209}, // frequencies for touch tone 4
  {770,1336}, // frequencies for touch tone 5
  {770,1477}, // frequencies for touch tone 6
  {852,1209}, // frequencies for touch tone 7
  {852,1336}, // frequencies for touch tone 8
  {852,1477}, // frequencies for touch tone 9
  {941,1209}, // frequencies for touch tone *
  {941,1477}, // frequencies for touch tone #
  {0,0} // pause
};

void setup()
{  
  pinMode(tone1Pin,OUTPUT); // Output for Tone 1
  pinMode(tone2Pin,OUTPUT); // Output for Tone 2
  pinMode(phoneHook,OUTPUT); // Output for picking/hanging up the phone
  digitalWrite(phoneHook, LOW);
  pinMode(setTimePin,INPUT_PULLUP); // Button for testing
  pinMode(hookButton,INPUT_PULLUP); // Button for taking phone off hook
  
  Ethernet.begin(mac, ip, gateway, subnet); 
  EthernetClient client; //This is used to get the time over HTTP
  //Get the current time from the Internet:
  unsigned long unixTime = webUnixTime(client);
  Serial.begin(115200);

  //Set the time for the school clock at 7:04:00
  Alarm.alarmRepeat(7, 4, 0, setTimeAlarm);
  //Sync real-time clock with Internet at 6:30:00
  Alarm.alarmRepeat(6, 30, 0, getInternetTime);
}
 
void loop()
{
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
  
  setTime();
}

void getInternetTime(){
  /* Callback for TimeAlarms used to sync real-time clock with Internet */
  
}

void setTime(){
  /* Sets the school clocks by the minute according to the real-time clock */
  
  Serial.println("setTime() started");
  //Number of seconds before the next minute that we start the dialing
  const int delay_ = 15;
  RTC.read(tm); //This is the current time
  Serial.print("Current time is ");
  Serial.print(tm.Hour);
  Serial.print(":");
  Serial.println(tm.Minute);
  
  //This represents the minute at which we will start the sequence
  int startMinute = tm.Minute;
  //If we don't have enough time do the sequence before the next minute, do it at the minute after:
  if (tm.Second+delay_ > 60) startMinute++;
  //If the start minute is 60, subtract minute by 60:
  startMinute %= 60;
  //Keep updating the time until we get to delay seconds before the next minute:
  while ((tm.Minute != startMinute) || (tm.Second+delay_ != 60)) RTC.read(tm);
  
  digitalWrite(phoneHook, HIGH); //Phone off hook
  delay(500);
  byte sequence[]={4,0,0,0,0}; //Dial 4000
  dialNumber(sequence,4); 
  delay(3000);
  
  sequence[0] = 7; //Then 750
  sequence[1] = 5;
  sequence[2] = 0;
  dialNumber(sequence,3);
  delay(1000);

  //These two digits represent the hour:
  sequence[0] = tm.Hour/10;
  sequence[1] = tm.Hour % 10;
  //These two digits represent the minute:
  sequence[2] = tm.Minute/10;
  sequence[3] = tm.Minute % 10;
  sequence[4] = 11; //This is the #
  dialNumber(sequence,5); //Dial the hour, minute, followed by #
  delay(500);
  
  sequence[0] = 1; //Start the password
  sequence[1] = 2;
  sequence[2] = 3;
  dialNumber(sequence,3);
  
  sequence[0] = 4; //Last digit of password:
  Serial.println("Wait for it...");
  while(tm.Second > 0) RTC.read(tm); //Wait for the minute to tick over
  dialNumber(sequence,1); //Finish the password
  delay(500);
  
  digitalWrite(phoneHook, LOW); //Hang up
  delay(500);
}
 
void playDTMF(byte digit, byte duration){
  /* Plays two DTMF tones to mimic pressing a button on the phone keypad
  digit -> Represents button on keypad, according to indexes in DTMF array above
  duration -> Number of milliseconds the tone should play for
  */
  
  boolean tone1state=false;
  boolean tone2state=false;
  int tone1delay=(500000/DTMF[digit][0])-10; // calculate delay (in microseconds) for tone 1 (half of the period of one cycle). 10 is a fudge factor to raise the frequency due to sluggish timing.
  int tone2delay=(500000/DTMF[digit][1])-10; // calculate delay (in microseconds) for tone 2 (half of the period of one cycle). 10 is a fudge factor to raise the frequency due to sluggish timing.
  unsigned long tone1timer=micros();
  unsigned long tone2timer=micros();
  unsigned long timer=millis(); // for timing duration of a single tone
  if(digit==12){
    delay(1000); // one second delay if digit is 12
  } else {
    while(millis()-timer<duration){
      if(micros()-tone1timer>tone1delay){
        tone1timer=micros(); // reset the timer
        tone1state=!tone1state; // toggle tone1state
        digitalWrite(tone1Pin, tone1state);
      }
      if(micros()-tone2timer>tone2delay){
        tone2timer=micros(); // reset the timer
        tone2state=!tone2state; // toggle tone2state
        digitalWrite(tone2Pin, tone2state);
      }
    }
    digitalWrite(tone1Pin,LOW);
    digitalWrite(tone2Pin,LOW);
  }
}
 
void dialNumber(byte numbers[],byte len){
  /* Mimics pressing a sequence of buttons on the phone keypad
  numbers -> Sequence of numbers representing phone buttons, as according to indexes in DTMF array
  len -> Length of numbers array
  */
  
  for(int i=0;i<len;i++){ //Loop through numbers
    playDTMF(numbers[i], 100);  // 100 msec duration of tone
    delay(300); // 300 msec pause between tones
  }
}

unsigned long webUnixTime (Client &client)
{
  /*
   * © Francesco Potortì 2013 - GPLv3
   *
   * Send an HTTP packet and wait for the response, return the Unix time
   */
  unsigned long time = 0;

  // Just choose any reasonably busy web server, the load is really low
  if (client.connect("google.com", 80))
    {
      // Make an HTTP 1.1 request which is missing a Host: header
      // compliant servers are required to answer with an error that includes
      // a Date: header.
      client.print(F("GET / HTTP/1.1 \r\n\r\n"));

      char buf[5];      // temporary buffer for characters
      client.setTimeout(5000);
      if (client.find((char *)"\r\nDate: ") // look for Date: header
    && client.readBytes(buf, 5) == 5) // discard
  {
    unsigned day = client.parseInt();    // day
    client.readBytes(buf, 1);    // discard
    client.readBytes(buf, 3);    // month
    int year = client.parseInt();    // year
    byte hour = client.parseInt();   // hour
    byte minute = client.parseInt(); // minute
    byte second = client.parseInt(); // second

    int daysInPrevMonths;
    switch (buf[0])
      {
      case 'F': daysInPrevMonths =  31; break; // Feb
      case 'S': daysInPrevMonths = 243; break; // Sep
      case 'O': daysInPrevMonths = 273; break; // Oct
      case 'N': daysInPrevMonths = 304; break; // Nov
      case 'D': daysInPrevMonths = 334; break; // Dec
      default:
        if (buf[0] == 'J' && buf[1] == 'a')
    daysInPrevMonths = 0;   // Jan
        else if (buf[0] == 'A' && buf[1] == 'p')
    daysInPrevMonths = 90;    // Apr
        else switch (buf[2])
         {
         case 'r': daysInPrevMonths =  59; break; // Mar
         case 'y': daysInPrevMonths = 120; break; // May
         case 'n': daysInPrevMonths = 151; break; // Jun
         case 'l': daysInPrevMonths = 181; break; // Jul
         default: // add a default label here to avoid compiler warning
         case 'g': daysInPrevMonths = 212; break; // Aug
         }
      }

    // This code will not work after February 2100
    // because it does not account for 2100 not being a leap year and because
    // we use the day variable as accumulator, which would overflow in 2149
    day += (year - 1970) * 365; // days from 1970 to the whole past year
    day += (year - 1969) >> 2;  // plus one day per leap year 
    day += daysInPrevMonths;  // plus days for previous months this year
    if (daysInPrevMonths >= 59  // if we are past February
        && ((year & 3) == 0)) // and this is a leap year
      day += 1;     // add one day
    // Remove today, add hours, minutes and seconds this month
    time = (((day-1ul) * 24 + hour) * 60 + minute) * 60 + second;
  }
    }
  delay(10);
  client.flush();
  client.stop();

  return time;
}
