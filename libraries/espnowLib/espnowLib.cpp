//
// A Library for ESP8266 ESP-NOW
//

#include <espnowLib.h>

#ifndef _MyLib_h
#define DebugOut Serial
#endif

// packet header format:  { 0xFF, 0xFF, class, type }
//   class : req, ack, data
//   type  : dataType start from 0x81, reqType start from 0x1
//   req and ack dont have body (header only)
//   data can have body
#define ESPNOW_IS_MSG_PCK(data)  (data[0]==0xFF && data[1]==0xFF)
#define ESPNOW_IS_REQ_PCK(data)  (ESPNOW_IS_MSG_PCK(data) && data[2]==0x1)
#define ESPNOW_IS_ACK_PCK(data)  (ESPNOW_IS_MSG_PCK(data) && data[2]==0x0)
#define ESPNOW_IS_DAT_PCK(data)  (ESPNOW_IS_MSG_PCK(data) && data[2]==0x2)
#define ESPNOW_ACK    {0xFF, 0xFF, 0x0}
#define ESPNOW_REQ    {0xFF, 0xFF, 0x1}
#define ESPNOW_DAT    {0xFF, 0xFF, 0x2}
/*
#define ESPNOW_REQ_WAKEUP  {0xFF, 0xFF, 0x1, 0x2}
#define ESPNOW_ACK_WAKEUP  {0xFF, 0xFF, 0x0, 0x2}
#define ESPNOW_REQ_POLL    {0xFF, 0xFF, 0x1, 0x1}
#define ESPNOW_ACK_POLL    {0xFF, 0xFF, 0x0, 0x1}
#define ESPNOW_DAT_DATA    {0xFF, 0xFF, 0x2, 0x80}
#define ESPNOW_ACK_DATA    {0xFF, 0xFF, 0x0, 0x80}
#define ESPNOW_DAT_TIME    {0xFF, 0xFF, 0x2, 0x81}
#define ESPNOW_ACK_TIME    {0xFF, 0xFF, 0x0, 0x81}
*/

//***** local functions ****************************************************************

String macAddrString(uint8_t* macaddr) {
  char s[18];
  sprintf(s,"%02X:%02X:%02X:%02X:%02X:%02X",macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
  return String(s);
}

//*****  EspNowBufferClass ****************************************************************
//
//    buffer for esp-now packet received
//

EspNowBufferClass espNowBuffer;

#define COPY_ARRAY(SRC, DST, LEN) { for(int i=0; i<(LEN); i++){ DST[i]=SRC[i]; } }

bool EspNowBufferClass::store(uint8_t *mac, uint8_t *data, uint8_t len) {
  if( ESPNOW_IS_ACK_PCK(data) ) {
    int idx = (recvAckNum) % EspNowBufferSize; // ring buffer
    COPY_ARRAY(mac,  recvAck[idx].mac, 6);
    COPY_ARRAY(data, recvAck[idx].data, len);
    recvAck[idx].espNo = -1;
    recvAckNum++;
    if( recvAckNum - 1 >= EspNowBufferSize ) {
      log +=  "Error: espnow store buffer overflow\r\n";
      return false;
    }
  } else if( ESPNOW_IS_REQ_PCK(data) ) {
    int idx = (recvReqNum) % EspNowBufferSize; // ring buffer
    COPY_ARRAY(mac,  recvReq[idx].mac, 6);
    COPY_ARRAY(data, recvReq[idx].data, len);
    recvReq[idx].espNo = -1;
    recvReqNum++;
    if( recvReqNum - 1 >= EspNowBufferSize ) {
      log +=  "Error: espnow store buffer overflow\r\n";
      return false;
    }
  } else if( ESPNOW_IS_DAT_PCK(data) ) { // data packet
    int idx = (recvDataNum) % EspNowBufferSize; // ring buffer
    COPY_ARRAY(mac,  recvData[idx].mac, 6);
    COPY_ARRAY(data, recvData[idx].data, len);
    recvData[idx].len = len;
    recvData[idx].espNo = -1;
    recvDataNum++;
    if( recvDataNum - 1 >= EspNowBufferSize ) {
      log +=  "Error: espnow store buffer overflow\r\n";
      return false;
    }
  }
  return true;
}

// check ack packet existence in the buffer
bool EspNowBufferClass::recvAckExists(uint8_t *mac, uint8_t ackType ){
    for(int i=0; i< recvAckNum; i++) {
      if( macAddrString(mac)==macAddrString(recvAck[i].mac) && ackType==getTypeFromAckBuffer(i) )
        return true;
    }
    return false;
}

// re-action for request
void EspNowBufferClass::processAllReq(void (*reqReaction)(int)){
  for (int i = 0; i < recvReqBufferMax(); i++ ) { // for each request in buffer
    (*reqReaction)(i);
  }
  clearReqBuffer();
}


// extract data from data packet
String EspNowBufferClass::getDataFromDataBuffer(int i){
  char buf[251]; // max payload + '\0'
  memcpy(buf, recvData[i].data, recvData[i].len);
  buf[recvData[i].len] = '\0';
  return String(buf+4); // ommit 4-byte header
}


//*****  ESPNOW call backs ****************************************************************

// !!! NEVER call delay()/yield() in call back functions

// default send callback
//   -- store log to espNowBuffer
void  default_send_cb(uint8_t* macaddr, uint8_t status) {
  String log = "";
  log += "send_cb\r\n";
  if( status != 0 ) log += "Error: send error\r\n";
  // debug
  log += " mac address: " + macAddrString(macaddr) + "\r\n";
  log += " status = " + String(status) + "\r\n"; //0:success
  espNowBuffer.log += log;
}


// default receive callback
//    - send ack
//    - store packet to espNowBuffer
//    - store log to espNowBuffer
void default_recv_cb(uint8_t *macaddr, uint8_t *data, uint8_t len) {
  String log = "";
  log += "recv_cb\r\n";
  // debug
  log += " mac address: " + macAddrString(macaddr) + "\r\n";
  log += " data: "+ sprintEspNowData(data,len) + "\r\n";
  // send ack
  if( ESPNOW_IS_REQ_PCK(data) || ESPNOW_IS_DAT_PCK(data) ) {
    log += "send ack...\r\n";
    uint8_t ack[4] = ESPNOW_ACK;
    ack[3] = data[3];
    esp_now_send(macaddr, ack, 4); // ack
  }
  // log
  espNowBuffer.log += log;
  // store
  espNowBuffer.store(macaddr, data, len);
}



//*****  SETUP ****************************************************************

void setupEspNow(uint8_t *mac, void (*send_cb)(uint8_t *, uint8_t),
                               void (*recv_cb)(uint8_t *, uint8_t *, uint8_t)) {
  DebugOut.println("setup esp-now...");
  WiFi.setAutoConnect(false); // turn off auto connect on power-on. this will be effective at next power-on

  if ( recv_cb == NULL ) {
    recv_cb = default_recv_cb;
  }
  if ( send_cb == NULL ) {
    send_cb = default_send_cb;
  }

  if (esp_now_init() == 0) {
    DebugOut.println("direct link  init ok");
  } else {
    DebugOut.println("direct link  init failed");
  }

  int r;
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER); // COMBO is better? 
                                                  // User guide says this setting not affect for slave
                                                  // This is effective for STA_AP mode of controller
  r = esp_now_register_recv_cb(recv_cb);
  r = esp_now_register_send_cb(send_cb);
  //r = esp_now_add_peer(mac, (uint8_t)ESP_NOW_ROLE_SLAVE, (uint8_t)WIFI_DEFAULT_CHANNEL, NULL, 0);
  // -> mac is ignored when it is explisitly specified, channel does not affect any function (by user guide)
}


//*****  SEND ****************************************************************

bool sendEspNowReq(uint8_t *mac, uint8_t type) {
  uint8_t buf[4] = ESPNOW_REQ;
  buf[3] = type;
  sendEspNow(mac, buf, 4);
}

// send string as ESP NOW data packet (waiting ack)
bool sendEspNowData(uint8_t *mac, String message, uint8_t type) {
  char buf[251] = ESPNOW_DAT; // 250=max payload
  buf[3] = type;
  message.toCharArray(buf+4, 251-4); // add 0 at the last
  int len = message.length() > 250-4 ? 250-4 : message.length(); // data length + 4(header) <= 250 bytes
  sendEspNow(mac, (uint8_t *)buf, len+4);
}

// send data thru ESP NOW packet (waiting ack)
bool sendEspNow(uint8_t *mac, uint8_t *data, int len) {
  DebugOut.println("send esp-now expecting reply...");
  // check
  if ( WiFi.isConnected() )
    DebugOut.println("error: WiFi is connected. esp-now send will fail.");
  if ( len > 250 )
    DebugOut.println("error: data length > 250. cannot send esp-now.");
  DebugOut.println(" data: "+ sprintEspNowData(data,len));
  DebugOut.println(" mac : "+ macAddrString(mac));
  // send
  int ackType = data[3];
  espNowBuffer.clearAckBuffer(); //ack should be checked only in this function
  bool success = false;
  for (int retry = 0; retry < 3 && !success; retry++) {
    DebugOut.println("esp-now send trial #" + String(retry));
    esp_now_send(mac, data, len);
    for (int i = 0; i < 30; i++) {
      DebugOut.print(espNowBuffer.log); // log print
      espNowBuffer.log = "";
      if ( espNowBuffer.recvAckExists(mac, ackType) ) {
        DebugOut.println("ack received");
        success = true;
        break;
      }
      delay(100);
    }
  }
  if ( success ) {
    DebugOut.println("esp-now sent successfully");
    return true;
  } else {
    DebugOut.println("error: fail to send esp-now");
    return false;
  }
}

//***** MISC ****************************************************************

// print string or binary data
String sprintEspNowData(uint8_t *data, int len) {
  String out = "";
  for(int i=0; i<len; i++){
    if( isprint(data[i]) ) {
      out += String((char)data[i]);
    } else {
      out += String("(0x")+String(data[i], HEX)+")";
    }
  }
  return out;
}

