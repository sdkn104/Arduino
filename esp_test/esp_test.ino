#include "FS.h"

#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
 
const char* ssid = "aterm-043262-g";
const char* password = "6f40290eb9eb8"; // wifi password

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP");

  Serial.print("connecting to WiFi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected.");
  Serial.print("local IP: ");
  Serial.println(WiFi.localIP());


  // NTP
  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  // NTPサーバを変更 (デフォルト: ntp.nict.jp)
  //setTimeServer("s2csntp.miz.nao.ac.jp");
  // NTP同期間隔を変更 (デフォルト: 300秒)
  //setSyncInterval(10);

  printSystemInfo();
  setupMyOTA();
  SPIFFS.begin();
  //SPIFFS.end();
}


void loop() {
  display();
  delay(4000);
  loopMyOTA();
}


void display(){

  time_t n = now();
  time_t t;

  char s[20];
  const char* format = "%04d-%02d-%02d %02d:%02d:%02d";

  // JST
  t = localtime(n, 9);
  sprintf(s, format, year(t), month(t), day(t), hour(t), minute(t), second(t));
  Serial.print("JST : ");
  Serial.println(s);

  // UTC
  t = localtime(n, 0);
  sprintf(s, format, year(t), month(t), day(t), hour(t), minute(t), second(t));
  //Serial.print("UTC : ");
  //Serial.println(s);
}

