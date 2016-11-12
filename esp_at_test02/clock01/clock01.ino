#include <Wire.h>

// Clock

// Original:
// DateTime.pde
// example sketch for the DateTime library

#include <DateTime.h>
//#include <DateTimeStrings.h>

void setup() {
  Serial.begin(9600);
  time_t  prevtime;
  prevtime = DateTime.makeTime(0,0,0,1,1,2000);
  DateTime.sync(prevtime);
}

void  loop() {
//  while( prevtime == DateTime.now() ){
//    delay(100);
//  }
  DateTime.available();
  digitalClockDisplay();
  delay(1000);
}

void digitalClockDisplay() {
  // digital clock display of current date and time
  Serial.print(DateTime.Hour, DEC);
  printDigits(DateTime.Minute);
  printDigits(DateTime.Second);
  Serial.print(" ");
  Serial.print(DateTime.Year+1900,DEC);
  Serial.print("/");
  Serial.print(DateTime.Month,DEC);
  Serial.print("/");
  Serial.println(DateTime.Day, DEC);
}

void printDigits(byte digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits, DEC);
}

