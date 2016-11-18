//
// Example sketch for MyLib, MyOTA, MyCockpit, NTP
//  - connect WiFi in STA mode
//  - NTP network time sync
//  - run OTA server
//  - run cockpit web server

extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>

CheckInterval CI(1000*5); // interval [ms]

ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open

//LogFile tlog("/testlog.txt", 1000);

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP......................");
  //DebugOut.setToFile();
  //Serial.setDebugOutput(true);
  //WiFi.printDiag(DebugOut);
    
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

  if( CI.check() ) { 
    DebugOut.println(getDateTimeNow()+" VCC: "+ESP.getVcc()/1024.0);
    //DebugOut.println(getDateTimeNow()+" A0: "+analogRead(A0)/1024.0);
    //tlog.println(getDateTimeNow() + "sadakane");
    //triggerIFTTT("basic",getDateTimeNow(),String(ESP.getVcc()/1024.0),"");
  }
  delay(100);
}

