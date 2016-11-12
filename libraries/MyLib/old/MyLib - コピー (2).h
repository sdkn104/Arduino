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

String getStatus();

bool WiFiConnect();
bool WiFiConnect(const char *ssid, const char *password);

class CheckInterval {
 public:
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
//extern CheckInterval CI;

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
        int _maxSize = 1000000000;
        int _size = 0;
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

String refreshFS(String tmpDir);

String HttpGet(const char *url);

void sendWoLtoToshiyukiPC();
void sendWoL(byte *mac);

String macAddress2String(uint8_t* macaddr);

//**************** ESP NOW ***********************************************

extern "C" {
  #include <espnow.h>
}

// message packet
#define ESPNOW_REQ_WAKEUP  {0xFF, 0xFF, 0x1, 0x2}
#define ESPNOW_ACK_WAKEUP  {0xFF, 0xFF, 0x0, 0x2}
#define ESPNOW_REQ_POLL    {0xFF, 0xFF, 0x1, 0x1}
#define ESPNOW_ACK_POLL    {0xFF, 0xFF, 0x0, 0x1}
#define ESPNOW_ACK_DATA    {0xFF, 0xFF, 0x0, 0x0}

//
typedef struct st_espNowStatus {
  bool send_cb;
  int  sendStatus;
  bool recv_cb; // data receive
  uint8_t  recvMac[6];
  uint8_t  recvData[250]; // max bytes in payload
  int      recvDataLen;
  bool ack_cb;
  uint8_t  ackMac[6];
  uint8_t  ackType;  // 4th byte of ack packet
} EspNowStatus;

extern EspNowStatus espNowStatus;

void default_send_cb(uint8_t* macaddr, uint8_t status);
void default_recv_cb(uint8_t *macaddr, uint8_t *data, uint8_t len);
void cont_recv_cb(uint8_t *macaddr, uint8_t *data, uint8_t len);
void slave_recv_cb(uint8_t *macaddr, uint8_t *data, uint8_t len);
void setupEspNow(uint8_t *mac, void (*send_cb)(uint8_t *, uint8_t),
                               void (*recv_cb)(uint8_t *, uint8_t *, uint8_t));
bool sendEspNow(String message);
bool sendEspNow(uint8_t *message, int len);

String sprintEspNowData(uint8_t *data, int len);

//**************** Json Config ***********************************************

/* Usage Model

  - at first, load
     jsonConfig.load();
  - then refer, modify, and save
     JsonObject& conf = jsonConfig.obj();
     refer or modify conf["xxx"] ...
     jsonConfig.save();
  - dont re-load
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
  JsonObject& obj() { return buffer->json; }; // return json obj with that manipulate the config data
  bool load(); // allocate buffer and load from json file
  bool save(); // save to json file
  bool clear() {delete buffer; }; // free memory
  long remainedCapacity() {return buffer->jsonBuffer.capacity() - buffer->jsonBuffer.size();};
};

extern JsonConfig jsonConfig;

#endif

