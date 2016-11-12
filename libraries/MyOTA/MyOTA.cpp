#include <MyOTA.h>
// My usual setting of OTA

// Usage:
//   - #include <MyOTA.h>
//   - call once setupMyOTA() after WiFi connected in setup()
//   - call once loopMyOTA() in loop()

#define OTA_PORT_NO 8266 // Port defaults to 8266

void setupMyOTA() { 
  ArduinoOTA.setPort(OTA_PORT_NO);
  
  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    SPIFFS.end(); // flush file system
    Serial.println("Start OTA");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();  
  DebugOut.print("Start OTA. server port:");
  DebugOut.println(OTA_PORT_NO);
}

void loopMyOTA(){
  ArduinoOTA.handle();  
}


