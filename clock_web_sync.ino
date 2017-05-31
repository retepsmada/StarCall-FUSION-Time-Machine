/**
 * The following is a code template that will reset the clocks
 * by getting the time from a Web server.
 * This will allow us to set the clock to a second's accuracy,
 * but without the code complexity and memory required for NTP.
 *
 * It is recommended that a Web server be used instead of NTP
 * when sub-second accuracy is not required,
 * according to the Arduino documentation.
 * https://playground.arduino.cc/Code/NTPclient
*/

enum month { JAN, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC };

typedef struct {
    unsigned day;
    month month;
    int year;
    byte hour;
    byte minute;
    byte second;
} time_obj;

time_obj webUnixTime (Client &client)
{
    /**
     * This function sends an request to g.cn (i.e. Google)
     * and gets the datetime from the Date HTTP header in the response.
     * This is an easy way to get the time without NTP.
     *  
     * Source: http://playground.arduino.cc//Code/Webclient
    */
    time_obj timeInfo;

    // Just choose any reasonably busy web server, the load is really low
    if (client.connect("g.cn", 80)) {
        // Make an HTTP 1.1 request which is missing a Host: header
        // compliant servers are required to answer with an error that includes
        // a Date: header.
        client.print(F("GET / HTTP/1.1 \r\n\r\n"));

        char buf[5];            // temporary buffer for characters
        client.setTimeout(5000);
        if (client.find((char *)"\r\nDate: ") // look for Date: header
            && client.readBytes(buf, 5) == 5) /* discard */ {
            timeInfo.day = client.parseInt();       // day
            client.readBytes(buf, 1);       // discard
            client.readBytes(buf, 3);       // month
            timeInfo.year = client.parseInt();       // year
            timeInfo.hour = client.parseInt();   // hour
            timeInfo.minute = client.parseInt(); // minute
            timeInfo.second = client.parseInt(); // second

            switch (buf[0]) {
                case 'F': timeInfo.month = FEB; break; // Feb
                case 'S': timeInfo.month = SEP; break; // Sep
                case 'O': timeInfo.month = OCT; break; // Oct
                case 'N': timeInfo.month = NOV; break; // Nov
                case 'D': timeInfo.month = DEC; break; // Dec
                default:
                  if (buf[0] == 'J' && buf[1] == 'a')
                      timeInfo.month = JAN;        // Jan
                  else if (buf[0] == 'A' && buf[1] == 'p')
                      timeInfo.month = APR;        // Apr
                  else switch (buf[2]) {
                      case 'r': timeInfo.month = MAR; break; // Mar
                      case 'y': timeInfo.month = MAY; break; // May
                      case 'n': timeInfo.month = JUN; break; // Jun
                      case 'l': timeInfo.month = JUL; break; // Jul
                      default: // add a default label here to avoid compiler warning
                      case 'g': timeInfo.month = AUG; break; // Aug
                  }
            }
        }
    }
    delay(10);
    client.flush();
    client.stop();

    return timeInfo;
}

//This keeps track of if we have started to set the clocks with DTMF yet:
boolean notStartedDTMF = true;

void loop() {
    //Get the time from g.cn:
    EthernetClient client;
    time_obj now = webUnixTime(client);
    //At 6:59:00, prepare to reset the time so it can be done swiftly at 7:00
    //but don't actually reset it yet:
    if ((now.hour == 6) && (now.minute == 59) && (now.second == 0) && notStartedDTMF) {
        DO THE DTMF THING
        ENTER ALL DIGITS OF PASSWORD EXCEPT LAST ONE
        //We have started DTMF:
        notStartedDTMF = false;
    }
    //At 7:00:00, immediately reset the time:
    if ((now.hour == 7) && (now.minute == 0) && (now.second == 0) && !notStartedDTMF) {
        ENTER LAST DIGIT OF PASSWORD
        //Reset notStartedDTMF now that we're done:
        notStartedDTMF = true;
    }
}