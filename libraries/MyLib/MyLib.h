#ifndef _MyLib_h
#define _MyLib_h

#ifdef ARDUINO_ARCH_ESP8266   // this macro is defined by Arduio IDE
#else
#define MYLIB_ARDUINO
#endif


#ifdef MYLIB_ARDUINO
#else
#include <ESP8266WiFi.h>
#include <FS.h>
#endif

#include <TimeLib.h>

// **** MAC Address ******************************************************************************
#ifndef MYLIB_ARDUINO
const int numMacAddr = 8;
extern uint8_t macAddrSTA[numMacAddr][6];
extern uint8_t macAddrAP[numMacAddr][6];
int getIdOfMacAddrSTA(uint8_t *mac);
#endif

// **** Utils *************************************************************************************
#ifndef MYLIB_ARDUINO
void ESP_restart();
void ESP_deepSleep(uint32_t time_us, RFMode mode);
bool ESP_rtcUserMemoryWrite(String text);
String ESP_rtcUserMemoryRead();
#endif

void updateDDNS();

String URLEncode(String smsg);

#define THIS_SKETCH getThisSketch(__FILE__,__DATE__,__TIME__)
String getThisSketch(const char *src, const char *date, const char *time);

void printSystemInfo();
String getSystemInfo();
String getFSInfo();


time_t getNow();
String getDateTime(time_t tm);
String getDateTimeNow();
time_t makeTime(byte sec, byte min, byte hour, byte day, byte month, int year );

String getStatus();

bool WiFiConnect();
bool WiFiConnect(const char *ssid, const char *password);

// Interval Timer
class CheckInterval {
 public:
  CheckInterval();
  CheckInterval(unsigned long interval);
  CheckInterval(unsigned long interval, int timeSrc);
  bool check();
  void setTimeSource(int src) { _timeSrc = src; };
  int  getTimeSource()        { return _timeSrc; };
  void set(unsigned long interval); // in msec
  unsigned long get() {return _interval;} // in msec
 private:
  unsigned long getTime(); // in msec
  unsigned long _interval;
  unsigned long _prev;
  int           _timeSrc;
};


#ifdef MYLIB_ARDUINO
#else

class FTPClient {
 public:
  FTPClient();
  ~FTPClient();
  int open(String server, String user, String pass);
  int cd(String dirName);
  int pwd();// TODO:
  int ls();
  int get(String fileName);
  int put(String fileName);
  int append(String fileName);
  int bye();
  String reply; // latest reply string from server
 private:
  String _serverIP;
  WiFiClient _cclient;
  WiFiClient _dclient;
  String getReply();
  void sendCmd(String cmd);
  int getPortFromPASVReply(String reply);
  int put_internal(String fileName, String cmd);
};

bool fileCreate(const char *path);
bool fileDelete(const char *path);
String fileReadAll(const char *path);
bool fileCopy(const char *path1, const char *path2);
bool fileAppend(const char *path, const char *contents);
long fileSize(const char *path);


#endif

class LogFile : public Print {
   public:
	LogFile(String fileName);
	LogFile(String fileName, int maxFileSize);
	void set(String fileName);
	void set(String fileName, int maxFileSize);
	String fileName() { return _fileName; }
	virtual size_t write(uint8_t); // override Print class
	virtual size_t write(const uint8_t *buffer, size_t size); // override Print class
   private:
        long _maxSize = 1000000000;
        long _size = 0;
        String _fileName;
};


class DebugOutClass : public Print {
   public:
	DebugOutClass() : _type(0), _type_prev(0), _logFile("/debugout_log.txt",100000) {}
	void setToSerial(){_type_prev=_type; _type=0;}
	void setToNull(){_type_prev=_type; _type=1;}
	void setToFile(){ _type_prev=_type; _type=2; } // dont change file name
	void setToFile(String file){ setToFile(); _logFile.set(file); }
	void setToFile(String file, int maxSize){ setToFile(); _logFile.set(file, maxSize); }
  	void setToPrevious(){ int t=_type; _type=_type_prev; _type_prev = t; }
  	int getType(){ return _type; }
  	//void setToType(int type){ _type = type; }
	String FileName() { return _logFile.fileName(); }
	virtual size_t write(uint8_t); // override Print class
	virtual size_t write(const uint8_t *buffer, size_t size); // override Print class
   private:
        int _type;
        int _type_prev;
	LogFile _logFile;
};

extern DebugOutClass DebugOut;


void triggerIFTTT(String event, String value1, String value2, String value3);
void triggerUbidots(String device, String json);

String refreshFS(String tmpDir);

String HttpGet(const char *url);

void sendWoLtoToshiyukiPC();
void sendWoL(byte *mac);

String macAddress2String(uint8_t* macaddr);

//**************** ESP NOW ***********************************************

#ifndef MYLIB_ARDUINO
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
  bool recvAckExists(uint8_t *mac, uint8_t ackType /*data[3]*/ ){
    for(int i=0; i< recvAckNum; i++) {
      if( macAddress2String(mac)==macAddress2String(recvAck[i].mac) && ackType==recvAck[i].data[3] )
        return true;
    }
    return false;
  };
  int recvDataBufferMax() { return recvDataNum < ESPNOW_BUFFER_SIZE ? recvDataNum : ESPNOW_BUFFER_SIZE; };
  int recvReqBufferMax()  { return recvReqNum  < ESPNOW_BUFFER_SIZE ? recvReqNum  : ESPNOW_BUFFER_SIZE; };
  bool store(uint8_t *mac, uint8_t *data, uint8_t len);
  void processAllReq(void (*reqReaction)(int));
  String getDataFromDataBuffer(int i);
};

extern EspNowBufferClass espNowBuffer;

// call backs
void default_send_cb(uint8_t* macaddr, uint8_t status);
void default_recv_cb(uint8_t *macaddr, uint8_t *data, uint8_t len);

// setup
void setupEspNow(uint8_t *mac, void (*send_cb)(uint8_t *, uint8_t),
                               void (*recv_cb)(uint8_t *, uint8_t *, uint8_t));
// send packet
bool sendEspNow(uint8_t *macaddr, String message, uint8_t dataType);
bool sendEspNow(uint8_t *macaddr, uint8_t *data, int len);

// loop func for controller
void loopEspnowController(void (*userFunc)(), void (*reqReaction)(int), uint8_t *slaveMac );

// misc
String sprintEspNowData(uint8_t *data, int len);

#endif

//**************** Json Config ***********************************************

/* Usage Model

  - at first, load
     jsonConfig.load() or loadRtcMem()
  - then refer, modify, and save
     JsonObject& conf = jsonConfig.obj();
     refer or modify conf["xxx"] ...
     jsonConfig.save() or saveRtcMem()
  - multiple call of load is ok
  - use jsonConfig.available() to check already loaded
*/


#include <ArduinoJson.h>

// TODO: should check and avoid from buffer overflow??
const int JSON_CONFIG_BUF_SIZE = 500; // alocated buffer size
const int JSON_CONFIG_BUF_SLACK_SIZE = 50; // make error if less slack
//const int JSON_CONFIG_BUF_SIZE = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2);

// this class to be used by JsonConfig class
class JsonConfigBuffer {
 public:
  StaticJsonBuffer<JSON_CONFIG_BUF_SIZE> jsonBuffer;
  JsonObject& json;
  // costructor
  JsonConfigBuffer(const char *str) : json(jsonBuffer.parseObject(str)) {};
};

// Json Config file/data
class JsonConfig {
 private:
  JsonConfigBuffer *buffer;
 public:
  JsonConfig() : buffer(NULL) {};
  JsonObject& obj() { return buffer ? buffer->json : JsonObject::invalid(); }; // return json obj
  bool available() { return buffer ? true : false; }
  bool load(); // allocate buffer and load from json file
  bool save(); // save to json file
  bool loadRtcMem(); // allocate buffer and load from RTC mem
  bool saveRtcMem(); // save to RTC mem
  bool clear() {delete buffer; }; // free memory
  long remainedCapacity() {return buffer->jsonBuffer.capacity() - buffer->jsonBuffer.size();};
};

extern JsonConfig jsonConfig;

//**************** IR Remote ESP8266 ***********************************************

#ifndef MYLIB_ARDUINO

// Only used for testing; can remove virtual for shorter code
#define VIRTUAL

class IRsend {
public:
  IRsend(int IRsendPin);
  void begin();
  void sendRaw(unsigned int buf[], int len, int hz);
  void sendRaw(unsigned int buf[], int len);
  void enableIROut(int khz);
  void enableIROut(int carrierLowTime, int carrierHighTime);
  VIRTUAL void mark(int usec);
  VIRTUAL void space(int usec);
  void setPulseTimeMult(double m) { pulseTimeMult = m; };
private:
  int carrierLowTime = 16;
  int carrierHighTime = 8;
  double pulseTimeMult = 1.0;
  int IRpin;
};

#endif


#endif


