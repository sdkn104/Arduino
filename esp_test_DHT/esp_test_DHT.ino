#include <ESP8266WiFi.h>

extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>

//--- DHT -----------------------------------
#include "DHT.h"

#define DHTPIN 5     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);
//-------------------------------------------

CheckInterval CI(4000);

ADC_MODE(ADC_VCC);

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start.............................................");
  
  DebugOut.println("start ESP");

  dht.begin();

  WiFiConnect();
  printSystemInfo();

  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  setupMyOTA();
  setupMyCockpit();
}

void loop() {
  loopMyOTA();
  loopMyCockpit();

  if( CI.check() ) {
    Serial.println(String(getDateTimeNow()));
    //delay(2000);
  
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);
  
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
  
    // Compute heat index in Fahrenheit (the default)
    float hif = dht.computeHeatIndex(f, h);
    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);
  
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %  ");
    Serial.print("Temp.: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F  ");
    Serial.print("Heat index: ");
    Serial.print(hic);
    Serial.print(" *C ");
    Serial.print(hif);
    Serial.println(" *F");
  }
}

