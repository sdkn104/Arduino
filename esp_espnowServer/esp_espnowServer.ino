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
#include <MyCockpit.h>

CheckInterval CI(1000 * 5); // interval for DebugOut log

ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open

int espMode;

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP......................");
  //DebugOut.setToFile();
  Serial.setDebugOutput(false);
  //WiFi.printDiag(DebugOut);

  // json config
  if ( ESP_rtcUserMemoryRead().startsWith("{") && jsonConfig.loadRtcMem() ) {
    ESP_rtcUserMemoryWrite("");
  } else {
    jsonConfig.load();
  }
  JsonObject &conf = jsonConfig.obj();
  // set default value
  if ( !conf["recvDataBufSize"] ) {
    conf["recvDataBufSize"] = 20 * 40;
    jsonConfig.save();
  }

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
    DebugOut.println("setup for STA mode...");
    wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
    WiFiConnect();
    printSystemInfo();
    ntp_begin(2390);
  } else {
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
    WiFi.softAP("foobar", "12345678", 1, 0); // ssid, passwd, channel, hide ssid
    // default 192.168.4.1
    setupEspNow(NULL, NULL, NULL);
  }

  // common setup
  setupMyOTA();
  addHtmlMyCockpit(String("Sketch: ") + THIS_SKETCH + "<BR><BR>");
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
  DebugOut.println(getDateTimeNow() + ": setup end.");
}


void loop() {
  JsonObject &conf = jsonConfig.obj();

  if ( conf["test"] == 1 ) {
    conf["test"] = 0;
    jsonConfig.save();

    Serial.println(ESP_rtcUserMemoryRead());
    ESP_rtcUserMemoryWrite("aaabbbcccdddeee");
    Serial.println(ESP_rtcUserMemoryRead());
    delay(3000);
  }

  // post process for call backs
  DebugOut.print(espNowBuffer.log);
  espNowBuffer.log = "";

  if ( espMode == 1 ) { // ESP-Now mode
    // send wakeup
    if ( conf["wakeup"] > 0 ) {
      DebugOut.println(getDateTimeNow() + ": send wakeup req...");
      int id = conf["wakeup"];
      if ( id >= numMacAddr ) {
        DebugOut.println("Error: illegal mac address id specified");
        conf["wakeup"] = 0; // complete
      } else {
        uint8_t message[] = ESPNOW_REQ_WAKEUP; // wakeup req
        uint8_t *mac = macAddrSTA[id];
        if ( sendEspNow(mac, message, 4) ) { // success
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
      // get mac id
      int macId = espNowBuffer.recvData[i].espNo == -1 ? getIdOfMacAddrSTA(espNowBuffer.recvData[i].mac) : espNowBuffer.recvData[i].espNo;
      // store to file
      String file = String("/espNowRcvData") + macId + ".txt";
      fileAppend(file.c_str(), getDateTimeNow().c_str());
      fileAppend(file.c_str(), ", ");
      fileAppend(file.c_str(), espNowBuffer.getDataFromDataBuffer(i).c_str());
      fileAppend(file.c_str(), "\r\n");
    }
    espNowBuffer.recvDataNum = 0; // clear data packet in buffer

    // upload received data file
    for ( int id = 0; id < numMacAddr; id++) {
      String file = String("/espNowRcvData") + id + ".txt";
      long sz = SPIFFS.exists(file) ? fileSize(file.c_str()) : 0;
      if ( sz > conf["recvDataBufSize"] ) {
        DebugOut.println(getDateTimeNow() + ": " + file + " is big. its time to switch to upload data...");
        conf["connectNet"] = 1;
      }
    }

    // change mode
    if ( conf["connectNet"] == 1 || conf["mode"] != String("EspNow") ) {
      DebugOut.println(getDateTimeNow() + ": " + "switch to STA flow...");
      jsonConfig.save();
      SPIFFS.end(); ESP.restart();
    }

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
  }

  // common
  loopMyCockpit();
  delay(500);
}

// reaction for request
void reqReaction(int req) {
  JsonObject &conf = jsonConfig.obj();
  uint8_t type = espNowBuffer.recvReq[req].data[3];
  DebugOut.println(type);
  if ( type == 1 ) { // poll req
    DebugOut.println(getDateTimeNow() + ": " + "poll action...");
    if ( conf["wakeup"] < 0 ) { // pending exists
      int id = conf["wakeup"];
      uint8_t *mac = macAddrSTA[-id];
      if ( macAddress2String(mac) == macAddress2String(espNowBuffer.recvReq[req].mac) ) { // this is pending
        DebugOut.println("send wakeup req...");
        uint8_t message[] = ESPNOW_REQ_WAKEUP;
        if ( sendEspNow(espNowBuffer.recvReq[req].mac, message, 4) ) { // success
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
        triggerIFTTT(iftttid, getDateTimeNow(), ln, "");
        time_t ut = makeTime(ln.substring(17, 19).toInt(), ln.substring(14, 16).toInt(), ln.substring(11, 13).toInt(), ln.substring(8, 10).toInt(), ln.substring(5, 7).toInt() - 1, ln.substring(0, 4).toInt())
                    - 60 * 60 * 9;
        if ( ln.substring(0, 1) == "2" ) {
          triggerUbidots(iftttid, "{\"temperature\":{\"value\": " + ln.substring(51, 56) + ", \"timestamp\":" + ut + "000}}");
          triggerUbidots(iftttid, "{\"humidity\":{\"value\": " + ln.substring(35, 40) + ", \"timestamp\":" + ut + "000}}");
          triggerUbidots(iftttid, "{\"voltage\":{\"value\": " + ln.substring(ln.length() - 5) + ", \"timestamp\":" + ut + "000}}");
        }
      }
      fs.close();
      if ( toomany ) {
        triggerIFTTT(iftttid, getDateTimeNow(), "too many data to send.", "");
      }
      fileDelete(file.c_str());
    }
  }
}

