//
// ESP master - Arduino slave model
// - ESP H/W Serial connected to Arduino S/W Serial
//   - connect Arduino S/W Serial TX port to ESP RX thru voltage divider
//   - ESP TX is open 

extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <ESP8266WiFi.h>
//#include <SoftwareSerial.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>

//SoftwareSerial espSerial(5, 4); // RX, TX

CheckInterval CI(3000);

ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open

LogFile tlog("/testlog.txt", 20000);

void setup() {
  Serial.begin(9600);
  Serial.println("");
  Serial.println("start ESP......................");
  //Serial.swap(); // !!!! SWAP to GPIO15(TX) and GPIO13(RX)
  DebugOut.setToFile();

  //espSerial.begin(9600);
  
  wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
  WiFiConnect();
  printSystemInfo();

  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  setupMyOTA();
  addHtmlMyCockpit(String("Sketch: ")+THIS_SKETCH+"<BR><BR>");
  addMyCockpit("/interval", 1, [](){
    String n = server.arg(0);
    CI.set(n.toInt());
    server.send(200, "text/plain", n + ", ok");
  });
  setupMyCockpit();
}

void loop() {
  loopMyOTA();
  loopMyCockpit();

  delay(50);

  if( CI.check() ) { 
    DebugOut.println(getDateTimeNow()+String(" VCC: ")+ESP.getVcc()/1024.0);
    //tlog.println(getDateTimeNow() + "sadakane");
    String s = "abcdefg";
    delay(100);
    espSerialWrite(s);
    delay(100);
    String t = espSerialRead();
    delay(100);
    DebugOut.println(getDateTimeNow()+s+" "+t+" : "+(s==t));          
  }
  delay(10);
}

// safe read from Serial
String espSerialRead() {
  String rs = "";
  while (Serial.available()) {
      char c = Serial.read();
      rs = rs + c;
      delay(5);
  }
  return rs;
}

// safe write to Serial
void espSerialWrite(String text) {
  int len = text.length();
  for (int i = 0; i < len; i++) {
    Serial.write(text[i]);
    delay(10);
  }
}


