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

////CheckInterval CI(1000*60*5);

////ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open

////LogFile tlog("/testlog.txt", 1000);

#define DebugOut Serial

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.setDebugOutput(true);
  WiFi.mode(WIFI_STA); 
//  wifi_set_sleep_type(MODEM_SLEEP_T); // default=modem
  wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
//  wifi_set_sleep_type(NONE_SLEEP_T); // default=modem

  Serial.println("");
  Serial.println("start ESP......................");
////  DebugOut.setToFile();
  
////  WiFiConnect();
/*
  const char* ssid = "HG8045-FB5C-bg";
  const char* password = "57dtsyne";
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DebugOut.print(".");
  }
  DebugOut.println(" connected");
  DebugOut.print("local IP: ");
  DebugOut.println(WiFi.localIP().toString());
*/
  Serial.println("delay 0......................");
  delay(5000);
  Serial.println("delay 1......................");
  delay(5000);

#if 1
  Serial.println("force sleep......................");
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);  // set WiFi mode to null mode.
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); // light sleep
  wifi_fpm_open(); // enable force sleep
  wifi_fpm_set_wakeup_cb(fpm_wakup_cb_func1); // Set wakeup callback
  int r = wifi_fpm_do_sleep(20000*1000); // us
  Serial.println(r); // 0:success
  delay(10000); // 1000ok, 100ok, 1 ng, 0 ng, 
  //delay(1); delayMicroseconds(500*1000); // 1-1000*1000 ok, 0-1000*2000 ng
//  Serial.println("after sleep 1......................");
#else
  Serial.println("deep sleep......................");
  ESP.deepSleep(60 * 1000000, RF_DISABLED);
  delay(10000);
#endif
    
//  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
////delay(5000);
/*  addHtmlMyCockpit(String("Sketch: ")+THIS_SKETCH+"<BR><BR>");
  addMyCockpit("/interval", 1, [](){
    String n = server.arg(0);
////    CI.set(n.toInt());
    server.send(200, "text/plain", n + ", ok");
  });
  setupMyCockpit();
*/
}

void loop() {
//  if( WiFi.isConnected() ) loopMyOTA();
//  if( WiFi.isConnected() ) loopMyCockpit();

////  if( CI.check() ) { 
////    DebugOut.println(getDateTimeNow()+" VCC: "+ESP.getVcc()/1024.0);
//    DebugOut.println(getDateTimeNow()+" A0: "+analogRead(A0)/1024.0);
    //tlog.println(getDateTimeNow() + "sadakane");
    //triggerIFTTT("basic",getDateTimeNow(),String(ESP.getVcc()/1024.0),"");
////  }
  
//  Serial.println("loop ESP......................");
//  delay(100);
//  if( WiFi.isConnected() ) Serial.println("connect ESP......................");
//  delay(1000);

//  static int cnt = 0; cnt++;
//  if( cnt ==  5 ) WiFi.disconnect(false);
//  if( cnt == 30 ) WiFiConnect();
  delay(5000);
}


void fpm_wakup_cb_func1(void) {
  Serial.println("wakeup cb......................");
  wifi_fpm_close();       // disable force sleep function
  wifi_set_opmode(WIFI_STA);
//  wifi_station_connect(); ?? ?? ?  // connect to AP
}




