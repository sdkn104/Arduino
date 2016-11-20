#ifndef _espnowLib_h
#define _espnowLib_h

#include <ESP8266WiFi.h>

extern "C" {
  #include <espnow.h>
}

// packet type
enum EspNowType {
  enPOLL   = 0x1,
  enWAKEUP = 0x2,
  enDATA   = 0x80,
  enTIME   = 0x81,
};

// buffer size of EspNowBufferClass
const int EspNowBufferSize = 5;

// buffer class
class EspNowBufferClass {
 public:
  String   log = "";
 private:
  // recv data
  int recvDataNum = 0;
  struct {
    uint8_t  mac[6];
    uint8_t  data[250]; // 250 = max bytes in payload
    int8_t  espNo; // 
    uint8_t  len; // data length
  } recvData[EspNowBufferSize];
  // recv req
  int recvReqNum = 0;
  struct {
    uint8_t  mac[6];
    uint8_t  data[4];
    int8_t  espNo;
  } recvReq[EspNowBufferSize];
  // recv ack
  int recvAckNum = 0;
  struct {
    uint8_t  mac[6];
    uint8_t  data[4];
    int8_t  espNo;
  } recvAck[EspNowBufferSize];
 public:
  bool store(uint8_t *mac, uint8_t *data, uint8_t len);
  void clearDataBuffer() {recvAckNum = 0;};
  void clearReqBuffer()  {recvReqNum = 0;};
  void clearAckBuffer()  {recvDataNum = 0;};
  bool recvAckExists(uint8_t *mac, uint8_t type);
  int  recvDataBufferMax() { return recvDataNum < EspNowBufferSize ? recvDataNum : EspNowBufferSize; };
  int  recvReqBufferMax()  { return recvReqNum  < EspNowBufferSize ? recvReqNum  : EspNowBufferSize; };
  String  getDataFromDataBuffer(int i);
  uint8_t getTypeFromDataBuffer(int i) {return recvData[i].data[3];};
  uint8_t getTypeFromReqBuffer(int i)  {return recvReq[i].data[3];};
  uint8_t getTypeFromAckBuffer(int i)  {return recvAck[i].data[3];};
  uint8_t *getMacFromDataBuffer(int i)  {return recvData[i].mac;};
  uint8_t *getMacFromReqBuffer(int i)   {return recvReq[i].mac;};
  uint8_t *getMacFromAckBuffer(int i)   {return recvAck[i].mac;};
  void processAllReq(void (*reqReaction)(int));
};

extern EspNowBufferClass espNowBuffer;

// call backs
//void default_send_cb(uint8_t* macaddr, uint8_t status);
//void default_recv_cb(uint8_t *macaddr, uint8_t *data, uint8_t len);

// setup
void setupEspNow(uint8_t *mac, void (*send_cb)(uint8_t *, uint8_t),
                               void (*recv_cb)(uint8_t *, uint8_t *, uint8_t));

// send packet
bool sendEspNowReq(uint8_t *macaddr, uint8_t type);
bool sendEspNowData(uint8_t *macaddr, String message, uint8_t type);
bool sendEspNow(uint8_t *macaddr, uint8_t *data, int len);

// misc
String sprintEspNowData(uint8_t *data, int len);

#endif // _espnowLib_h
