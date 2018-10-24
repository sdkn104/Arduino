//


extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>
#include <espnowLib.h>

CheckInterval CI(10000);
CheckInterval AliveInterval(3600*1000);

AliveCheck aliveCheck;

LogFile SerialCmdLog("serialLog.txt", 2000);

ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP......................");
  DebugOut.setToNull();
    
  // json config
  jsonConfig.load();
  jsonConfig.setFlush(jsonConfigFlush);
  jsonConfig.flush();
  JsonObject &conf = jsonConfig.obj();
  
  wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
  WiFiConnect();
  printSystemInfo();

  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  setupMyOTA();
  SET_THIS_SKETCH();
  addMyCockpit("/alive", 1, []() {
    String id = server.arg(0);
    aliveCheck.registerAlive(id.toInt());
    server.send(200, "text/plain", String("alive signal get from ID ")+id);
  });  
  addMyCockpit("/aliveCheck", 0, []() {
    aliveCheck.checkAlive();
    server.send(200, "text/plain", aliveCheck.log);
  });  
  addMyCockpit("/aliveCheckEach", 1, []() {
    String id = server.arg(0);
    aliveCheck.checkAlive(id.toInt());
    server.send(200, "text/plain", aliveCheck.log);
  });
  addMyCockpit("/setWakeup", 1, []() {
    String id = server.arg(0);
    String polng = jsonConfig.obj()["polng"];
    String wkup = jsonConfig.obj()["wakeup"];
    jsonConfig.obj()["wakeup"] = id.toInt();
    server.send(200, "text/plain", String("conf[polng]: ")+polng + "\r\nconf[wakeup]: " + wkup + "->"+id);
  });
  setupMyCockpit();

  aliveCheck.init();
}

void loop() {
  JsonObject &conf = jsonConfig.obj();
  loopMyOTA();
  loopMyCockpit();

  // receive message from server(espnow) and response
  if( Serial.available() ){
    // get data
    String rec = Serial.readStringUntil('\r');
    int macid = rec.substring(0,2).toInt();
    int type  = rec.substring(3,6).toInt();
    String msg  = rec.substring(7);
    // check, reply, and action
    // format: "macId(2):type(3):string"
    // format: "cmd(5):string"
    if( rec.substring(0,6) == "polng:" ) {
      // receive wakeup id
      String s = getDateTimeNow() + " " + rec.substring(6);
      conf["polng"] = s;
      // return wakeup id
      if( conf.containsKey("wakeup") && conf["wakeup"] != 0 ) {
        String s = conf["wakeup"];
        Serial.print("wakup:"+s+"\r");
        conf["wakeup"] = 0;
      } else {
        Serial.print("OK\r");        
      }
      Serial.flush();        
    } else if( rec.substring(2,3) != ":" || rec.substring(6,7) != ":" ) {
      Serial.print("NG\r");
      Serial.flush();
    } else if( macid > 0 && ( type == enDATA || type == 0x00) ) {
      Serial.print("OK\r");
      Serial.flush();
      String data = rec.substring(7,rec.length());
      uploadData(String("espnow")+(macid==5?5:3), data);
      // alive
      aliveCheck.registerAlive(macid);
    } else if( rec == "00:000:request time" ) {
      Serial.print(now());
      Serial.print("\r");
      Serial.flush();
    } else if( macid > 0 && type == enSWITCH ) {
      Serial.print("OK\r");
      Serial.flush();
      if( msg.endsWith(" 5") ) {
        HttpGet("http://192.168.1.109/IRremote?arg=0&arg=0"); // TV power on
      }
      fileAppend("/fromSwitch.txt",(rec+"\r\n").c_str());
    } else {
      Serial.print("NG\r");
      Serial.flush();
    }
    DebugOut.println(getDateTimeNow()+" received("+rec+")");
    SerialCmdLog.println(getDateTimeNow()+" received("+rec+")");
  }

  if( CI.check() ) {
    DebugOut.println(getDateTimeNow());
  }
  if( AliveInterval.check() ) {
    aliveCheck.checkAlive();
  }
  delay(10);
}

void jsonConfigFlush(){
  JsonObject &conf = jsonConfig.obj();
  // set default value
  if ( !conf.containsKey("interval") ) {
    conf["interval"] = 1000 * 60 * 5;
    jsonConfig.save();
  }
  if ( !conf.containsKey("aliveCheckInterval") ) {
    conf["aliveCheckInterval"] = 1000 * 60 * 60;
    jsonConfig.save();
  }
  if ( !conf.containsKey("polng") ) {
    conf["polng"] = "";
    jsonConfig.save();
  }
  if ( !conf.containsKey("wakeup") ) {
    conf["wakeup"] = 0;
    jsonConfig.save();
  }
  // reflect conf to global variables
  if ( conf.containsKey("DebugOut") ) {
    int t = conf["DebugOut"];
    DebugOut.setType(t);
  }
  unsigned long t = conf["interval"];
  CI.set(t);
  t = conf["aliveCheckInterval"];
  AliveInterval.set(t);
}

void uploadData(String key, String data) {
  String ln = data;
  int code = triggerBigQuery(key+"_rcv", getDateTimeNow(), key, getDateTimeNow(), ln);
  if( code >= 300 ) {
    triggerIFTTT(key, getDateTimeNow(), ln, "");
  }
  //String url = String("http://orangepione.sada.org/cgi-bin/storelog.py?v=")+key+","+getDateTimeNow()+","+ln;
  //String resp = HttpGet(url.c_str());
  //DebugOut.println("trigger orangepi: response="+resp);
  time_t ut = makeTime(ln.substring(17, 19).toInt(), ln.substring(14, 16).toInt(), ln.substring(11, 13).toInt(), ln.substring(8, 10).toInt(), ln.substring(5, 7).toInt() - 1, ln.substring(0, 4).toInt())
              - 60 * 60 * 9;
  if ( ln.substring(0, 1) == "2" ) {
    //String url = String("http://orangepione.sada.org/cgi-bin/storelog.py?v=")+URLEncode(key+","+getDateTimeNow()+","+ln.substring(51, 56)+","+ln.substring(35, 40)+","+ln.substring(ln.length() - 5));
    //String resp = HttpGet(url.c_str());
    //DebugOut.println("trigger orangepi: response="+resp);
    //triggerUbidots(key, "{\"temperature\":{\"value\": " + ln.substring(51, 56) + ", \"timestamp\":" + ut + "000}}");
    //triggerUbidots(key, "{\"humidity\":{\"value\": " + ln.substring(35, 40) + ", \"timestamp\":" + ut + "000}}");
    //triggerUbidots(key, "{\"voltage\":{\"value\": " + ln.substring(ln.length() - 5) + ", \"timestamp\":" + ut + "000}}");
    String iso = getDateTimeISOUTC(now());
    triggerM2X(key, "temperature", "{\"value\": " + ln.substring(51, 56) + ", \"timestamp\":\"" + iso + "\"}");
    triggerM2X(key, "humidity", "{\"value\": " + ln.substring(35, 40) + ", \"timestamp\":\"" + iso + "\"}");
    triggerM2X(key, "voltage", "{\"value\": " + ln.substring(ln.length() - 5) + ", \"timestamp\":\"" + iso + "\"}");
  }
}

