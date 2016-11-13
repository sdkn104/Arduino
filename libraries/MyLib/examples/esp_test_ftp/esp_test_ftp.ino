#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>



const char* ssid = "aterm-043262-g";
const char* password = "6f40290eb9eb8"; // wifi password

CheckInterval CI(5000);
CheckInterval CIcsv(1000*60*5);
CheckInterval CIddns(1000*60*60*24*5);

//ADC_MODE(ADC_VCC);

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP");

//  for(int j=0; j<15; j++){
//  for(int i=0; i<20; i++){
//    float sdev, min, max;
////    float val = analogRead(A0);
//    float val = ESP.getVcc();
//    Serial.println(val);
//    delay(2);
//  }
//  delay(5000);
//  }
  
  WiFiConnect();

  printSystemInfo();

  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  setupMyOTA();
  setupMyCockpit();
  //updateDDNS();
}

void loop() {
  loopMyOTA();
  loopMyCockpit();
  if( CIddns.check() ){
    updateDDNS();
  }
  if( CI.check() ) { 
    Serial.println(".");
  }

  byte inChar = Serial.read();
  if(inChar == 'f') {
    FTPClient ftp;
    Serial.println(ftp.open("192.168.1.8", "admin", "password"));
    Serial.println(ftp.cd("disk1/share/sadakane/FTP"));
    Serial.println(ftp.pwd());
    Serial.println(ftp.ls());
    Serial.println(ftp.append("log.txt"));
    //Serial.println(ftp.ls());
    Serial.println(ftp.bye());
  }
}




