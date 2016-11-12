#include <ESP8266WiFi.h>

extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <FS.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>
#include <Statistic.h>


const int beepPin = 5;
const int sensorPin = 4;
const int dhtPin = 13; // DHT11

//--- DHT -----------------------------------
#include "DHT.h"
#define DHTPIN dhtPin     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);
//-------------------------------------------

CheckInterval CI(3000);
CheckInterval CIdht(1000*60*5);
CheckInterval CIup(1000*60*30);
//CheckInterval CI(3000);
//CheckInterval CIdht(1000*6);
//CheckInterval CIup(1000*10);

// control parameters
float beepHz = 5000.0;
bool  enable = 1;
bool  IFTTTenable = 0;


ADC_MODE(ADC_VCC);

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start.............................................");

  // *** setting debug output *********
  DebugOut.setToFile("/catlog.txt");
  //DebugOut.setToSerial();
  DebugOut.println("start ESP");

  delay(500);
  pinMode(beepPin, OUTPUT);

  // BEEP
  beep(beepPin, 5000,500);
  delay(500);
  beep(beepPin, 5000,500);

  wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
  WiFiConnect();
  printSystemInfo();

  // BEEP
  beep(beepPin, 1000,500);
  delay(500);
  beep(beepPin, 1000,500);

  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  setSyncInterval(60*60*12);
  setupMyOTA();
  addMyCockpit("/beep1", [](){
    String out = String("beep1: 5000 Hz\r\n");
    beepHz = 5000.0;
    server.send(200, "text/plain", out);
  });
  addMyCockpit("/beep2", [](){
    String out = String("beep2: 800 Hz\r\n");
    beepHz = 800.0;
    server.send(200, "text/plain", out);
  });
  addMyCockpit("/beep3", [](){
    String out = String("beep3: 200 Hz\r\n");
    beepHz = 200.0;
    server.send(200, "text/plain", out);
  });
  addMyCockpit("/beep0", [](){
    String out = String("beep0: OFF\r\n");
    beepHz = -1.0;
    server.send(200, "text/plain", out);
  });
  addMyCockpit("/enable", [](){
    enable = !enable;
    String out = String("enable: ") + enable;
    server.send(200, "text/plain", out);
  });
  addMyCockpit("/IFTTTenable", [](){
    IFTTTenable = !IFTTTenable;
    String out = String("IFTTTenable: ") + IFTTTenable;
    server.send(200, "text/plain", out);
  });
  setupMyCockpit();
  SPIFFS.begin();
  dht.begin();
}

void loop() {
  loopMyOTA();
  loopMyCockpit();

  // PIR sensor
  if( enable && CI.check() ) { 
    static int prev = LOW;
    int cur = digitalRead(sensorPin); // high active 
    String dt = getDateTimeNow();
    //DebugOut.println(dt + " PIR:" + prev + " -> " + cur);
    if( cur==HIGH && beepHz > 0.0 ) {
      beep(beepPin, beepHz, 200);
      beep(beepPin, 15000, 500);
      beep(beepPin, beepHz, 200);
    }
    if( prev == LOW && cur == HIGH ) {
      DebugOut.println(dt + " PIR:" + prev + " -> " + cur);
      if(IFTTTenable) triggerIFTTT("PIR_Sensor",dt,"turnOn","0");
    }
    if( prev == HIGH && cur == LOW ) {
      DebugOut.println(dt + " PIR:" + prev + " -> " + cur);
      if(IFTTTenable) triggerIFTTT("PIR_Sensor",dt,"turnOff","0");
    }
    prev = cur;
  }

  // DHT sensor & Vcc monitor
  if( CIdht.check() ){
    String dt = getDateTimeNow();
    DebugOut.print(dt+" ");
    handleDHT();

    Statistic s(0.0, 0.01);
    for(int i=0; i<20; i++){
      s.add(ESP.getVcc()/1024.0);
      delay(5);
    }
    DebugOut.print(dt+" Vcc: ");
    DebugOut.println(s.summary());
  }
  
  // copy log to ftp site
  if( CIup.check() && SPIFFS.exists("/catlog.txt") ) {
    FTPClient ftp;
    ftp.open("192.168.1.8", "admin", "password");
    ftp.cd("disk1/share/sadakane/FTP/cat");
    ftp.pwd();
    ftp.ls();
    ftp.append("catlog.txt");
    ftp.bye();        
    SPIFFS.remove("/catlog.txt");
  }

  delay(1000); // for saving power. 
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

void triggerIFTTT(String event, String value1, String value2, String value3){
    WiFiClient client;
    if (client.connect("maker.ifttt.com", 80)) {
      DebugOut.println("connected to ifttt");
      String d = URLEncode(getDateTimeNow());
      String s = String("GET /trigger/")+URLEncode(event)+"/with/key/uoVHdqccPNfLG8UbUR6Al?value1=" + URLEncode(value1) + "&value2=" + URLEncode(value2) + "&value3=" + URLEncode(value3) + " HTTP/1.1\r\n";
      DebugOut.println(s);
      client.print(s);
      client.print("Host: maker.ifttt.com\r\n");
      client.print("Connection: close\r\n");     
      client.print("Accept: */*\r\n");   
      client.print("User-Agent: Mozilla/4.0 (compatible; esp8266 Lua; Windows NT 5.1)\r\n");   
      client.print("\r\n");
      DebugOut.println("Request has sent to ifttt");
    } 
}

void handleDHT(){
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
  
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) ) {
      DebugOut.println("Failed to read from DHT sensor!");
      return;
    }
  
    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);
  
    DebugOut.print("Humidity: ");
    DebugOut.print(h);
    DebugOut.print(" %,  ");
    DebugOut.print("Temp.: ");
    DebugOut.print(t);
    DebugOut.print(" *C, ");
    DebugOut.print("Heat index: ");
    DebugOut.print(hic);
    DebugOut.println(" *C ");
}

