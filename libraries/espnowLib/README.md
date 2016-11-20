# espnowLib

This libaray is intent for easy-to use of [ESP-NOW function](https://espressif.com/en/products/software/esp-now/overview) of ESP8266 in [Arduino-ESP8266 environment](https://github.com/esp8266/Arduino).

This library implement the followings:
* Req-Ack protocol
* Buffer for received ESP-Now packets

## Quick Start

'''xx
extern "C" {
#include <user_interface.h> // for sleep mode
}
#include <ESP8266WiFi.h>

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("foobar", "12345678", 1, 0); // ssid, passwd, channel, hide ssid
  setupEspNow(NULL, NULL, NULL);
}

void loop() {
  espNowBuffer.processAllReq(reqReaction);

  // action for data reveiced
  for (int i = 0; i < espNowBuffer.recvDataBufferMax(); i++ ) { // for each data packet in buffer
    Serial.println( espNowBuffer.getDataFromDataBuffer(i) );
  }
  espNowBuffer.recvDataNum = 0; // clear data packet in buffer

  delay(500);
}
'''
