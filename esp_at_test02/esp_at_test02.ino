/*
  TEST for ESP01
  - connecting Arduino H/W Serial to ESP
  - Arduino generates and sends AT commands
  - Arduino save the serial communication to eeprom
  usage:
  - connect Arduino TX(1) to ESP RX (via level converter)
  - upload this program to Arduino
  - connect Arduino serial RX(0) to ESP TX
  - (shut down Serial monitor)
  - reset Arduino by reset button
  - reset ESP by jumper wire within 10 minites
  ref: https://www.zybuluo.com/kfihihc/note/31135
*/

#include <EEPROM.h>

int RSTBPin = 7;
int internalLedPin = 13;  //digital 13番ピン

void setup() {
  int addr = 0;

  // pins
  pinMode(RSTBPin, OUTPUT);
  pinMode(internalLedPin, OUTPUT);
  digitalWrite(internalLedPin, LOW);

  Serial.begin(115200);  // 9600, 14400, 19200, 28800, 31250, 38400, 57600, 115200
  Serial.setTimeout(5000);

  // auto reset ESP
  //digitalWrite(RSTBPin, LOW);
  //delay(100);
  digitalWrite(RSTBPin, HIGH);

  delay(10000); // wait for manual reset ESP
  digitalWrite(internalLedPin, HIGH);


  //test if the module is ready
  //Serial.print("AT+RST"); // print command seems to also flash the input buffer
  //Serial.print("\r\n");
  delay(3000);
  digitalWrite(internalLedPin, LOW);
  while (Serial.available()) {
    char b = Serial.read();
    EEPROM.put(addr, b);
    addr++;
  }
  if( addr != 0 ) {
    EEPROM.put(1000, addr);
  }
  delay(1000);
  digitalWrite(internalLedPin, HIGH);
}

void loop() { // run over and over
  if ( Serial.available() > 0 ) {
    String key = Serial.readStringUntil('.');
    Serial.print("arduino receive ");
    Serial.println(key);
    if ( key == "rrrr" ) { // read eeprom
      int addr;
      EEPROM.get(1000, addr);
      Serial.print("arduino read eeprom addr 1000: ");
      Serial.println(addr);

      for (int a = 0; a < addr && a < 200; a++) {
        char val = EEPROM.read(a);
        Serial.print(val);
      }
      Serial.println("");
    }
  }
  delay(200);
}


