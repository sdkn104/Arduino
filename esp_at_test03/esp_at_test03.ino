/*
  TEST for ESP01
  - use Arduino as USB-Serial converter
  usage:
  - upload this program to Arduino (this just turn off ATMEGA TX/RX)
  - run the program, and wait a second
  - connect Arduino serial RX(0) to ESP RX (via level converter)
  - connect Arduino serial TX(1) to ESP TX (!! dont connect unless running this program)
  - reset ESP by reset button
  ref: http://www.elec-cafe.com/esp8266-esp-01-firmware-update/
*/

int internalLedPin = 13;  //digital 13番ピン

void setup() {
  // disable Serial (not needed?)
  delay(1000); // wait for boot loader finish??
  Serial.end();
  pinMode(1,INPUT); // disable TX
  
  // initialize digital pin 13 as an output.
  pinMode(internalLedPin, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(internalLedPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);              // wait for a second
  digitalWrite(internalLedPin, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);              // wait for a second
}

