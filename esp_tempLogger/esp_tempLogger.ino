
// DHT temp/humid sensor logger with ESPNOW
//  - connect GPIO15 to RSTB for deep sleep

extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <espnowLib.h>
#include <MyCockpit.h>

int id = 10; // espnow device ID

//--- DHT -----------------------------------
#include "DHT.h"
#define DHTPIN 4     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT22, AM2320
DHT dht(DHTPIN, DHTTYPE);
//-------------------------------------------

CheckInterval CI; // for STA mode

// use analogRead() instead of getVcc(). 
//   - use for the device that analog IO pin connected to VDD
//#define GET_VCC_BY_ANALOG_READ 

#ifdef GET_VCC_BY_ANALOG_READ  
#else
ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open
#endif

uint8_t *slaveMac = macAddrAP[8];  // slave AP mac address

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
  jsonConfig.load();
  jsonConfigFlush();
  JsonObject &conf = jsonConfig.obj();

  if ( conf["mode"] == String("EspNow") || conf["mode"] == String("EspNowDSleep") ) {
    // EspNow mode setup (No WiFi)
    espMode = 1;
    WiFi.mode(WIFI_STA);
    setupEspNow(NULL, NULL, NULL);

  } else {
    // STA mode setup
    espMode = 0;
    wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
    WiFiConnect(id);
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
    addMyCockpit("/status", 0, []() {
      server.send(200, "text/plain", getDHT());
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
    delay(100);
  }
}

void jsonConfigFlush(){
  JsonObject &conf = jsonConfig.obj();
  // set default to json
  if ( !conf["numPoll"] ) {
    conf["numPoll"] = 10;
    jsonConfig.save();
  }
  if ( !conf["interval"] ) {
    conf["interval"] = 1000 * 60 * 15;
    jsonConfig.save();
  }
  // reflect conf to global variables
  unsigned long t = conf["interval"];
  CI.set(t);
  if ( conf["DebugOut"] ) {
    int t = conf["DebugOut"];
    DebugOut.setType(t);
  }
}

String getMessage() {
#ifdef GET_VCC_BY_ANALOG_READ  
  double a = analogRead(A0) / 1024.0;
#else
  double a = ESP.getVcc() / 1024.0;
#endif
  String d = getDHT();
  String s = d + ", " + String(a,2);
  return s;
}

void userFunc() {
  sendEspNowData(slaveMac, getMessage(), enDATA);
}

void reqReaction(int reqid) {
  JsonObject &conf = jsonConfig.obj();
  uint8_t type = espNowBuffer.getTypeFromReqBuffer(reqid);
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



