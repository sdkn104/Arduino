// This program include just basic/standard functions:
//   OTA, NTP, Cockpit
// use this for test

extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>

CheckInterval CI(1000*60*60);
CheckInterval CIddns(1000*60*60*24);

ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open

LogFile tlog("/testlog.txt", 1000);

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP......................");
  //DebugOut.setToFile();
  
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
  
  //HTTPGet("http://www.yahoo.co.jp/");
  //HTTPGet("http://www43.tok2.com/home/pine104/index.html");
}

void loop() {
  loopMyOTA();
  loopMyCockpit();

  if( CIddns.check() ) {
    updateDDNS();
  }

  if( CI.check() ) { 
    DebugOut.println(getDateTimeNow()+" VCC: "+ESP.getVcc()/1024.0);
//    DebugOut.println(getDateTimeNow()+" A0: "+analogRead(A0)/1024.0);
    //tlog.println(getDateTimeNow() + "sadakane");
    //triggerIFTTT("basic",getDateTimeNow(),String(ESP.getVcc()/1024.0),"");
  }
  delay(100);
}

