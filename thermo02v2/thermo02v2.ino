//
//
//

#include <DateTime.h>
#include <EEPROM.h>
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

boolean resonseToEsp = false;
void printlnResponse(String line) {
  if ( resonseToEsp ) {
    int dly = 100;
    delay(dly);
    espSerialWrite("mcu_busy = true\r");
    delay(dly);
    //    espSerialWrite("ard.d=mynow()\r");
    //    delay(dly);
    //    espSerialWrite("ard.fappendln()\r");
    //    delay(dly);
    espSerialWrite("ard.d=\"" + line + "\"\r");
    delay(dly);
    espSerialWrite("ard.fappendln()\r");
    delay(dly);
    espSerialWrite("mcu_busy = nil;\r");
    delay(dly);
  } else {
    Serial.println(line);
  }
}

//---------------------------------------------------------------

#define EEPROM_ADDR_MAX  900

//
int sensorPin = A0;   //アナログ0番ピン
int relayPin = 4;    //digital 4番ピン
int internalLedPin = 13;  //digital 13番ピン

// status variables
time_t savedTime; // the time at whith saved
char mode = 'n';
word espInterval = 1800;
word eepromAddr = 0;
byte relayOn;
int thermoHigh = 1000; // in 0.1℃
int thermoLow  = -1000; // in 0.1℃
word thermoInterval = 20; // sec

// globals
time_t prevtime = 0;
word sensorInterval = 10;

void setup() {
  Serial.begin(9600);           //シリアルモニタに表示する為の設定
  Serial.println("arduino starting");

  // read eeprom
  loadStatus(); // from eeprom

  // set clock
  //savedTime = DateTime.makeTime(30, 59, 23, 1, 1, 2000);
  DateTime.sync(savedTime);
  Serial.print("arduino set clock to the time read from eeprom: ");
  Serial.println(digitalClock());

  // analog sensor input
  analogReference(INTERNAL); // DEFAULT:5V, INTERNAL:1.1V
  //analogReference(DEFAULT); // DEFAULT:5V, INTERNAL:1.1V

  // relay pin
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  relayOn = 0;

  // wait -- blinking LED
  Serial.print("wait...");
  pinMode(internalLedPin, OUTPUT); // initialize digital pin 13 as an output.
  for (int i = 0; i < 30; i++) {
    digitalWrite(internalLedPin, HIGH);   // turn the LED on
    delay(300);
    digitalWrite(internalLedPin, LOW);   // turn the LED off
    delay(200);
  }
  Serial.println("arduino invoked.");

  // connect to esp
  Serial.println("connect to ESP8266");
  espSerial.begin(9600);
  delay(200);
  espSerial.write("\r");
  //delay(200);
  //espSerialWrite("file.remove(\"thermo.txt\")\r");
  delay(200);
  espSerialWrite("ard.csvfile=\"thermo.txt\"\r");
  delay(200);
  espSerialWrite("ard.fcnt=4\r");
  delay(200);

  Serial.println("start loop.");
}

void loop() {
  time_t curtime = DateTime.now();
  String cmdFromESP;

  //----- get cmd from ESP
  if ( curtime % 20 == 0 ) {
    cmdFromESP = espSubmit("print(ard.cmd)\r");
    espSerialWrite("ard.cmd=\"\"\r");
    cmdFromESP.trim();
    if ( cmdFromESP.endsWith(">") ) {
      cmdFromESP.remove(cmdFromESP.length() - 1);
    }
    cmdFromESP.trim();
    Serial.println("arduino cmd from ESP: " + cmdFromESP);
  }

  //----- handling command from host
  if ( Serial.available() > 0 || cmdFromESP != "" ) {
    char rd = cmdFromESP[0];
    String arg = cmdFromESP.substring(1);
    resonseToEsp = true;
    if ( Serial.available() > 0 ) {
      rd = Serial.read();
      arg = Serial.readStringUntil('\r');
      resonseToEsp = false;
    }
    Serial.print("arduino receive ");
    Serial.println(rd);
    if ( rd == '?' ) { // help
      printlnResponse(F("p print"));
      printlnResponse(F("w write mode"));
      printlnResponse(F("n normal mode"));
      printlnResponse(F("s save status"));
      printlnResponse(F("i set print interval"));
      printlnResponse(F("I set esp8266 interval"));
      printlnResponse(F("t set time"));
      printlnResponse(F("r print temperature from eeprom"));
      printlnResponse(F("a set eeprom next address"));
      printlnResponse(F("h set thermo high"));
      printlnResponse(F("H set thermo low"));
      printlnResponse(F("o set thermo interval"));
      printlnResponse(F("v relay turn on"));
      printlnResponse(F("V relay turn off"));
    } else if ( rd == 'p' ) { // print setings
      printStatus();
      String a = "arduino print current ";
      printlnResponse(a + "time " + digitalClock());
      printlnResponse(a + "mode = " + String(mode));
      printlnResponse(a + "sensor interval = " + String(sensorInterval));
      printlnResponse(a + "esp8266 interval = " + String(espInterval));
      printlnResponse(a + "eeprom addr = " + String(eepromAddr));
      printlnResponse(a + "thermo high = " + String(thermoHigh));
      printlnResponse(a + "thermo low  = " + String(thermoLow));
      printlnResponse(a + "thermo interval  = " + String(thermoInterval));
      printlnResponse(a + "relay status = " + String(relayOn));
    } else if (rd == 'w') { // eeprom write mode
      mode = rd;
      printlnResponse("arduino set mode = " + mode);
      printlnResponse("arduino start saving temp. to eeprom with addr=0x0");
      eepromAddr = 0;
    } else if ( rd == 'n' ) { // normal mode
      mode = rd;
      printlnResponse("arduino set mode = " + mode);
      printlnResponse("arduino stop saving temp. to eeprom");
    } else if ( rd == 't' ) { // set time ex. t201512241923.
      //String arg = Serial.readStringUntil('.');
      time_t ttime = DateTime.makeTime(0, arg.substring(10, 12).toInt(), arg.substring(8, 10).toInt(), arg.substring(6, 8).toInt(), arg.substring(4, 6).toInt(), arg.substring(0, 4).toInt());
      DateTime.sync(ttime);
      printlnResponse("arduino set time to " + arg);
    } else if ( rd == 'i' ) { // set interval
      //String arg = Serial.readStringUntil('.');
      sensorInterval = arg.toInt();
      printlnResponse("arduino set interval to " + arg);
    } else if ( rd == 'I' ) { // set eeprom interval
      //String arg = Serial.readStringUntil('.');
      espInterval = arg.toInt();
      printlnResponse("arduino set eeprom interval to " + arg);
    } else if ( rd == 'a' ) { // set eeprom address
      //String arg = Serial.readStringUntil('.');
      eepromAddr = arg.toInt();
      printlnResponse("arduino set eeprom (next) address to " + arg);
    } else if ( rd == 'h' ) { // set thermo high
      //String arg = Serial.readStringUntil('.');
      thermoHigh = arg.toInt();
      printlnResponse("arduino set thermo high to " + arg);
    } else if ( rd == 'H' ) { // set thermo low
      //String arg = Serial.readStringUntil('.');
      thermoLow = arg.toInt();
      printlnResponse("arduino set thermo low to " + arg);
    } else if ( rd == 'o' ) { // set thermo interrval
      //String arg = Serial.readStringUntil('.');
      thermoInterval = arg.toInt();
      printlnResponse("arduino set thermo low to " + arg);
      if ( sensorInterval > thermoInterval ) {
        sensorInterval = thermoInterval;
        printlnResponse("arduino change sensor interval to " + arg);
      }
    } else if ( rd == 's' ) { // save settings
      saveStatus();
    } else if ( rd == 'r' ) { // read eeprom
      //String arg = Serial.readStringUntil('.');
      int eepmax = arg.toInt();
      printlnResponse("arduino read eeprom addr 0 to " + String(eepmax));
      for (int a = 0; a <= eepmax && a < 200; a++) {
        int val = EEPROM.read(a * 5);
        time_t t;
        EEPROM.get(a * 5 + 1, t);
        printlnResponse(String(a) + "#," + stringDateTime(t) + "%,"
                        + String(modTemp(val)) + "|," + String(val, DEC));
      }
    } else if ( rd == 'v' ) { // turn on relay
      relayTurnOn();
    } else if ( rd == 'V' ) { // turn off relay
      relayTurnOff();
    } else {
      printlnResponse("arduino warning illegal command syntax");
    }
  }

  // -----sensor read
  if ( curtime % sensorInterval == 0 && curtime != prevtime ) {
    prevtime = curtime;
    int sensorValue;
    float sum = 0.0;
    float sumsq = 0.0;
    for (int i = 0; i < 10; i++) { //空読み
      sensorValue = analogRead(sensorPin);    //アナログ0番ピンからの入力値を取得
      delay(2);
    }
    for (int i = 0; i < 20; i++) {
      sensorValue = analogRead(sensorPin);    //アナログ0番ピンからの入力値を取得
      sum += (float)sensorValue;
      sumsq += (float)sensorValue * (float)sensorValue;
      delay(2);
    }
    float ave = sum / 20.0; // ave
    float sdev = sumsq / 20.0 - ave * ave;
    if ( sdev < 0 ) {
      sdev = 0;
    }
    sdev = sqrt(sdev); //std. dev.
    sensorValue = int(ave + 0.5); // rounding
    float temp  = modTemp(sensorValue);     //温度センサーからの入力値を変換
    printDateTime(curtime);
    Serial.println(" " + String(ave) + " -> " + String(sensorValue)
                   + String(" sd=") + String(sdev)
                   + String(" temp. ") + String(temp));

    //----- write to eeprom
    if ( mode == 'w' && prevtime % 7200 == 0 ) {
      if ( eepromAddr * 5 + 4 > EEPROM_ADDR_MAX ) eepromAddr = 0;
      EEPROM.update(eepromAddr * 5, (byte)sensorValue);
      EEPROM.put(eepromAddr * 5 + 1, curtime);
      //saveStatus();
      Serial.print("arduino write to eeprom with addr = ");
      Serial.println(eepromAddr);
      eepromAddr++;
    }

    //----- write to esp
    if ( mode == 'w' && prevtime % espInterval == 0 ) {
      writeTempToESP(temp, ave, sdev);
      Serial.println("arduino write to ESP");
    }

    //----- thermostat
    if ( curtime % thermoInterval == 0 ) {
      if (temp > thermoHigh / 10.0) {
        relayTurnOff();
      } else if (temp < thermoLow / 10.0) {
        relayTurnOn();
      }
    }
  }

  delay(200);                           //in msec.
}

void writeTempToESP(float temp, float ave, float sdev) {
  // write to ESP8266
  //espSerialWrite("fappendFile=\"thermo.txt\"\r\n");
  int dly = 200;
  delay(dly);
  espSerialWrite("mcu_busy = true\r");
  delay(dly);
  espSerialWrite("ard.f[0]=mynow()\r");
  delay(dly);
  //Serial.println("ard.f[1]=" + String(temp*100,0) + "\r");
  espSerialWrite("ard.f[1]=" + String(temp * 100, 0) + "\r");
  delay(dly);
  espSerialWrite("ard.f[2]=" + String(ave * 100, 0) + "\r");
  delay(dly);
  espSerialWrite("ard.f[3]=" + String(sdev * 100, 0) + "\r");
  delay(dly);
  espSerialWrite("ard.fappendcsv()\r");
  delay(dly+500);
  espSerialWrite("ard.appendgss()\r");
  delay(dly+500);
  espSerialWrite("mcu_busy = nil;\r");
  delay(dly);
}

//アナログ入力値を摂氏度℃に変換
float modTemp(int analog_val) {
  //float v  = 5;     // 基準電圧値( V )
  float v  = 1.1;     // 基準電圧値( V )
  float tempC = ((v * analog_val) / 1024) * 100;  // 摂氏に換算
  return tempC;
}

String digitalClock() {
  DateTime.available();
  return stringDateTime(DateTime.now());
}

String stringDateTime(time_t tm) {
  byte second, minute, hour, day, wday, month, year;
  DateTime.localTime(&tm, &second, &minute, &hour, &day, &wday, &month, &year);

  String s;
  s = String(year + 1900, DEC) + "/" + stringDigits(month)
      + "/"  + stringDigits(day) + " " + stringDigits(hour) + ":"
      + stringDigits(minute) + ":" + stringDigits(second);
  return s;
}

void printDateTime(time_t tm) {
  Serial.print(stringDateTime(tm));
}

String stringDigits(byte digits) {
  if (digits < 10)
    return "0" + String(digits, DEC);
  else
    return String(digits, DEC);
}

// relay
void relayTurnOn() {
  relayOn = 1;
  digitalWrite(relayPin, HIGH);
  digitalWrite(internalLedPin, HIGH);
  Serial.println("arduino turn on relay");
}
void relayTurnOff() {
  relayOn = 0;
  digitalWrite(relayPin, LOW);
  digitalWrite(internalLedPin, LOW);
  Serial.println("arduino turn off relay");
}

//---------------------------------------
void saveStatus() {
  EEPROM.put(1023, mode);//char
  savedTime = DateTime.now();
  EEPROM.put(1019, savedTime);
  EEPROM.put(1015, espInterval);//word
  EEPROM.put(1011, eepromAddr);//word
  EEPROM.put(1007, thermoHigh);//word
  EEPROM.put(1003, thermoLow);//word
  EEPROM.put(999, thermoInterval);//word
  Serial.println(F("arduino saved settings to eeprom"));
}

void loadStatus() {
  EEPROM.get(1023, mode);
  EEPROM.get(1019, savedTime);
  EEPROM.get(1015, espInterval);
  EEPROM.get(1011, eepromAddr);
  EEPROM.get(1007, thermoHigh);//word
  EEPROM.get(1003, thermoLow);//word
  EEPROM.get(999, thermoInterval);//word
  Serial.println(F("arduino load settings from eeprom"));
}

void printStatus() {
  String a = F("arduino print saved ");
  time_t eetime;
  char m;
  word ee;
  int eee;
  EEPROM.get(1019, eetime);
  Serial.print(a + F("  time "));
  Serial.println(stringDateTime(eetime));
  Serial.print(a + F("  mode = "));
  EEPROM.get(1023, m);
  Serial.println(m);
  Serial.print(a + F("  esp8266 interval = "));
  EEPROM.get(1015, ee);
  Serial.println(ee);
  Serial.print(a + F("  eeprom addr = "));
  EEPROM.get(1011, ee);
  Serial.println(ee);
  Serial.print(a + F("  thermo high = "));
  EEPROM.get(1007, eee);
  Serial.println(eee);
  Serial.print(a + F("  thermo low = "));
  EEPROM.get(1003, eee);
  Serial.println(eee);
  Serial.print(a + F("  thermo interval = "));
  EEPROM.get(999, ee);
  Serial.println(ee);
}

