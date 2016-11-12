// This program include just basic/standard functions:
//   OTA, NTP, Cockpit
// use this for test

extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>

//--- DHT -----------------------------------
#include "DHT.h"

#define DHTPIN 4     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT22, AM2320

DHT dht(DHTPIN, DHTTYPE);
//-------------------------------------------

CheckInterval CI(1000*60*30);

//ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open

////LogFile tlog("/testlog.txt", 1000);

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP......................");
  DebugOut.setToFile();
  
  wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
  
  WiFiConnect();
  printSystemInfo();

  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  setupMyOTA();
  addHtmlMyCockpit(String("Sketch: ")+THIS_SKETCH+"<BR><BR>");
  addMyCockpit("/interval", 1, [](){
    String n = server.arg(0);
    CI.set(n.toInt());
    server.send(200, "text/plain", n + ", ok");
  });
  setupMyCockpit();
}

void loop() {
  loopMyOTA();
  loopMyCockpit();

  if( CI.check() ) { 
    String t = getDateTimeNow();
//    double v = ESP.getVcc()/1024.0;
    double a = analogRead(A0)/1024.0;
    String d = getDHT();
//    DebugOut.println(getDateTimeNow()+" VCC: "+v);
    DebugOut.println(t+" A0[V]: "+a);
    DebugOut.println(t+" T: "+d);
    //tlog.println(t + "sadakane");
    triggerIFTTT("basic",t,String(a),String(d));
  }

  delay(1000);
}

String getDHT(){
    String out = "";
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();  
    // Compute heat index in Celsius
    float hi = dht.computeHeatIndex(t, h, false);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) ) {
      return "Failed to read from DHT sensor!";
    }
  
    return String("Humidity: ") + h + " %  " + "Temp.: " + t + " *C " + 
          "Heat index: " + hi;
}

