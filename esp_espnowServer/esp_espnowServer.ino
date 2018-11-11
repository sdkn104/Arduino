//
// ESPNOW server
//  - espnow slave mode, and WiFi STA connection mode
//  - get data from espnow controllers (esp_tempLogger,etc)
//  - upload data to cloud
//

extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <espnowLib.h>
#include <MyCockpit.h>

int id = 8; // device No.

CheckInterval CI(1000 * 10); // interval for DebugOut log
CheckInterval CItime(1000*60*60*24); // interval for clock adjust
CheckInterval CIpoll(1000*30); // interval for polling to serverSTA

ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open

int espMode;

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP......................");
  DebugOut.setToNull();
  Serial.setDebugOutput(false);
  //WiFi.printDiag(DebugOut);

  // json config
  jsonConfig.load();
  jsonConfig.setFlush(jsonConfigFlush);
  jsonConfig.flush();
  JsonObject &conf = jsonConfig.obj();

  // decide AP/STA mode
  if ( conf["connectNet"] == 1 ) {
    espMode = 0;
  } else if ( conf["wakeup"] > 0 ) {
    espMode = 1;
  } else if ( conf["mode"] == String("EspNow") || conf["mode"] == String("EspNowDSleep") ) {
    espMode = 1;
  } else {
    espMode = 0;
  }

  if ( espMode == 0 ) {
    setupForSTA();
  } else {
    setupForEspNow();
  }
  setupForCommon();
  DebugOut.println(getDateTimeNow() + ": setup end.");
}


void setupForSTA() {
  DebugOut.println("setup for STA mode...");
  wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
  WiFiConnect(id);
  printSystemInfo();
  ntp_begin(2390);
}

void setupForEspNow() {
  JsonObject &conf = jsonConfig.obj();
  DebugOut.println("seutp for espnow (AP) mode...");
  // restore time
  if ( conf["time"] ) {
    unsigned long t = conf["time"];
    setTime(t);
  }
  // setup
  WiFi.mode(WIFI_AP);
  //IPAddress local_IP(192,168,4,2); // IP address for AP I/F
  //IPAddress gateway(192,168,1,1);
  //IPAddress subnet(255,255,255,0);
  //DebugOut.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
  uint8_t mac[6];
  WiFi.softAPmacAddress(mac);
  int id = getIdOfMacAddrAP(mac);
  char ssid[100];
  sprintf(ssid, "foobar%02d", id);
  WiFi.softAP(ssid, "12345678", 1, 0); // ssid, passwd, channel, hide ssid
  // default 192.168.4.1
  setupEspNow(NULL, NULL, NULL);
}


void setupForCommon() {
  setupMyOTA();
  SET_THIS_SKETCH();
  addMyCockpit("/conf", 0, []() {
    String s;
    jsonConfig.obj().prettyPrintTo(s);
    server.send(200, "text/plain", s);
  });
  addMyCockpit("/interval", 1, []() {
    String n = server.arg(0);
    CI.set(n.toInt());
    server.send(200, "text/plain", n + ", ok");
  });
  addMyCockpit("/recvDataBufSize", 1, []() {
    String n = server.arg(0);
    jsonConfig.obj()["recvDataBufSize"] = n.toInt();
    jsonConfig.save();
    server.send(200, "text/plain", n + ", ok");
  });
  addMyCockpit("/espnowSerial", 1, []() {
    String no = server.arg(0);
    jsonConfig.obj()["espnowSerial"] = no.toInt();
    jsonConfig.save();
    server.send(200, "text/plain", "ok");
  });
  addMyCockpit("/toEspNow", 0, []() {
    jsonConfig.obj()["mode"] = "EspNow";
    jsonConfig.save();
    server.send(200, "text/plain", "ok");
  });
  addMyCockpit("/fromEspNow", 0, []() {
    jsonConfig.obj()["mode"] = "STA";
    jsonConfig.save();
    server.send(200, "text/plain", "ok");
  });
  addMyCockpit("/connectNet", 0, []() {
    jsonConfig.obj()["connectNet"] = 1;
    jsonConfig.save();
    server.send(200, "text/plain", "ok");
  });
  addMyCockpit("/wakeupController", 1, []() {
    String no = server.arg(0);
    jsonConfig.obj()["wakeup"] = no.toInt();
    jsonConfig.save();
    server.send(200, "text/plain", "ok");
  });
  setupMyCockpit();
}

void jsonConfigFlush(){
  JsonObject &conf = jsonConfig.obj();
  // set default value
  if ( !conf.containsKey("recvDataBufSize") ) {
    conf["recvDataBufSize"] = 20 * 40;
    jsonConfig.save();
  }
  if ( !conf.containsKey("interval") ) {
    conf["interval"] = 1000 * 60 * 5;
    jsonConfig.save();
  }
  if ( !conf.containsKey("espnowSerial") ) {
    conf["espnowSerial"] = 1;
    jsonConfig.save();
  }
  // reflect conf to global variables
  if ( conf.containsKey("DebugOut") ) {
    int t = conf["DebugOut"];
    DebugOut.setType(t);
  }
  unsigned long t = conf["interval"];
  CI.set(t);
}


void loop() {
  JsonObject &conf = jsonConfig.obj();

  // print espnow log
  DebugOut.print(espNowBuffer.log);
  espNowBuffer.log = "";

  if ( espMode == 1 ) { // ESP-Now mode
    // send wakeup
    if ( conf["wakeup"] > 0 ) {
      DebugOut.println(getDateTimeNow() + ": send wakeup req...");
      int id = conf["wakeup"];
      if ( id >= numMacAddr || id < 0 ) {
        DebugOut.println("Error: illegal mac address id specified");
        conf["wakeup"] = 0; // complete
      } else if( macAddress2String(macAddrSTA[id]) == String(WiFi.macAddress()) ) {
        conf["wakeup"] = 0; // complete
        conf["mode"] = "STA";
        jsonConfig.save();
        DebugOut.println("waking up myself...");
      } else {
        uint8_t *mac = macAddrSTA[id];
        if ( sendEspNowReq(mac, enWAKEUP) ) { // success
          conf["wakeup"] = 0; // complete
        } else { // fail
          conf["wakeup"] = -id; // pending for polling // TODO: pending for only one device
        }
      }
    }

    // re-action for request
    espNowBuffer.processAllReq(reqReaction);

    // action for data reveiced
    for (int i = 0; i < espNowBuffer.recvDataBufferMax(); i++ ) { // for each data packet in buffer
      // get info of packet
      int type = espNowBuffer.getTypeFromDataBuffer(i);
      int macId = getIdOfMacAddrSTA(espNowBuffer.getMacFromDataBuffer(i));
      String out = getDateTimeNow() + ", " + espNowBuffer.getDataFromDataBuffer(i);
      // send to Serial
      String id = macId < 10 ? String("0") + String(macId) : String(macId);
      char ty[4];
      sprintf(ty,"%03d",type);
      String rply = sendCommandToServerSTA(id + ":" + ty + ":" + out);
      DebugOut.println(getDateTimeNow() + " Serial: id(" + id + ") send(" + out + ") reply(" + rply + ")");
      // store to file, if fail to send to Serial
      if ( rply != "OK" ) {
        String file = String("/espNowRcvData") + macId + ".txt";
        fileAppend(file.c_str(), out.c_str());
        fileAppend(file.c_str(), "\r\n");
      }
    }
    espNowBuffer.clearDataBuffer();

    // upload received data file, if the file is big
    for ( int id = 0; id < numMacAddr; id++) {
      String file = String("/espNowRcvData") + id + ".txt";
      long sz = SPIFFS.exists(file) ? fileSize(file.c_str()) : 0;
      if ( sz > conf["recvDataBufSize"] ) {
        DebugOut.println(getDateTimeNow() + ": " + file + " is big. its time to switch to upload data...");
        conf["connectNet"] = 1;
      }
    }

    // update clock
    if( CItime.check() ) {
        String rply = sendCommandToServerSTA("00:000:request time"); // send request
        unsigned long tm = rply.toInt();
        if( tm > 0 && String(tm) == rply ) { // check format
          setTime(tm);
          DebugOut.println("clock adjusted");
        }
        DebugOut.println(getDateTimeNow() + " Serial: reply(" + rply + ")");
    }

    // polling
    if( CIpoll.check() ) {
        String w = conf["wakeup"];
        String msg = String("polng:svr_wakeup=") + w;
        String rply = sendCommandToServerSTA(msg); // send request
        if( rply.substring(0,6) == "wakup:" ) {
          conf["wakeup"] = rply.substring(6).toInt();
        }
        DebugOut.println(getDateTimeNow() + " Serial: reply(" + rply + ")");
    }

    // change mode
    if ( conf["connectNet"] == 1 || conf["mode"] != String("EspNow") ) {
      DebugOut.println(getDateTimeNow() + ": " + "switch to STA flow...");
      jsonConfig.save();
      SPIFFS.end(); ESP.restart();
    }
    loopMyCockpit();
    delay(100);

  } else { // STA mode
    // upload data
    if ( conf["connectNet"] == 1 ) { // one shot STA mode
      uploadRecvData();
      DebugOut.println(getDateTimeNow() + ": exit one-shot STA mode...");
      conf["connectNet"] = 0; // reset connectNet
      conf["time"] = now();      // store time
      jsonConfig.save();
      SPIFFS.end(); ESP.restart(); // for return to espnow mode
    }

    // change mode
    if ( conf["wakeup"] > 0 || conf["mode"] == String("EspNow") ) {
      DebugOut.println(getDateTimeNow() + ": switch to esp-now flow...");
      conf["time"] = now();      // store time
      jsonConfig.save();
      SPIFFS.end(); ESP.restart();
    }

    // put log
    if ( CI.check() ) {
      DebugOut.println("sta mode: " + getDateTimeNow() + " VCC: " + ESP.getVcc() / 1024.0);
    }

    loopMyOTA();
    loopMyCockpit();
    delay(50);
  }
}

// reaction for request
void reqReaction(int req) {
  JsonObject &conf = jsonConfig.obj();
  uint8_t type = espNowBuffer.getTypeFromReqBuffer(req);
  DebugOut.println(type);
  if ( type == enPOLL ) { // poll req
    DebugOut.println(getDateTimeNow() + ": " + "poll action...");
    if ( conf["wakeup"] < 0 ) { // pending exists
      int id = conf["wakeup"];
      uint8_t *mac = macAddrSTA[-id];
      if ( macAddress2String(mac) == macAddress2String(espNowBuffer.getMacFromReqBuffer(req)) ) { // this is pending
        DebugOut.println("send wakeup req...");
        if ( sendEspNowReq(espNowBuffer.getMacFromReqBuffer(req), enWAKEUP) ) { // success
          conf["wakeup"] = 0;
        }
      }
    }
  }
}

// upload recept data to network
bool uploadRecvData() {
  for ( int id = 0; id < numMacAddr; id++) {
    String file = String("/espNowRcvData") + id + ".txt";
    //long sz = SPIFFS.exists(file) ? fileSize(file.c_str()) : 0;
    //if ( sz > jsonConfig.obj()["recvDataBufSize"] ) {
    if ( SPIFFS.exists(file) ) {
      File fs = SPIFFS.open(file, "r");
      if ( !fs ) {
        DebugOut.println("Error: cannot open file");
        continue;
      }
      String iftttid = String("espnow") + id;
      bool toomany = false;
      for (int i = 0; fs.available(); i++ ) {
        if ( i > 50 ) {
          toomany = true;
          break;
        }
        String ln = fs.readStringUntil('\r');
        uploadData(iftttid, ln);
        fs.close();
        if ( toomany ) {
          triggerIFTTT(iftttid, getDateTimeNow(), "too many data to send.", "");
        }
        fileDelete(file.c_str());
      }
    }
  }
}


void uploadData(String key, String data) {
  String ln = data;
  triggerIFTTT(key, getDateTimeNow(), ln, "");
  time_t ut = makeTime(ln.substring(17, 19).toInt(), ln.substring(14, 16).toInt(), ln.substring(11, 13).toInt(), ln.substring(8, 10).toInt(), ln.substring(5, 7).toInt() - 1, ln.substring(0, 4).toInt())
              - 60 * 60 * 9;
  if ( ln.substring(0, 1) == "2" ) {
    triggerUbidots(key, "{\"temperature\":{\"value\": " + ln.substring(51, 56) + ", \"timestamp\":" + ut + "000}}");
    triggerUbidots(key, "{\"humidity\":{\"value\": " + ln.substring(35, 40) + ", \"timestamp\":" + ut + "000}}");
    triggerUbidots(key, "{\"voltage\":{\"value\": " + ln.substring(ln.length() - 5) + ", \"timestamp\":" + ut + "000}}");
  }
}


String sendCommandToServerSTA(String data) {
      if ( jsonConfig.obj()["espnowSerial"] == 1 ) {
        Serial.setTimeout(1000);
        while(Serial.available()) { byte c = Serial.read(); } // clear Serial buffer
        Serial.print(data + "\r");
        Serial.flush();
        delay(0);
        return Serial.readStringUntil('\r');
      }
      return "";
}

