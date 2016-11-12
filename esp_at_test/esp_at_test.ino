// Auruino transparent to ESP

#include <SoftwareSerial.h>

SoftwareSerial espSerial(2, 3); // RX, TX

// safe write to espSerial
void espSerialWrite(String text) {
  int len = text.length();
  for (int i = 0; i < len; i++) {
    espSerial.write(text[i]);
    delay(10);
  }
}

// submit command to ESP and return the result
String espSubmit(String text) {
  // block ESP background job
  espSerialWrite("mcu_busy = true\r"); 
  // read garbages from esp
  while (espSerial.available()) {
    espSerial.read();
    delay(5);
  }
  espSerial.overflow(); // clear overflow flag
  delay(200);
  // submit command
  espSerialWrite(text);
  delay(1000); // wait for finish command execution
  // get result
  String rs = "";
  if ( espSerial.overflow() ) {
    Serial.println("error: espSerial RX is overflow.");
  } else {
    while (espSerial.available()) {
      char c = espSerial.read();
      rs = rs + c;
      delay(5);
    }
  }
  // stop blocking ESP background job
  espSerialWrite("mcu_busy = nil\r");
  return rs;
}

int RSTBPin = 7;

void setup() {
  // RSTB pin
  //pinMode(RSTBPin, OUTPUT);

  // reset
  //digitalWrite(RSTBPin, LOW);
  //delay(100);
  //digitalWrite(RSTBPin, HIGH);

  //delay(10000);
  Serial.begin(9600);  // 9600, 14400, 19200, 28800, 31250, 38400, 57600, 115200
  Serial.print("arduino starting... ");
  espSerial.begin(9600);
  Serial.println("ready.");

  //delay(5000);
  //espSerial.print("AT");
  //espSerial.print("\r");
  //digitalWrite(RSTBPin, LOW);
  //delay(2);
  //digitalWrite(RSTBPin, HIGH);

  delay(2000);

  int dly = 100;
  espSerial.write("\r");
  delay(dly);
  espSerial.write("\r");
  delay(dly);
  //espSerial.write("print(\"AadsfsafdsafdsafasfdsafasfdfffB\");\r");
  //espSerial.write("fappend(\"thermoLog.txt\",\"aaaa\\n\")\r");
  Serial.println("start testing");
  delay(dly);
  espSerialWrite("mcu_busy = true\r");
  delay(dly);
  espSerialWrite("file.remove(\"a.txt\")\r");
  delay(dly);
  espSerialWrite("file.remove(\"e.txt\")\r");
  delay(dly);
  espSerialWrite("ard.file=\"a.txt\"\r");
  delay(dly);
  espSerialWrite("ard.cnt=0\r");
  delay(dly);
  espSerialWrite("ard.f[1]=\"aaaaaaaaaaaaaaaaaaaa\"\r");
  delay(dly);
  for (int i = 0; i < 0; i++) {
    espSerialWrite("ard.f[0]=" + String(i) + "\r");
    delay(dly);
    espSerialWrite("ard.test()\r");
    delay(dly);
  }
  espSerialWrite("mcu_busy = nil\r");
  delay(dly);


  String r = espSubmit("mydate()\r");
  Serial.println(r);
  //r = espSubmit("myls()\r");
  //Serial.println(r);
  //r = espSubmit("print(444)\r");
  //Serial.println(r);
}

void loop() { // run over and over
  if (espSerial.available()) {
    Serial.write(espSerial.read());
  }
  if (Serial.available()) {
    //Serial.read(); espSerial.write("print(\"a\")\r\n");
    espSerial.write(Serial.read());
    //Serial.println("sent.");
  }
}


