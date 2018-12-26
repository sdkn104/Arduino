//
// Example sketch for MyLib, MyOTA, MyCockpit, NTP
//  - connect WiFi in STA mode
//  - NTP network time sync
//  - run OTA server
//  - run cockpit web server

extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>

int id = 9; // device No.

CheckInterval CI(1000*5); // interval [ms]

ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open

int ledPin1 = 4;     //digital 4番ピン
int ledPin2 = 5;     //digital 5番ピン

int leadTime1 = 0;
int leadTime2 = 0;
int onTime1 = 100;
int onTime2 = 100;
int offTime1 = 100;
int offTime2 = 100;

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP......................");
  //DebugOut.setToFile();
  //Serial.setDebugOutput(true);
  //WiFi.printDiag(DebugOut);

  jsonConfig.load();
  //jsonConfig.setFlush(jsonConfigFlush);
  //jsonConfig.flush();
  JsonObject &conf = jsonConfig.obj();
      
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin1, LOW); // low -> light on
  digitalWrite(ledPin2, LOW); // 

  
  //wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
  WiFiConnect();
  printSystemInfo();

  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  setupMyOTA();
  SET_THIS_SKETCH();
  addMyCockpit("/interval", 1, [](){
    String n = server.arg(0);
    CI.set(n.toInt());
    server.send(200, "text/plain", n + ", ok");
  });
  addMyCockpit("/onTime1", 1, [](){
    String n = server.arg(0);
    onTime1 = n.toInt();
    server.send(200, "text/plain", n + ", ok");
  });
  addMyCockpit("/onTime2", 1, [](){
    String n = server.arg(0);
    onTime2 = n.toInt();
    server.send(200, "text/plain", n + ", ok");
  });
  addMyCockpit("/offTime1", 1, [](){
    String n = server.arg(0);
    offTime1 = n.toInt();
    server.send(200, "text/plain", n + ", ok");
  });
  addMyCockpit("/offTime2", 1, [](){
    String n = server.arg(0);
    offTime2 = n.toInt();
    server.send(200, "text/plain", n + ", ok");
  });
  addMyCockpit("/leadTime2", 1, [](){
    String n = server.arg(0);
    leadTime2 = n.toInt();
    server.send(200, "text/plain", n + ", ok");
  });
  setupMyCockpit();
}

void loop() {
  loopMyOTA();
  loopMyCockpit();

  static int cnt = 0;
  cnt++;
  
  if( (cnt - leadTime1)  % (onTime1 + offTime1) == 0 ) {
    digitalWrite(ledPin1, LOW); // low -> light on    
  }
  if( (cnt - leadTime1)  % (onTime1 + offTime1) == onTime1 ) {
    digitalWrite(ledPin1, HIGH); // low -> light on    
  }
  if( (cnt - leadTime2)  % (onTime2 + offTime2) == 0 ) {
    digitalWrite(ledPin2, LOW); // low -> light on    
  }
  if( (cnt - leadTime2)  % (onTime2 + offTime2) == onTime2 ) {
    digitalWrite(ledPin2, HIGH); // low -> light on    
  }
  delay(100);
}

