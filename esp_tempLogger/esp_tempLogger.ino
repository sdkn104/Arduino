
// DHT temp/humid sensor logger with ESPNOW

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

CheckInterval CI; // for STA mode

ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open

uint8_t *slaveMac = macAddrAP[4];  // slave AP mac address

int espMode;

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP......................");
  //DebugOut.setToFile();
  //Serial.setDebugOutput(true);
  //WiFi.printDiag(DebugOut);

  dht.begin();

  // load json config
  if ( ESP_rtcUserMemoryRead().startsWith("{") && jsonConfig.loadRtcMem() ) {
    ESP_rtcUserMemoryWrite(""); // delete
  } else {
    jsonConfig.load();
  }
  JsonObject &conf = jsonConfig.obj();
  // set default json
  if ( !conf["numPoll"] ) {
    conf["numPoll"] = 10;
    jsonConfig.save();
  }
  if ( !conf["interval"] ) {
    conf["interval"] = 1000 * 60 * 5;
    jsonConfig.save();
  }
  // set interval for STA mode
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

    // cockpit menu
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
    loopEspnowController(userFunc, reqReaction, slaveMac);    
  } else { // STA mode
    // change mode
    if ( conf["mode"] == String("EspNow") || conf["mode"] == String("EspNowDSleep") ) {
      DebugOut.println("switch to esp-now flow...");
      SPIFFS.end(); ESP.restart();
    }

    loopMyOTA();
    loopMyCockpit();

    if ( CI.check() ) {
      String s = getMessage();
      DebugOut.println(s);
      DebugOut.println(getDateTimeNow() + " VCC: " + ESP.getVcc() / 1024.0);
    }
    delay(500);
  }
}

String getMessage() {
  double a = ESP.getVcc() / 1024.0;
  //double a = analogRead(A0) / 1024.0;
  String d = getDHT();
  String s = d + ", " + a;
  return s;
}

void userFunc() {
  sendEspNow(slaveMac, getMessage(), 0);
}

void reqReaction(int reqid) {
  JsonObject &conf = jsonConfig.obj();
  uint8_t *req = espNowBuffer.recvReq[reqid].data;
  uint8_t type = req[3];
  DebugOut.println(type);
  if ( type == 2 ) { // wakeup req
    DebugOut.println("get wakeup packet. exit esp-now mode...");
    conf["mode"] = "STA";
  }
}

// get sensor values
String getDHT() {
  String out = "";
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)

  float t = dht.readTemperature(); // read force
  float h = dht.readHumidity(); // read not force
  // Check if any reads failed and exit early (to try again).
  if ( isnan(h) || isnan(t) ) {
    delay(2300); // > 2sec
    t = dht.readTemperature(); // read force
    h = dht.readHumidity(); // read not force
  }
  if ( isnan(h) || isnan(t) ) {
    return "Failed to read from DHT sensor!";
  }

  // Compute heat index in Celsius
  float hi = dht.computeHeatIndex(t, h, false);

  return String("Humidity: ") + h + " %  " + "Temp.: " + t + " *C " +
         "Heat index: " + hi;
}



