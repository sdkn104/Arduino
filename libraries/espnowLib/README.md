# espnowLib

This libaray is intent for easy-to use of [ESP-NOW function](https://espressif.com/en/products/software/esp-now/overview) of ESP8266 in [Arduino-ESP8266 environment](https://github.com/esp8266/Arduino).

This library implement the followings:
* Req-Ack protocol
* Buffer for received ESP-Now packets

## Quick Start

Example sketch for controller

```Arduino
#include <ESP8266WiFi.h>
#include <espnowLib.h>

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  setupEspNow(NULL, NULL, NULL);
}

void loop() {
  uint8_t slaveMac[] = {0x1A,0xFE,0x34,0xD5,0xC7,0xAF}; // mac address of slave
  sendEspNow(slaveMac, "message to be sent", 0); // send message to the slave
  delay(10000);
}
```

Example sketch for slave

```Arduino
#include <ESP8266WiFi.h>
#include <espnowLib.h>

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("foobar", "12345678", 1, 0); // setup WiFi Access pont
  setupEspNow(NULL, NULL, NULL);
}

void loop() {
  for (int i = 0; i < espNowBuffer.recvDataBufferMax(); i++ ) { // for each data packet in buffer
    String data = espNowBuffer.getDataFromDataBuffer(i);
    Serial.println( data ); // action for received data packet
  }
  espNowBuffer.recvDataNum = 0; // clear data packet in buffer

  delay(500);
}
```
