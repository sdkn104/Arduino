// SPIFFS setup for ESP8266

// 1. set correct flash size in tools menu of IDE
// 2. upload this program
// 3. run this program

#include <FS.h>
#include <MyLib.h>

void setup() {
  Serial.begin(115200);
  DebugOut.setToSerial();
  DebugOut.println("ESP wait");
  for(int i=15; i>0; i--){
    DebugOut.println(String("waiting ... ") + i + "/15");
    delay(1000);
  }
  DebugOut.println(getSystemInfo());
  DebugOut.println("start SPIFFS format. wait a minutes...");
  ESP.wdtDisable(); // to avoid wdt reset during format() 
  if( SPIFFS.format() )   DebugOut.println("SPIFFS format end successfully");
  else                    DebugOut.println("SPIFFS format failed");
  ESP.wdtEnable(0);

  DebugOut.println("SPIFFS.begin()");
  if( SPIFFS.begin() ) {
    DebugOut.println("SPIFFS.begin() end successfully");
    DebugOut.println(getFSInfo());  
  }  else {
    DebugOut.println("SPIFFS.begin() failed");
  }

  DebugOut.println("Start loop");
}
 
void loop() {
  DebugOut.println("loop");
  delay(3000);
}

