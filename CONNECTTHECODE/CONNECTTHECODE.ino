#include <Ethernet.h>
#include <SPI.h>

/* The sequence for dialing the clocks is this:
 *  Extension of system: 4000
 *  Select time set: 750
 *  Input time in military time: 7:00 would be 0700
 *  Set time: #
 *  Password: 1234
*/

const byte tone1Pin=3; // pin for tone 1
const byte tone2Pin=4; // pin for tone 2
byte PhoneNumber[]={8,6,7,5,3,0,9}; // for special characters: 10=*, 11=#, 12=1sec delay
byte PhoneNumberLength = 7;  // adjust to length of phone number
const byte buttonPin=7; // for momentary switch

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ip[] = {172, 19, 1, 521};
byte gateway[] = {172, 19, 1, 1};
byte subnet[] = {};

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
  pinMode(buttonPin,INPUT_PULLUP); // Button
  Ethernet.begin(mac, ip, gateway, subnet); 
  EthernetClient client; //For Time HTTP
  unsigned long unixTime = webUnixTime(client); //For Time HTTP
}
 
void loop()
{
  if(digitalRead(buttonPin)==LOW){  // If the button is pushed
    dialNumber(PhoneNumber,PhoneNumberLength);  // Dial the number
  }
}
 
void playDTMF(byte digit, byte duration){
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
 
void dialNumber(byte number[],byte len){ //Main function for actualy inputing numbers
  for(int i=0;i<len;i++){
    playDTMF(number[i], 100);  // 100 msec duration of tone
    delay(100); // 100 msec pause between tones
  }
}
//END OF FILE

/*
 * © Francesco Potortì 2013 - GPLv3
 *
 * Send an HTTP packet and wait for the response, return the Unix time
 */

unsigned long webUnixTime (Client &client)
{
  unsigned long time = 0;

  // Just choose any reasonably busy web server, the load is really low
  if (client.connect("g.cn", 80))
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