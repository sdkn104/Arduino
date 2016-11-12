#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>

const char* ssid = "aterm-043262-g";
const char* password = "6f40290eb9eb8"; // wifi password

CheckInterval CI(5000);
CheckInterval CIled(20);
CheckInterval CIddns(1000*60*60*24*2);

ADC_MODE(ADC_VCC);

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP");

  delay(500);
  pinMode(2, OUTPUT);
  
  WiFiConnect(ssid, password);
  printSystemInfo();

  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  setupMyOTA();
  setupMyCockpit();
}

void loop() {
  loopMyOTA();
  loopMyCockpit();

  if( CI.check() ) { 
    static float hz = 10.0;
    hz = hz * 1.2;
    int us = 1000000.0/hz;
    Serial.println(String(getDateTimeNow()) + " us=" + us + " Hz=" + hz);
    beep(2, hz, 1000);
  }
  delay(1);
  
  if( 0 && CIled.check() ) { 
      static int tgl = 0;
      tgl++;
      if( digitalRead(0)==HIGH && tgl % 40 == 0 ) { 
        digitalWrite(2,LOW); // on
      } else {
        digitalWrite(2,HIGH); // off
      }
  }
}

void beep(int pin, float hz, int ms){
    int itv = ceil(1000000.0/hz/2.0); // [us]
    int cnt = ceil(ms*hz/1000.0);
    for(int i=0; i<cnt; i++) {
        digitalWrite(pin,LOW);
        delayMicroseconds(itv);
        delay(0);
        digitalWrite(pin,HIGH);      
        delayMicroseconds(itv);
        delay(0);
    }  
}

