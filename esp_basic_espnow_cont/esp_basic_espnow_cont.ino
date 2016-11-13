
// This program include just basic/standard functions:
// use this for test

extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>

//--- DHT -----------------------------------
#include "DHT.h"
#define DHTPIN 4     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT22, AM2320
DHT dht(DHTPIN, DHTTYPE);
//-------------------------------------------

CheckInterval CI(1000 * 60);

//ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open

LogFile tlog("/testlog.txt", 1000);

uint8_t *mac = macAddrAP[4];  // slave AP mac address

int espMode;

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP......................");
  //DebugOut.setToFile();
  //Serial.setDebugOutput(true);
  //WiFi.printDiag(DebugOut);

  dht.begin();

  // json config
  if ( ESP_rtcUserMemoryRead().startsWith("{") && jsonConfig.loadRtcMem() ) {
    ESP_rtcUserMemoryWrite("");
  } else {
    jsonConfig.load();
  }
  JsonObject &conf = jsonConfig.obj();
  // set default
  if ( !conf["numPoll"] ) {
    conf["numPoll"] = 10;
    jsonConfig.save();
  }
  if ( !conf["interval"] ) {
    conf["interval"] = 1000 * 60 * 5;
    jsonConfig.save();
  }
  //
  unsigned long t = conf["interval"];
  CI.set(t);

  if ( conf["mode"] == String("EspNow") || conf["mode"] == String("EspNowDSleep") ) {
    // EspNow mode setup (No WiFi)
    espMode = 1;
    setupEspNow(NULL, NULL, NULL);

  } else {
    // STA mode setup
    espMode = 0;
    wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
    WiFiConnect();
    printSystemInfo();

    ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
    setupMyOTA();
    addHtmlMyCockpit(String("Sketch: ") + THIS_SKETCH + "<BR><BR>");
    addMyCockpit("/interval", 1, []() {
      String n = server.arg(0);
      CI.set(n.toInt());
      jsonConfig.obj()["interval"] = (unsigned long)n.toInt(); // cast ok?
      jsonConfig.save();
      server.send(200, "text/plain", n + ", ok");
    });
    addMyCockpit("/numPoll", 1, []() {
      String n = server.arg(0);
      jsonConfig.obj()["numPoll"] = n.toInt();
      jsonConfig.save();
      server.send(200, "text/plain", n + ", ok");
    });
    addMyCockpit("/toEspNow", 0, []() {
      jsonConfig.obj()["mode"] = "EspNow";
      jsonConfig.save();
      server.send(200, "text/plain", "ok");
    });
    addMyCockpit("/toEspNowDSleep", 0, []() {
      jsonConfig.obj()["mode"] = "EspNowDSleep";
      jsonConfig.save();
      server.send(200, "text/plain", "ok");
    });
    setupMyCockpit();
  }
}

void loop() {
  JsonObject &conf = jsonConfig.obj();

  if ( espMode == 1 ) { // ESPNOW (non-WiFi) mode
    // counter increment
    int cnt = conf["cntDSleep"] ? conf["cntDSleep"] : 0;
    conf["cntDSleep"] = cnt + 1;
    jsonConfig.saveRtcMem(); //  use RTC memory
    // do userFunc
    int numPoll = conf["numPoll"];
    if ( cnt % numPoll == 0 ) {
      sendEspNow(mac, userFunc());
    }
    // poll req
    uint8_t pollReq[] = ESPNOW_REQ_POLL;
    sendEspNow(mac, pollReq, 4);
    delay(500); // wait poll action

    // re-action for request
    for (int i = 0; i < espNowBuffer.recvReqBufferMax(); i++ ) { // for each request in buffer
      uint8_t type = espNowBuffer.recvReq[i].data[3];
      DebugOut.println(type);
      if ( type == 2 ) { // wakeup req
        DebugOut.println("get wakeup packet. exit esp-now mode...");
        conf["mode"] = "STA";
      }
    }
    espNowBuffer.recvReqNum = 0; // clear req buffer

    // change mode
    if ( conf["mode"] == "STA" ) {
      jsonConfig.save();
      SPIFFS.end();
      ESP.restart();
    }
    // delay/sleep
    if ( conf["mode"] == String("EspNow") ) { // no sleep
      delay(CI.get() / numPoll);
    } else {
      DebugOut.println("entering deep sleep...");
      SPIFFS.end();
      ESP.deepSleep(CI.get() * 1000 / numPoll, WAKE_RF_DEFAULT); // connect GPIO16 to RSTB
      delay(1000); // wait for getting sleep
    }

  } else { // STA mode
    // change mode
    if ( conf["mode"] == String("EspNow") || conf["mode"] == String("EspNowDSleep") ) {
      DebugOut.println("switch to esp-now flow...");
      SPIFFS.end(); ESP.restart();
    }

    loopMyOTA();
    loopMyCockpit();

    if ( CI.check() ) {
      String s = userFunc();
      DebugOut.println(s);
      DebugOut.println(getDateTimeNow() + " VCC: " + ESP.getVcc() / 1024.0);
    }
    delay(1000);
  }
}

String userFunc() {
  //    String s = "espnow:"+String(millis())+" "+ESP.getVcc()/1024.0;
  double a = analogRead(A0) / 1024.0;
  String d = getDHT();
  String s = d + ", " + a;
  return s;
}


String getDHT() {
  String out = "";
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)

  float t = dht.readTemperature(); // read force
  float h = dht.readHumidity(); // read not force
  // Check if any reads failed and exit early (to try again).
  if( isnan(h) || isnan(t) ) {
    delay(2300); // > 2sec
    t = dht.readTemperature(); // read force
    h = dht.readHumidity(); // read not force
  }
  if( isnan(h) || isnan(t) ) {
    return "Failed to read from DHT sensor!";
  }
  
  // Compute heat index in Celsius
  float hi = dht.computeHeatIndex(t, h, false);

  return String("Humidity: ") + h + " %  " + "Temp.: " + t + " *C " +
       "Heat index: " + hi;
}

