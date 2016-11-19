#ifndef _espnowLib_h
#define _espnowLib_h

//#ifdef ARDUINO_ARCH_ESP8266   // this macro is defined by Arduio IDE

#include <ESP8266WiFi.h>

extern "C" {
  #include <espnow.h>
}

// packet format:  { 0xFF, 0xFF, class, type }
//   class : req, ack, data
//   type  : dataType start from 0x81, reqType start from 0x1
#define ESPNOW_IS_MSG_PCK(data)  (data[0]==0xFF && data[1]==0xFF)
#define ESPNOW_IS_REQ_PCK(data)  (ESPNOW_IS_MSG_PCK(data) && data[2]==0x1)
#define ESPNOW_IS_ACK_PCK(data)  (ESPNOW_IS_MSG_PCK(data) && data[2]==0x0)
#define ESPNOW_IS_DAT_PCK(data)  (ESPNOW_IS_MSG_PCK(data) && data[2]==0x2)
#define ESPNOW_REQ_WAKEUP  {0xFF, 0xFF, 0x1, 0x2}
#define ESPNOW_ACK_WAKEUP  {0xFF, 0xFF, 0x0, 0x2}
#define ESPNOW_REQ_POLL    {0xFF, 0xFF, 0x1, 0x1}
#define ESPNOW_ACK_POLL    {0xFF, 0xFF, 0x0, 0x1}
#define ESPNOW_DAT_DATA    {0xFF, 0xFF, 0x2, 0x80}
#define ESPNOW_ACK_DATA    {0xFF, 0xFF, 0x0, 0x80}
#define ESPNOW_DAT_TIME    {0xFF, 0xFF, 0x2, 0x81}
#define ESPNOW_ACK_TIME    {0xFF, 0xFF, 0x0, 0x81}


// esp now buffer
const int ESPNOW_BUFFER_SIZE = 5;

class EspNowBufferClass {
 public:
  String   log = "";
  // recv data
  int recvDataNum = 0;
  struct {
    uint8_t  mac[6];
    uint8_t  data[250]; // 250 = max bytes in payload
    int8_t  espNo; // 
    uint8_t  len; // data length
  } recvData[ESPNOW_BUFFER_SIZE];
  // recv req
  int recvReqNum = 0;
  struct {
    uint8_t  mac[6];
    uint8_t  data[4];
    int8_t  espNo;
  } recvReq[ESPNOW_BUFFER_SIZE];
  // recv ack
  int recvAckNum = 0;
  struct {
    uint8_t  mac[6];
    uint8_t  data[4];
    int8_t  espNo;
  } recvAck[ESPNOW_BUFFER_SIZE];
  //
  bool recvAckExists(uint8_t *mac, uint8_t ackType /*data[3]*/ );
  int recvDataBufferMax() { return recvDataNum < ESPNOW_BUFFER_SIZE ? recvDataNum : ESPNOW_BUFFER_SIZE; };
  int recvReqBufferMax()  { return recvReqNum  < ESPNOW_BUFFER_SIZE ? recvReqNum  : ESPNOW_BUFFER_SIZE; };
  bool store(uint8_t *mac, uint8_t *data, uint8_t len);
  void processAllReq(void (*reqReaction)(int));
  String getDataFromDataBuffer(int i);
};

extern EspNowBufferClass espNowBuffer;

// call backs
//void default_send_cb(uint8_t* macaddr, uint8_t status);
//void default_recv_cb(uint8_t *macaddr, uint8_t *data, uint8_t len);

// setup
void setupEspNow(uint8_t *mac, void (*send_cb)(uint8_t *, uint8_t),
                               void (*recv_cb)(uint8_t *, uint8_t *, uint8_t));

// send packet
bool sendEspNow(uint8_t *macaddr, String message, uint8_t dataType);
bool sendEspNow(uint8_t *macaddr, uint8_t *data, int len);

// misc
String sprintEspNowData(uint8_t *data, int len);

#endif // _espnowLib_h

