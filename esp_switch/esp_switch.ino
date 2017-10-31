/*
 * ESPNOW Switch
 * 
 * When power-on, reset, or S/W restart,  starup with WiFi STA mode (maintainance mode).
 * To enter deep-sleep mode (normal mode), push startDSleep button on Custom Page.
 * 
 * Reaction of button push:
 *  - any button pushes -> send the number of pushes to ESPNOW slave.
 *  - push ten times -> restart ESP with STA mode
 */


extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>
#include <espnowLib.h>

// pin def
const int speaker_pin  = 13; // output
const int switch_pin   = 5; // input
const int rst_keep_pin = 4; // output
const int led_pin      = 2; // output on-board LED

// ESPNOW slave
uint8_t *slaveMac = macAddrAP[8];  // slave AP mac address

CheckInterval CI(1000 * 5); // interval [ms]

ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open


int beep_start = 0;
int beep_stop = 0;

//LogFile tlog("/testlog.txt", 1000);

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP......................");
  //DebugOut.setToFile();
  //Serial.setDebugOutput(true);
  //WiFi.printDiag(DebugOut);

  // pin setting
  pinMode(speaker_pin, OUTPUT); // digital out, or PWM out
  pinMode(rst_keep_pin, OUTPUT);
  pinMode(led_pin, OUTPUT);
  analogWrite(speaker_pin, 0); // stop PWM
  digitalWrite(rst_keep_pin, HIGH); // disable RST
  digitalWrite(led_pin, HIGH); // off

  // load json config
  jsonConfig.load();
  jsonConfigFlush();
  JsonObject &conf = jsonConfig.obj();

  // check startup mode
  struct rst_info *rstInfo = ESP.getResetInfoPtr();
  if ( rstInfo->reason == REASON_DEEP_SLEEP_AWAKE ) {
    conf["STAmode"] = 0;
  } else {
    conf["STAmode"] = 1; // TODO: no need to use conf, ok to use variable
  }
  
  // setup for STA mode
  if ( conf["STAmode"] == 1 ) {
    //digitalWrite(led_pin, LOW); // LOW : light-on
    wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
    WiFiConnect();
    printSystemInfo();

    ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
    setupMyOTA();
    SET_THIS_SKETCH();
    addMyCockpit("/startDSleep", 0, []() {
      jsonConfig.obj()["STAmode"] = 0;
      jsonConfig.saveRtcMem();
      server.send(200, "text/plain", " ok");
    });
    addMyCockpit("/interval", 1, []() {
      String n = server.arg(0);
      CI.set(n.toInt());
      server.send(200, "text/plain", n + ", ok");
    });
    addMyCockpit("/beepStart", 1, []() {
      String f = server.arg(0);
      beep_start = f.toInt();
      beep_stop = 0;
      server.send(200, "text/plain", f + ", ok");
    });
    addMyCockpit("/beepStop", 0, []() {
      beep_stop = 1;
      server.send(200, "text/plain", "ok");
    });
    addMyCockpit("/watch", 0, []() {
      int p0 = digitalRead(switch_pin);
      server.send(200, "text/plain", String(p0) + ", ok");
    });
    setupMyCockpit();

    // waiting to enter deep sleep (for trouble in loop() code)
    DebugOut.println("waiting to enter deep sleep mode");
    for (int i = 0; i < 30; i++) {
      loopMyOTA();
      loopMyCockpit();
      digitalWrite(led_pin, i % 2);
      delay(100);
    }
    digitalWrite(led_pin, HIGH); // off
  }

  // procedure for deep sleep mode
  if ( conf["STAmode"] == 0 ) {
    // wake up sound
    beep(speaker_pin, 500, 150);
    beep(speaker_pin, 800, 100);
    beep(speaker_pin, 1000, 300);

    // process
    switchProcess();
    
    // shut down sound
    beep(speaker_pin, 1000, 100);
    beep(speaker_pin, 800, 100);
    beep(speaker_pin, 500, 200);

    // enter to deep sleep
    SPIFFS.end();
    jsonConfig.saveRtcMem();
    ESP.deepSleep(0, WAKE_RF_DEFAULT); // no timer wakeup
    delay(1000);
  }
}

void loop() {
  JsonObject &conf = jsonConfig.obj();

  loopMyOTA();
  loopMyCockpit();

  // enter to deep sleep
  if ( conf["STAmode"] == 0 ) {
    SPIFFS.end();
    jsonConfig.saveRtcMem();
    ESP.deepSleep(0, WAKE_RF_DEFAULT); // no timer wakeup
    delay(1000);
  }

  // for test
  if ( beep_start ) {
    beepStart(speaker_pin, beep_start);
    beep_start = 0;
  }
  if ( beep_stop ) {
    beepStop(speaker_pin);
    beep_stop = 0;
  }

  // for debug
  if ( conf["STAmode"]==1 && CI.check() ) {
    DebugOut.println(getDateTimeNow() + " VCC: " + ESP.getVcc() / 1024.0);
    //beep(speaker_pin, 500.0, 1000);
  }

  delay(1);
}

void jsonConfigFlush() {
  JsonObject &conf = jsonConfig.obj();
  // set default to json
  if ( !conf["interval"] ) {
    conf["interval"] = 1000 * 5;
    jsonConfig.save();
  }
  if ( !conf["DebugOut"] ) {
    conf["DebugOut"] = DebugOut.getType();
  }
  // reflect conf to global variables
  unsigned long t = conf["interval"];
  CI.set(t);
  int d = conf["DebugOut"];
  DebugOut.setType(d);
}

void switchProcess(){  
  unsigned int tm;
  int waitTime = 2000;

  // wait for switch push
  time_t stime=millis(); 
  while( (tm=detectSwPush(0))==0 ) {
    if ( millis() > stime + waitTime ) break; // time out
    delay(10);
  }
  
  // record switch push pattern
  unsigned int swtime[20];
  int swcnt = 0;
  if ( tm != 0 ) {
    swtime[swcnt++] = tm;
    for(int i=0; i<300; i++){
      if( (tm=detectSwPush(0)) != 0 ){ 
        swtime[swcnt++] = tm;
        if( swcnt >= 20 ) break;
        i=0;
      }
      delay(10);
    }
  }

    
  // reaction for sw push
  if( swcnt > 0 ) {  // sw pushed
    // setup ESPNOW
    WiFi.mode(WIFI_STA);
    setupEspNow(NULL, NULL, NULL);

    // reproduce by sound
    //digitalWrite(led_pin, LOW); // on
    for(int i=0; i<swcnt; i++) {
      DebugOut.println(String("swtime[")+i+"] = "+swtime[i]);
      unsigned long t = i==0 ? 0 : swtime[i]-swtime[i-1];
      t = t > 200 ? t-200 : 0; // max(t-200,0)
      t = t > 100 ? t : 100; // max(t,100)
      delay(t);
      beep(speaker_pin, 1000, 200);
    }
    //digitalWrite(led_pin, HIGH); // off
    
    // reaction
    delay(500);
    sendEspNowData(slaveMac,String(swcnt),enSWITCH);
    if( swcnt == 10 ) {
      beep(speaker_pin, 800, 100);
      beep(speaker_pin, 1600, 100);
      beep(speaker_pin, 800, 100);
      beep(speaker_pin, 1600, 300);
      ESP_restart();
    }
  }
}

unsigned long detectSwPush(int pre_push_time) {
  int maxGap  = 10; // ms
  int minPush = 10; // ms

  if ( pre_push_time == 0 && digitalRead(switch_pin) == 1 ) return 0;

  unsigned long startTime = millis() - pre_push_time;
  beepStart(speaker_pin, 500.0);
  while (1) {
    for (int i = 0;  digitalRead(switch_pin) == 0; i++ ) {
      if ( i > 1000 * 10 ) {
        beepStop(speaker_pin);  // error: too long push
        return 0;
      }
      delay(1);
    }
    unsigned long gapStart = millis();
    if ( gapStart - startTime < minPush ) {
      beepStop(speaker_pin);  // error: too short push
      return 0;
    }
    for (int i = 0; digitalRead(switch_pin) == 1; i++ ) {
      if ( millis() - gapStart > maxGap ) {
        beepStop(speaker_pin);
        return startTime; // TODO: take care of time variable overflow
      }
      if ( i > 1000 * 10 ) {
        beepStop(speaker_pin);  // error: millis overflow
        return 0;
      }
      delay(1);
    }
  }
  beepStop(speaker_pin);
  return 0; // never
}


void beep(int pin, float hz, int ms) {
  beepStart(pin, hz);
  delay(ms);
  beepStop(pin);
}

void beepStart(int pin, float hz) {
  analogWriteFreq((int)hz);
  analogWrite(pin, 512); // half duty?
}

void beepStop(int pin) {
  analogWrite(pin, 0);
}



