
// DHT temp/humid sensor logger with ESPNOW or RF24 or HTTP
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

// select ESPNOW or RF24
//#define ENABLE_ESPNOW  // enable ESPNOW communication, else use RF24

// device ID (macId)
int deviceId;  // for auto detected device ID

//--- DHT -----------------------------------
#include "DHT.h"
#define DHTPIN 4     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT22, AM2320
DHT dht(DHTPIN, DHTTYPE);
//-------------------------------------------

/****************** RF24 ***************************/
#include "RF24.h"
RF24 radio(1, 5); // (CEpin, CSpin)
byte addressSVR[6] = "svr01";
byte addressDEV[6] = "dev01";
struct RF24Payload {
  uint8_t cmd;
  uint8_t deviceId;
  float temperature;
  float hummidity;
};
/***************************************************/

CheckInterval CI; // for STA mode

// use analogRead() instead of getVcc().
//   - use for the device that analog IO pin connected to VDD
//#define GET_VCC_BY_ANALOG_READ

#ifdef GET_VCC_BY_ANALOG_READ
#else
ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open
#endif

// ---- for ESPNOW ----------------------------------
uint8_t *slaveMac = macAddrAP[8];  // espnow slave AP mac address
// --------------------------------------------------

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

  deviceId = getDeviceId(); // auto detect deviceID

  // set STA mode when startup by power-on or external reset
  struct rst_info *rstInfo = ESP.getResetInfoPtr();
  if ( rstInfo->reason == REASON_DEFAULT_RST /* startup by power on */ || rstInfo->reason == REASON_EXT_SYS_RST /* external reset */) {
    conf["mode"] = "STA";
  }

  if ( conf["mode"] == String("EspNow") || conf["mode"] == String("EspNowDSleep") ) {
    // EspNow/RF24 mode setup (No WiFi)
    espMode = 1;
#ifdef ENABLE_ESPNOW
    WiFi.mode(WIFI_STA);
    setupEspNow(NULL, NULL, NULL);
#else
    WiFi.mode(WIFI_OFF);
#endif
  } else {
    // STA mode setup
    espMode = 0;
    wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
    WiFiConnect(deviceId);
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
      String s = getDHT(NULL, NULL);
      s = s + "\r\nRadio is connected? -> " + (radio.isPVariant() + 0);
      RF24Payload payload;
      payload.cmd = 0x08; // test
      payload.deviceId = deviceId;
      s = s + "\r\nSend test packet -> " + (radio.write( &payload, sizeof(payload) ) + 0);

      server.send(200, "text/plain", s);
    });
    setupMyCockpit();
  }

  setupRF24(addressDEV, addressSVR);
  delay(0);
}

void setupRF24(byte * addressRX, byte * addressTX) {
  // powerOn and get into Standby-I mode
  bool r = radio.begin();
  DebugOut.println(String("radio.begin ") + (r + 0));
  if ( !r ) return;

  // Set Power Amplifier (PA) level to one of four levels: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX(default)
  // The power levels correspond to the following output levels respectively: NRF24L01: -18dBm, -12dBm,-6dBM, and 0dBm
  //radio.setPALevel(RF24_PA_LOW);
  // set retry (setting big value might cause WDT time out ??)
  radio.setRetries(15, 3); // (delay, count) = (retry interval in 250us, max 15,  max retry count, max 15)
  radio.setAutoAck(true);
  radio.setDataRate( RF24_1MBPS ); // 1Mbps is most reliable (default)

  // Open a writing and reading pipe
  radio.openWritingPipe(addressTX);
  radio.openReadingPipe(1, addressRX);
}


void loop() {
  JsonObject &conf = jsonConfig.obj();

  if ( espMode == 1 ) { // ESPNOW/RF24 (non-WiFi) mode
#ifdef ENABLE_ESPNOW
    loopEspnowController(userFunc, reqReaction, slaveMac);
#else
    loopEspnowController(userFunc, NULL, NULL);
#endif
  } else { // STA mode
    // change mode
    if ( conf["mode"] == String("EspNow") || conf["mode"] == String("EspNowDSleep") ) {
      DebugOut.println("switch to esp-now flow...");
      SPIFFS.end(); ESP.restart();
    }

    loopMyOTA();
    loopMyCockpit();

    if ( CI.check() ) {
      // upload data
      String s = getMessage();
      String data = getDateTimeNow() + ", " + s;
      int macId = deviceId; //getIdOfMacAddrSTA(WiFi.macAddress());
      uploadData(macId2DeviceName(macId), data);
      // log
      DebugOut.println(s);
      DebugOut.println(getDateTimeNow() + " VCC: " + ESP.getVcc() / 1024.0);
    }
    delay(100);
  }
}

void jsonConfigFlush() {
  JsonObject &conf = jsonConfig.obj();
  // set default to json
  if ( !conf["numPoll"] ) {
    conf["numPoll"] = 1;
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
  String d = getDHT(NULL, NULL);
  String s = d + ", " + String(a, 2);
  return s;
}

void userFunc() {
#ifdef ENABLE_ESPNOW
  sendEspNowData(slaveMac, getMessage(), enDATA);
#else
  float t, h;
  String s = getDHT(&t, &h);
  sendRF24(t, h);
#endif
}

#ifdef ENABLE_ESPNOW
void reqReaction(int reqid) {
  JsonObject &conf = jsonConfig.obj();
  uint8_t type = espNowBuffer.getTypeFromReqBuffer(reqid);
  DebugOut.println(type);
  if ( type == 2 ) { // wakeup req
    DebugOut.println("get wakeup packet. exit esp-now mode...");
    conf["mode"] = "STA";
  }
}
#endif

// get sensor values
String getDHT(float * temp, float * humm) {
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

  if ( temp != NULL ) *temp = t;
  if ( humm != NULL ) *humm = h;
  if ( isnan(h) || isnan(t) ) {
    return "Failed to read from DHT sensor!";
  }

  // Compute heat index in Celsius
  float hi = dht.computeHeatIndex(t, h, false);

  return String("Humidity: ") + h + " %  " + "Temp.: " + t + " *C " +
         "Heat index: " + hi;
}


void uploadData(String key, String data) {
  String ln = data;
  int code = triggerBigQuery(key + "_rcv", getDateTimeNow(), key, getDateTimeNow(), ln);
  if ( code >= 300 ) {
    triggerIFTTT(key, getDateTimeNow(), ln, "");
  }
}

// send data by RF24, receive wakeup packet, change mode if wakeup packet is for me
void sendRF24(float temp, float humm) {
  DebugOut.println("start sendRF24");
  if ( !radio.isPVariant() ) return; // RF24 module not connected
  // send data
  RF24Payload payload;
  payload.cmd = 0x01;
  payload.deviceId = deviceId;
  payload.temperature = temp;
  payload.hummidity = humm;
  String log = getDateTimeNow() + " Now sending in 0 micros\r\n";
  unsigned long t = micros();
  if (!radio.write( &payload, sizeof(payload) )) { // fail send
    log = log + "failed in " + ((int)(micros() - t)) + " micros\r\n";
  } else { // success send
    log = log + "sent in " + ((int)(micros() - t)) + " micros\r\n";
    // wait wakeup packet
    radio.startListening(); // get into RX mode
    log = log + "start listening in " + ((int)(micros() - t)) + " micros\r\n";
    for (int i = 0; i < 100 && !radio.available(); i++) {
      delay(1);
    }
    if ( radio.available() ) { // packet come
      log = log + "start reading RX FIFO in " + ((int)(micros() - t)) + " micros\r\n";
      // read wait packet
      while (radio.available()) {
        radio.read( &payload, sizeof(payload) );
      }
      String rec = String(payload.cmd) + "," + payload.deviceId + "," + payload.hummidity + "," + payload.temperature;
      log = log + "received " + rec + " in " + ((int)(micros() - t)) + " micros\r\n";
      if ( payload.cmd == 0x0F && payload.deviceId == deviceId ) {
        JsonObject &conf = jsonConfig.obj();
        conf["mode"] = "STA";
      }
    } else {
      log = log + "received none in " + ((int)(micros() - t)) + " micros\r\n";
    }
    delay(0); // necesary. if not stopListening() would abort.
    radio.stopListening(); // get into Standby-I mode
    delay(0); // necesary. if not WDT would timed out
    log = log + "stop listening in " + ((int)(micros() - t)) + " micros\r\n";
  }
  DebugOut.print(log);
}

