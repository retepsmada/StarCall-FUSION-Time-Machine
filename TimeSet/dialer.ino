void setTime(){
  /* Sets the school clocks by the minute according to the real-time clock */

  digitalWrite(phoneHook, LOW); //Make sure phone is on hook
  
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
