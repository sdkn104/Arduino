# espnowLib

This libaray is intent for easy-to use of [ESP-NOW function](https://espressif.com/en/products/software/esp-now/overview) of ESP8266 in [Arduino-ESP8266 environment](https://github.com/esp8266/Arduino).

This library implement the followings:
* Req-Ack protocol
* Buffer for received ESP-Now packets
* packet header format which define req/ack/data packet type

## Quick Start

### Example sketch for controller node (device):
It sends message string to the slave node.
```Arduino
#include <ESP8266WiFi.h>
#include <espnowLib.h>

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  setupEspNow(NULL, NULL, NULL); // setup esp-now
}

void loop() {
  uint8_t slaveMac[] = {0x1A,0xFE,0x34,0xD5,0xC7,0xAF}; // mac address of slave
  sendEspNowData(slaveMac, "message to be sent", 0); // send message to the slave, wait for ack packet
  delay(10000);
}
```

### Example sketch for slave node (server):
It receives message string from the controller node.
```Arduino
#include <ESP8266WiFi.h>
#include <espnowLib.h>

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("foobar", "12345678", 1, 0); // setup WiFi Access pont
  setupEspNow(NULL, NULL, NULL); // setup esp-now
}

void loop() {
  for (int i = 0; i < espNowBuffer.recvDataBufferMax(); i++ ) { // for each data packet in buffer
    String data = espNowBuffer.getDataFromDataBuffer(i);
    Serial.println( data ); // action for received data packet
  }
  espNowBuffer.clearDataBuffer(); // clear data buffer

  delay(500);
}
```
### Example Description
ESP-Now is a protocol which enables low-power communication between ESP8266 nodes without WiFi connection.
sendEspNow() send a ESP-NOW packet that includes specified data(string) to the slave node, and wait for ack packet reply. When the slave node receives the packet, it return ack packet and store the recevied packet in buffer espNowBuffer in background.

## API Specification (Digest)

#### void setupEspNow(NULL, NULL, NULL)
This function initializes espnow and setup call back functions for espnow packet receive/send event.
The receive call back function automatically reply ack packet, and store received packet to the buffer espNowBuffer.
This function should be called in setup() for slave and controller node.

#### bool sendEspNowReq(uint8_t *macaddr, uint8_t type)
#### bool sendEspNowData(uint8_t *macaddr, String message, uint8_t type)

####   void espNowBuffer.clearDataBuffer()
####   void espNowBuffer.clearReqBuffer()
####   void espNowBuffer.clearAckBuffer()
####   bool espNowBuffer.recvAckExists(uint8_t *mac, uint8_t type)
####   int  espNowBuffer.recvDataBufferMax()
####   int  espNowBuffer.recvReqBufferMax()
####   String  espNowBuffer.getDataFromDataBuffer(int i)
####   uint8_t espNowBuffer.getTypeFromDataBuffer(int i)
####   uint8_t espNowBuffer.getTypeFromReqBuffer(int i)
####   uint8_t espNowBuffer.getTypeFromAckBuffer(int i)
####   uint8_t *espNowBuffer.getMacFromDataBuffer(int i)
####   uint8_t *espNowBuffer.getMacFromReqBuffer(int i)
####   uint8_t *espNowBuffer.getMacFromAckBuffer(int i)
