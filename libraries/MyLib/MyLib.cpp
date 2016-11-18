
#ifdef ARDUINO_ARCH_ESP8266   // this macro is defined by Arduio IDE
#else
#define MYLIB_ARDUINO
#include <Arduino.h>
#endif

#include <MyLib.h>

#ifndef MYLIB_ARDUINO
extern "C" {
#include <user_interface.h> // for sleep mode
}
#include <ESP8266HTTPClient.h>
#include <WakeOnLan.h>
#endif

// **** MAC Address ******************************************************************************
#ifndef MYLIB_ARDUINO

uint8_t macAddrSTA[numMacAddr][6] = {
 {0,0,0,0,0,0}, // No.0
 {0x18,0xFE,0x34,0xD5,0xC7,0xA7}, // No.1
 {0,0,0,0,0,0},
 {0x5C,0xCF,0x7F,0x17,0xB0,0x99}, // No.3
 {0x5C,0xCF,0x7F,0x17,0xC0,0xB2}, // No.4
 {0x5C,0xCF,0x7F,0x17,0xB6,0x1F}, // No.5
 {0x5C,0xCF,0x7F,0x16,0xDE,0x9F}, // No.6
 {0x5C,0xCF,0x7F,0xD6,0x50,0xB4}  // No.7
};

uint8_t macAddrAP[numMacAddr][6] = {
 {0,0,0,0,0,0}, // No.0
 {0x1A,0xFE,0x34,0xD5,0xC7,0xA7}, // No.1
 {0,0,0,0,0,0},
 {0x5E,0xCF,0x7F,0x17,0xB0,0x99}, // No.3
 {0x5E,0xCF,0x7F,0x17,0xC0,0xB2}, // No.4
 {0x5E,0xCF,0x7F,0x17,0xB6,0x1F}, // No.5
 {0x5E,0xCF,0x7F,0x16,0xDE,0x9F}, // No.6
 {0x5E,0xCF,0x7F,0xD6,0x50,0xB4}  // No.7
};

int getIdOfMacAddrSTA(uint8_t *mac) {
  for(int id=0; id<numMacAddr; id++){
    if( macAddress2String(mac) == macAddress2String(macAddrSTA[id]) )
      return id;
  }
  return -1;
}

#endif // MYLIB_ARDUINO

// **** Utils *************************************************************************************
#ifndef MYLIB_ARDUINO

void ESP_restart() {
  jsonConfig.save();
  SPIFFS.end();
  ESP.restart();
}

void ESP_deepSleep(uint32_t time_us, RFMode mode) {
  jsonConfig.save();
  SPIFFS.end();
  ESP.deepSleep(time_us, mode);
}

// 
bool ESP_rtcUserMemoryWrite(String text) {
  char buf[512];
  if( text.length() + 1 > 512 ) {
    DebugOut.println("Error: too long string to save RTC memory.");
    return false;
  }
  text.toCharArray(buf, 512);  // toCharArray added 0 at the end of string
  return ESP.rtcUserMemoryWrite(0, (uint32_t *)buf, 512);
}

String ESP_rtcUserMemoryRead() {
  uint32_t buf[512/4];
  char *str = (char *)buf;

  if( ! ESP.rtcUserMemoryRead(0, buf, 512) )
    return String("");      

  // check null existence
  for(int i=0; i<512; i++) {
    if( str[i] == 0 ) 
      return String(str);
  }
  return String(""); // not a string
}

#endif // MYLIB_ARDUINO

// **** DDNS *************************************************************************************

#ifdef MYLIB_ARDUINO
#else
void updateDDNS(){
  //String addr = "http://dynupdate.no-ip.com/nic/update?hostname=sdkn104.hopto.org";
  //  http://sdkn104:gorosan@dynupdate.no-ip.com/nic/update?hostname=sdkn104.hopto.org
  HTTPClient http;
  http.begin("http://dynupdate.no-ip.com/nic/update?hostname=sdkn104.hopto.org");
  http.setAuthorization("sdkn104", "gorosan");
  int httpCode = http.GET();
  // httpCode will be negative on error
  if(httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    DebugOut.println(String("connected to DDNS. Status code: ")+ httpCode);
  } else {
    DebugOut.println(String("failed to connect to DDNS, error: ")+ http.errorToString(httpCode).c_str());
  }
  http.end();


#if 0
  WiFiClient client;
  if (client.connect("dynupdate.no-ip.com", 80)) {
    DebugOut.println("connected to DDNS");
    String s = "GET /nic/update?hostname=sdkn104.hopto.org HTTP/1.1\r\n";
    client.print(s);
    client.print("Host: dynupdate.no-ip.com\r\n");
    client.print("Connection: close\r\n");     
    client.print("Accept: */*\r\n");   
    client.print("User-Agent: Mozilla/4.0 (compatible; esp8266 Lua; Windows NT 5.1)\r\n");   
    client.print("\r\n");
    DebugOut.println("Request has sent to DDNS");
  }
#endif

}
#endif

// ***** URL Encode ******************************************************************************

String URLEncode(String smsg) {
    const char *hex = "0123456789abcdef";
    const char *msg = smsg.c_str();

    String encodedMsg = "";
    while (*msg!='\0'){
        if( ('a' <= *msg && *msg <= 'z')
                || ('A' <= *msg && *msg <= 'Z')
                || ('0' <= *msg && *msg <= '9') ) {
            encodedMsg += *msg;
        } else {
            encodedMsg += '%';
            encodedMsg += hex[*msg >> 4];
            encodedMsg += hex[*msg & 15];
        }
        msg++;
    }
    //DebugOut.println(encodedMsg);
    return encodedMsg;
}

//***** Get Status ***********************************************************************

String getThisSketch(const char *src, const char *date, const char *time){
  String the_path = src;
  int slash_loc = the_path.lastIndexOf('\\');
  String out = the_path.substring(slash_loc+1);
  out = out + " (compiled on: " + date + " at " + time + ")";
  return out;
}


#ifdef MYLIB_ARDUINO
#else
String getSystemInfo() {
  String o = String("");
  o += "**** system info *******\r\n";
  o += "getSdkVersion: " + String(ESP.getSdkVersion());
  o += "\r\ngetBootVersion: " + String(ESP.getBootVersion());
  o += "\r\nChipID: ";
  o += ESP.getChipId();
  o += "\r\nFlashChipID: 0x";
  o += ESP.getFlashChipId(), HEX;
  o += "\r\nFlashChipSpeed[Hz]: ";
  o += ESP.getFlashChipSpeed();
  o += "\r\nFlashChipMode: ";
  o += ESP.getFlashChipMode();
  o += "=";
  FlashMode_t ideMode = ESP.getFlashChipMode();
  o += (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN");
  o += "\r\nFlashChipSize[byte]: ";
  o += ESP.getFlashChipSize();
  o += "\r\nFlashChipRealSize[byte]: ";
  o += ESP.getFlashChipRealSize();
  o += "\r\nSketch size: ";
  o += ESP.getSketchSize();
  o += "\r\nFree Sketch Size: ";
  o += ESP.getFreeSketchSpace();
  o += "\r\nFreeHeap[byte]: ";
  o += ESP.getFreeHeap();
  o += "\r\nWiFi Mode: ";
  o += WiFi.getMode();
  o += "\r\nMAC Address for STA: ";
  o += WiFi.macAddress();
  o += "\r\nMAC Address for AP:  ";
  o += WiFi.softAPmacAddress();
  o += "\r\nlocal IP for STA: ";
  o += WiFi.localIP().toString();
  o += "\r\nlocal IP for AP: ";
  o += String(WiFi.softAPIP());
  o += "\r\nReset Reason: " + String(ESP.getResetReason());
  o += "\r\ngetResetInfo: " + String(ESP.getResetInfo());
  o += "\r\nBoot mode: " + String(ESP.getBootMode());
  o += "\r\nESP start time: " + getDateTime(getNow()-millis()/1000) + ", " + millis()/1000/60/60 + " hours ago";
  String t = wifi_get_sleep_type()==NONE_SLEEP_T ? "NONE" : wifi_get_sleep_type()==MODEM_SLEEP_T ? "MODEM" : wifi_get_sleep_type()==LIGHT_SLEEP_T ? "LIGHT" : "OTHER";
  o += "\r\nSleep mode: " + t;
  o += "\r\n***********************\r\n";
  return o;
}

void printSystemInfo() {
  DebugOut.print(getSystemInfo());
}

String getFSInfo() {
  FSInfo fs_info;
  if( !SPIFFS.info(fs_info) ) { DebugOut.println("Error: fail to get info"); }
  String o = String("");
  o += "**** file system info *******\r\n";
  o += "totalBytes: ";
  o += fs_info.totalBytes;
  o += "\r\nusedBytes: ";
  o += fs_info.usedBytes;
  o += "\r\nblockSize: ";
  o += fs_info.blockSize;
  o += "\r\npageSize: ";
  o += fs_info.pageSize;
  o += "\r\nmaxOpenFiles: ";
  o += fs_info.maxOpenFiles;
  o += "\r\nmaxPathLength: ";
  o += fs_info.maxPathLength;
  o += "\r\n***********************\r\n";
  return o;
}

String getStatus() {
  String o = String();
  o += String("analogRead(A0): ") + analogRead(A0);
  o += String("\r\ngetVcc: ") + ESP.getVcc() + "  " + ESP.getVcc()/1024.0 + "V  -- valid only if TOUT pin is open and ADC_MODE(ADC_VCC) called";
  o += "\r\ndigitalRead(0): " + String(digitalRead(0));
  return o;
}
#endif

// ***** Interval Timer ********************************************************************************

CheckInterval::CheckInterval() : CheckInterval(1000, 0) {}

CheckInterval::CheckInterval(unsigned long interval) : CheckInterval(interval, 0) {}

CheckInterval::CheckInterval(unsigned long interval, int timeSrc) {
  _timeSrc = timeSrc;
  _interval = interval;
  _prev = -1; // fire also at the first time
  DebugOut.print("init interval ");
  DebugOut.println(interval);
}

unsigned long CheckInterval::getTime() {
  if( _timeSrc == 0 ) return millis(); // [msec]
  else                return now()*1000; // [msec], TimeLib.h
}

bool CheckInterval::check() {
  //DebugOut.println(String("check interval; ")+getTime()+" "+_prev+" "+_interval);
  unsigned long c = getTime() / _interval;
  if( c != _prev ) {
    _prev = c;
    return true;
  }
  return false;
}

void CheckInterval::set(unsigned long interval) {
  _interval = interval;
}


// **** Time Utility ***************************************************************


// return current Unix Time (seconds from Unix Epoch 1970/1/1 0:00 GMT(UTC))
time_t getNow() {
  return now(); // based on Time.h
}


// unix time to local time (JST) string
String getDateTime(time_t tm){
  char s[20];
  const char* format = "%04d-%02d-%02d %02d:%02d:%02d";
  time_t t = tm + 9*60*60; // JST
  sprintf(s, format, year(t), month(t), day(t), hour(t), minute(t), second(t));
  return String(s) + " JST";
}


String getDateTimeNow(){
  return getDateTime(getNow());
}


// converts time components to time_t (number of seconds from 1970/1/1 0:00, not accounting leap seconds)
// note: Unix Time also does not account leap seconds by definition
// note: year argument is full four digit year (or digits since 2000), i.e.1975, (year 8 is 2008)  
//       month argument = the month number - 1 (0,1,2,...,11)
// * copy from DateTime.h by T.Sadakane
#define LEAP_YEAR(_year) ((_year%4)==0)
static  byte monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};
time_t makeTime(byte sec, byte min, byte hour, byte day, byte month, int year ){
   int i;
   time_t seconds;

   if(year < 69) 
      year+= 2000;
    // seconds from 1970 till 1 jan 00:00:00 this year
    seconds= (year-1970)*(60*60*24L*365);

    // add extra days for leap years
    for (i=1970; i<year; i++) {
        if (LEAP_YEAR(i)) {
            seconds+= 60*60*24L;
        }
    }
    // add days for this year
    for (i=0; i<month; i++) {
      if (i==1 && LEAP_YEAR(year)) { 
        seconds+= 60*60*24L*29;
      } else {
        seconds+= 60*60*24L*monthDays[i];
      }
    }

    seconds+= (day-1)*3600*24L;
    seconds+= hour*3600L;
    seconds+= min*60L;
    seconds+= sec;
    return seconds; 
}


// *************** WiFi Connect *************************************************

#ifdef MYLIB_ARDUINO
#else

bool WiFiConnect() {
//  const char* ssid = "aterm-043262-g";
//  const char* password = "6f40290eb9eb8"; // wifi password
  const char* ssid = "HG8045-FB5C-bg";
  const char* password = "57dtsyne"; // wifi password
  return WiFiConnect(ssid, password);
}

bool WiFiConnect(const char *ssid, const char *password) {
  DebugOut.print("connecting to WiFi ");
  WiFi.mode(WIFI_STA); // default: WIFI_AP_STA, or load from flash (persistent function)
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DebugOut.print(".");
  }
  DebugOut.println(" connected");
  DebugOut.print("local IP: ");
  DebugOut.println(WiFi.localIP().toString());
  return true;
}

#endif

//**** FTP Client *******************************************************

#ifdef MYLIB_ARDUINO
#else

/*
   FTP passive client
*/

FTPClient::FTPClient() {}

FTPClient::~FTPClient() {
  DebugOut.println("FTP client stop");
  _cclient.stop();
  _dclient.stop();
}

int FTPClient::open(String server, String user, String pass) {
  _serverIP = server;
  if (_cclient.connect(_serverIP.c_str(),21)) {
    DebugOut.println(F("connected to FTP server"));
  } else {
    DebugOut.println(F("failed to connect to FTP server"));
    return 0;
  }
  if( getReply() > "220") { _cclient.stop(); return 0; }
  
  sendCmd(String(F("USER "))+user);
  if( getReply() > "331")  return 0;

  sendCmd(String(F("PASS "))+pass);
  if( getReply() > "230")  return 0;

  sendCmd(F("SYST"));
  if( getReply() > "215")  return 0;

  sendCmd(F("Type I"));
  if( getReply() > "200")  return 0;

  return 1;
}

int FTPClient::get(String fileName) {
  sendCmd(F("PASV"));
  if( getReply() > "227") return 0;
  unsigned int port = getPortFromPASVReply(reply);

  if (_dclient.connect(_serverIP.c_str(),port)) {
    DebugOut.print(F("connected to Data port " ));
    DebugOut.println(port);
  } else {
    DebugOut.print(F("failed to connect to Data port "));
    DebugOut.println(port);
    return 0;
  }

  fileName.trim();
  if( fileName.startsWith("/") ) fileName = fileName.substring(1);
  sendCmd(String(F("RETR ")) + fileName);
  if( getReply() > "226")  return 0;

  if( !SPIFFS.begin() ) DebugOut.println("Error: fail to mount SPIFFS"); // it is safe to duplicate call this
  File f = SPIFFS.open(String("/")+fileName, "w");
  if (!f) {
    DebugOut.println("file open failed");
    return 0;
  }
  while(_dclient.connected()) {
    while(_dclient.available()) {
      char c = _dclient.read();  // TODO: buffering?
      f.write(c);      
      delay(0);
    }
  }
  f.close();
  _dclient.stop();
  DebugOut.println(F("disconnected to Data port"));
  if( getReply() > "226") return 0; // server reply after it disconnect (to show EOF)

  return 1;
}

int FTPClient::put(String fileName) {
  return put_internal(fileName, "STOR");
}

int FTPClient::append(String fileName) {
  return put_internal(fileName, "APPE");
}

int FTPClient::put_internal(String fileName, String cmd) {
  sendCmd(F("PASV"));
  if( getReply() > "227") return 0;
  unsigned int port = getPortFromPASVReply(reply);

  if (_dclient.connect(_serverIP.c_str(),port)) {
    DebugOut.print(F("connected to Data port " ));
    DebugOut.println(port);
  } else {
    DebugOut.print(F("failed to connect to Data port "));
    DebugOut.println(port);
    return 0;
  }

  fileName.trim();
  if( fileName.startsWith("/") ) fileName = fileName.substring(1);
  sendCmd(String(cmd) + " " + fileName);
  if( getReply() > "226")  return 0;

  if( !SPIFFS.begin() ) DebugOut.println("Error: fail to mount SPIFFS");
  File f = SPIFFS.open(String("/")+fileName, "r");
  if (!f) {
    DebugOut.println("file open failed");
    return 0;
  }

  while(f.available()) {
    char c = f.read();  // TODO: buffering?
    _dclient.write(c);      
    delay(0);
  }
  f.close();
  _dclient.stop(); // close connection to send EOF
  DebugOut.println(F("disconnected to Data port"));
  if( getReply() > "226") return 0;

  return 1;
}

int FTPClient::getPortFromPASVReply(String reply) {
  char sarr[128];
  reply.toCharArray(sarr,128);
  char *tStr = strtok(sarr,"(,");
  int array_pasv[6];
  for ( int i = 0; i < 6; i++) {
    tStr = strtok(NULL,"(,");
    array_pasv[i] = atoi(tStr);
    if(tStr == NULL) {
      DebugOut.println(F("Bad PASV Answer"));    
    }
  }  
  unsigned int hiPort = array_pasv[4] << 8;
  unsigned int loPort = array_pasv[5] & 255;
  hiPort = hiPort | loPort;
  return (int)hiPort;
}

int FTPClient::bye() {
  sendCmd(F("QUIT"));
  if( getReply() > "221") return 0;

  _cclient.stop();
  DebugOut.println(F("disconnected to FTP server"));
  return 1;
}

int FTPClient::pwd() {
  sendCmd(F("XPWD"));
  if( getReply() != "257") return 0;
  return 1;
}

int FTPClient::cd(String dirName) {
  dirName.trim();
  sendCmd(String(F("CWD ")) + dirName);
  if( getReply() > "250") return 0;
  return 1;
}

int FTPClient::ls() {
  sendCmd(F("PASV"));
  if( getReply() > "227") return 0;
  unsigned int port = getPortFromPASVReply(reply);

  if (_dclient.connect(_serverIP.c_str(),port)) {
    DebugOut.print(F("connected to Data port " ));
    DebugOut.println(port);
  } else {
    DebugOut.print(F("failed to connect to Data port "));
    DebugOut.println(port);
    return 0;
  }

  sendCmd(F("LIST"));
  if( getReply() >= "200")  return 0;

  while(_dclient.connected()) {
    while(_dclient.available()) {
      char c = _dclient.read();  // TODO: buffering?
      char s[2] = { 0, 0};
      s[0] = c;
      DebugOut.print(s);      
      delay(0);
    }
  }
  _dclient.stop(); // close connection to send EOF
  DebugOut.println(F("disconnected to Data port"));
  if( getReply() >= "200") return 0;

  return 1;
}


String FTPClient::getReply() {
  _cclient.setTimeout(1000); // in msec
  String buf = _cclient.readStringUntil('\n'); // TODO: multiple line reply
  DebugOut.println(buf);
  reply = buf;
  return buf.substring(0,3);
}

void FTPClient::sendCmd(String cmd){
  _cclient.println(cmd);
  DebugOut.print("---> ");
  DebugOut.println(cmd);
}

#endif

//***** File *****************************************************************

#ifndef MYLIB_ARDUINO

bool fileCreate(const char *path) {
  File file = SPIFFS.open(path, "w");
  if(!file) {
    DebugOut.println("Error: fail to open file");
    return false;
  }
  file.close();
  return true;
}

bool fileDelete(const char *path) {
  if( ! SPIFFS.exists(path) ) 
    return true;
  else {
    bool res = SPIFFS.remove(path);
    if( !res ) DebugOut.println("Error: fail to remove file");
    return res;
  }
}

String fileReadAll(const char *path) {
  if(path == "/") {
    DebugOut.println("ERROR: cannot read. bad path.");
    return "";
  }
  if(!SPIFFS.exists(path)){
    DebugOut.println("ERROR: cannot read. file not found.");
    return "";
  }
  File file = SPIFFS.open(path, "r");
  if(!file) {
    DebugOut.println("ERROR: cannot read. open fail.");
    return "";
  }
  String out = "";
  while (file.available()) {
    int t = file.read();
    if( t == -1 ) break; 
    out += String((char)t);
  }
  file.close();
  return out;
 }

bool fileCopy(const char *path1, const char *path2) {
  if(path1 == "/" || path2 == "/") {
    DebugOut.println("ERROR: cannot copy. bad path.");
    return false;
  }
  if(!SPIFFS.exists(path1)){
    DebugOut.println("ERROR: cannot copy. file not found.");
    return false;
  }
  if(SPIFFS.exists(path2)){
    DebugOut.println("ERROR: cannot copy. file exists.");
    return false;
  }
  File file1 = SPIFFS.open(path1, "r");
  if(!file1) {
    DebugOut.println("ERROR: cannot copy. open fail.");
    return false;
  }
  File file2 = SPIFFS.open(path2, "w");
  if(!file2) {
    file1.close();
    DebugOut.println("ERROR: cannot copy. open fail.");
    return false;
  }
  while (file1.available()) {
    int t = file1.read();
    if( t == -1 ) break; 
    file2.write(t);
  }
  file1.close();
  file2.close();
  return true;
}

bool fileAppend(const char *path, const char *contents) {
  if(path == "/") {
    DebugOut.println("ERROR: bad path.");
    return false;
  }
  File file = SPIFFS.open(path, "a");
  if(!file) {
    DebugOut.println("ERROR: open fail.");
    return false;
  }
  file.print(contents);
  file.close();
  return true;
}

long fileSize(const char *path) { 
    long sz = 0;
    File file = SPIFFS.open(path, "r");
    if (file) {
      sz = file.size();
      file.close();
    } else {
      DebugOut.println("Error: cannot open file.");
    }
    return sz;
}


#endif


//***** LogFile ****************************************************************


// !!!! DONT USE DebugOut in LogFile Class, to avoid recursive loop

LogFile::LogFile(String fileName, int maxFileSize){
  set(fileName, maxFileSize);
}

LogFile::LogFile(String fileName){
  set(fileName);
}

void LogFile::set(String fileName, int maxFileSize){
  _fileName = fileName;
  _maxSize = maxFileSize;
}

void LogFile::set(String fileName){
  _fileName = fileName;
}

#define PRINTERR(STR) { if( _fileName == DebugOut.FileName() ) Serial.println(STR); else DebugOut.println(STR); }

size_t LogFile::write(uint8_t c) {
#ifdef MYLIB_ARDUINO
  Serial.write(c);
#else
  if( _size >= _maxSize ) {
    String fd = String(_fileName) + "_prev";
    if( SPIFFS.exists(fd) ) { if( !SPIFFS.remove(fd) ) PRINTERR("Error: fail to remove file"); }
    if( SPIFFS.rename(_fileName.c_str(), fd.c_str()) ) { PRINTERR("Error: fail to rename file"); }
    _size = 0;
  }

  File fs = SPIFFS.open(_fileName, "a");
  if(!fs){ 
      PRINTERR(String("ERROR: fail to open file: ")+_fileName);
      if( _fileName == DebugOut.FileName() ) {
        Serial.println(" call SetToSerial()");
        DebugOut.setToSerial(); // to prevent from SPIFFS trouble
      }
      return 0;
  }
  if( _size % 100 == 0 ) {
    _size = fs.size();
  }
  fs.print((char)c);
  fs.close();
  _size++;

  return 1;
#endif
}

size_t LogFile::write(const uint8_t *buffer, size_t size) {
  for(int i=0; i<size; i++){
    write(buffer[i]); // todo: buffering, for flash lifetime
  }
  return size;
}


//***** DebugOut *****************************************************************


DebugOutClass DebugOut;


size_t DebugOutClass::write(uint8_t c) {
  if(_type == 0 ) { return Serial.print((char)c); }
  else if( _type == 1 ) { return 0; }
#ifndef MYLIB_ARDUINO
  else if( _type == 2 ) {
    if( !SPIFFS.begin() ) Serial.println("Error: fail to mount SPIFFS"); 
    return _logFile.write(c); 
  }
#endif
  return 0;
}

size_t DebugOutClass::write(const uint8_t *buffer, size_t size) {
  for(int i=0; i<size; i++){
    write(buffer[i]);
  }
  return size;
}

//***** IFTTT *****************************************************************

#ifndef MYLIB_ARDUINO

#if 0
void triggerIFTTT(String event, String value1, String value2, String value3){
    String key = "uoVHdqccPNfLG8UbUR6Al";
    WiFiClient client;
    if (client.connect("maker.ifttt.com", 80)) {
      DebugOut.println("connected to ifttt");
      String d = URLEncode(getDateTimeNow());
      String s = String("GET /trigger/")+URLEncode(event)+"/with/key/"+key+"?value1=" + URLEncode(value1) + "&value2=" + URLEncode(value2) + "&value3=" + URLEncode(value3) + " HTTP/1.1\r\n";
      DebugOut.print(s);
      client.print(s);
      client.print("Host: maker.ifttt.com\r\n");
      client.print("Connection: close\r\n");     
      client.print("Accept: */*\r\n");   
      client.print("User-Agent: Mozilla/4.0 (compatible; esp8266 Lua; Windows NT 5.1)\r\n");   
      client.print("\r\n");
      DebugOut.println("Request has sent to ifttt");
    } 
}
#else
void triggerIFTTT(String event, String value1, String value2, String value3){
    String key = "uoVHdqccPNfLG8UbUR6Al";
    String url = String("http://maker.ifttt.com/trigger/")+URLEncode(event)+"/with/key/"+key
       + "?value1=" + URLEncode(value1) + "&value2=" + URLEncode(value2) + "&value3=" + URLEncode(value3) 
       + " HTTP/1.1\r\n";

    String resp = HttpGet(url.c_str());
    DebugOut.println("response: "+resp);
}
#endif

#endif

//***** Ubidots *****************************************************************

#ifndef MYLIB_ARDUINO

void triggerUbidots(String device, String json){
    String token = "vWnTu7m17QzeJ5VfCumlucxWkHUKdo"; // user key
    String url = String("http://things.ubidots.com/api/v1.6/devices/")+URLEncode(device)+"/?token="+token;
    //DebugOut.println(url+json);

    HTTPClient http;
    http.begin(url.c_str());
    http.addHeader("Content-Type","application/json",false, false);
    int httpCode = http.POST(json);
    if(httpCode > 0) {
        DebugOut.printf("code: %d\n", httpCode);
        if(httpCode == HTTP_CODE_OK) {
          DebugOut.println(http.getString());
        }
    } else {
        DebugOut.printf("failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

#endif

//***** FS refresh *****************************************************************

#ifndef MYLIB_ARDUINO

String refreshFS(String tmpDir){
  tmpDir = "disk1/share/sadakane/FTP/refresh";
  DebugOut.setToNull();
  //DebugOut.setToFile("/ftplog.txt");
  String log = "";
  String files = "";
  int status = 1;

  FTPClient ftp;
  log = log + "\r\nftpopen: " + ftp.open("192.168.1.100", "sadakane", "gorosan4252karatani");
  delay(0);
  log = log + "\r\nftpcd:   " + ftp.cd(tmpDir);
  delay(0);
  Dir dir = SPIFFS.openDir("/");
  delay(0);
  while(dir.next()){
    File entry = dir.openFile("r");
    String fname = entry.name();
    files += fname + "&";
    delay(0);
    int s = ftp.put(fname);
    status *= s;
    log = log + "\r\nftpput: " + fname + " " + s;
    delay(0);
    entry.close();
    delay(0);
  }

  if( status == 0 ) return log + " FAIL";

  bool r = SPIFFS.format();
  log = log + "\r\nformat: " + r;
  delay(0);

  while( files != "" ) {
    String file = files.substring(0,files.indexOf("&"));
    files.replace(file+"&","");
    int s = ftp.get(file);
    delay(0);
    status *= s;
    log = log + "\r\nftpget: " + file + " " + s;
  }

  log = log + "\r\nftpbye: " + ftp.bye();
  delay(0);

  return log;
}



// **** HTTP, ETC *************************************************************************************

String HttpGet(const char *url) {
        String payload = "";
        HTTPClient http;

        DebugOut.print(String("[HTTP] GET ")+url+ " -> ");
        //http.begin("https://192.168.1.12/test.html", "7a 9c f4 db 40 d3 62 5a 6e 21 bc 5c cc 66 c8 3e a1 45 59 38"); //HTTPS
        http.begin(url); //HTTP
        int httpCode = http.GET();

        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            DebugOut.printf("code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                payload = http.getString();
            }
        } else {
            DebugOut.printf("failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
        return payload;
}

// Wake-on-LAN packet
void sendWoLtoToshiyukiPC(){
    byte m[] = { 0xCC, 0x52, 0xAF, 0x4A, 0xEF, 0xFA };
    sendWoL(m);
}

void sendWoL(byte *mac){
  // get target broadcast IP address
  if( WiFi.getMode() != WIFI_STA && WiFi.getMode() != WIFI_AP_STA ) 
    DebugOut.println("Warn: magic packet may be sent to wrong IP address...");
  IPAddress msk = WiFi.subnetMask(); // valid only for STA mode
  IPAddress gw  = WiFi.gatewayIP();  // valid only for STA mode
  IPAddress tgt = IPAddress(gw[0]|(~msk[0]), gw[1]|(~msk[1]), gw[2]|(~msk[2]), gw[3]|(~msk[3]));
  // send UDP magic packet
  WiFiUDP UDP;
  UDP.begin(9); //start UDP client, not sure if really necessary.
  WakeOnLan::sendWOL(tgt, UDP, mac, 6 /*sizeof mac*/);
  UDP.stop();
  DebugOut.printf("sent WoL magic packet to %0X:%0X:%0X:%0X:%0X:%0X, by ", 
                  mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  DebugOut.println(tgt);
}

String macAddress2String(uint8_t* macaddr) {
  String out = "";
  for (int i = 0; i < 6; i++) {
    if( macaddr[i] < 8 ) out += "0";
    out += String(macaddr[i], HEX);
    if (i < 5) out +=":";
  }
  return out;
}




//***** ESP-Now ****************************************************************
#ifndef MYLIB_ARDUINO

// EspNowBufferClass
//    buffer for esp-now packet received

EspNowBufferClass espNowBuffer;

#define COPY_ARRAY(SRC, DST, LEN) { for(int i=0; i<(LEN); i++){ DST[i]=SRC[i]; } }

bool EspNowBufferClass::store(uint8_t *mac, uint8_t *data, uint8_t len) {
  if( ESPNOW_IS_ACK_PCK(data) ) {
    int idx = (recvAckNum) % ESPNOW_BUFFER_SIZE; // ring buffer
    COPY_ARRAY(mac,  recvAck[idx].mac, 6);
    COPY_ARRAY(data, recvAck[idx].data, len);
    recvAck[idx].espNo = -1;
    recvAckNum++;
    if( recvAckNum - 1 >= ESPNOW_BUFFER_SIZE ) {
      log +=  "Error: espnow store buffer overflow\r\n";
      return false;
    }
  } else if( ESPNOW_IS_REQ_PCK(data) ) {
    int idx = (recvReqNum) % ESPNOW_BUFFER_SIZE; // ring buffer
    COPY_ARRAY(mac,  recvReq[idx].mac, 6);
    COPY_ARRAY(data, recvReq[idx].data, len);
    recvReq[idx].espNo = -1;
    recvReqNum++;
    if( recvReqNum - 1 >= ESPNOW_BUFFER_SIZE ) {
      log +=  "Error: espnow store buffer overflow\r\n";
      return false;
    }
  } else if( ESPNOW_IS_DAT_PCK(data) ) { // data packet
    int idx = (recvDataNum) % ESPNOW_BUFFER_SIZE; // ring buffer
    COPY_ARRAY(mac,  recvData[idx].mac, 6);
    COPY_ARRAY(data, recvData[idx].data, len);
    recvData[idx].len = len;
    recvData[idx].espNo = -1;
    recvDataNum++;
    if( recvDataNum - 1 >= ESPNOW_BUFFER_SIZE ) {
      log +=  "Error: espnow store buffer overflow\r\n";
      return false;
    }
  }
  return true;
}

// re-action for request
void EspNowBufferClass::processAllReq(void (*reqReaction)(int)){
  for (int i = 0; i < recvReqBufferMax(); i++ ) { // for each request in buffer
    (*reqReaction)(i);
  }
  recvReqNum = 0; // clear req buffer
}


// extract data from data packet
String EspNowBufferClass::getDataFromDataBuffer(int i){
  char buf[251]; // max payload + '\0'
  memcpy(buf, recvData[i].data, recvData[i].len);
  buf[recvData[i].len] = '\0';
  return String(buf+4); // ommit 4-byte header
}


// --- ESPNOW call backs ----

// !!! NEVER call delay()/yield() in call back functions

// default send callback
//   -- store log to espNowBuffer
void  default_send_cb(uint8_t* macaddr, uint8_t status) {
  String log = "";
  log += "send_cb\r\n";
  if( status != 0 ) log += "Error: send error\r\n";
  // debug
  log += " mac address: " + macAddress2String(macaddr) + "\r\n";
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
  log += " mac address: " + macAddress2String(macaddr) + "\r\n";
  log += " data: "+ sprintEspNowData(data,len) + "\r\n";
  // send ack
  if( ESPNOW_IS_REQ_PCK(data) || ESPNOW_IS_DAT_PCK(data) ) {
    log += "send ack...\r\n";
    uint8_t ack[] = { 0xFF, 0xFF, 0x0, 0x0 };
    ack[3] = data[3];
    esp_now_send(macaddr, ack, 4); // ack
  }
  // log
  espNowBuffer.log += log;
  // store
  espNowBuffer.store(macaddr, data, len);
}


// setup ESP NOW
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


// send string as ESP NOW data packet (waiting ack)
bool sendEspNow(uint8_t *mac, String message, uint8_t dataType) {
  char buf[250] = ESPNOW_DAT_DATA; // 250=max payload
  buf[3] = dataType;
  int len = message.length() > 250-4 ? 250-4 : message.length(); // data length + 4(header) <= 250 bytes
  message.toCharArray(buf+4, len);
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
  // send
  int ackType = data[3];
  espNowBuffer.recvAckNum = 0; // clear all in ack buffer. ack should be checked only in this function
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


// loop function for espnow controller
//   - call userFunc() for each conf["numPoll"] times
//   - send polling request packet to slave slaveMac[]
//   - call reqReaction(req) for each req in espnowBuffer
//   - restart if conf["mode"] == "STA", to change mode
//   - sleep or deepSleep at the end of this function (sleep time = conf["interval"]/conf["numPoll"])
//   * config save to RTC memory (setup() should read config from RTC memory)
void loopEspnowController(void (*userFunc)(), void (*reqReaction)(int), uint8_t *slaveMac ) {
  JsonObject &conf = jsonConfig.obj();
  // increment counter
  int cnt = conf["cntDSleep"] ? conf["cntDSleep"] : 0;
  conf["cntDSleep"] = cnt + 1;
  jsonConfig.saveRtcMem(); //  use RTC memory

  // do userFunc
  int numPoll = conf["numPoll"];
  if ( cnt % numPoll == 0 ) {
    (*userFunc)();
  }

  // send polling req
  uint8_t pollReq[] = ESPNOW_REQ_POLL;
  sendEspNow(slaveMac, pollReq, 4);
  delay(500); // wait poll actions

  // re-action for request
  espNowBuffer.processAllReq(reqReaction);

  // change mode
  if ( conf["mode"] == "STA" ) {
    jsonConfig.saveRtcMem(); //  use RTC memory
    jsonConfig.save(); // for safety
    SPIFFS.end();
    ESP.restart();
  }
  // delay/sleep
  long intv = conf["interval"];
  if ( conf["mode"] == String("EspNow") ) { // no deep sleep
    delay(intv / numPoll);
  } else { // deep sleep
    DebugOut.println("entering deep sleep...");
    SPIFFS.end();
    ESP.deepSleep(intv * 1000 / numPoll, WAKE_RF_DEFAULT); // connect GPIO16 to RSTB
    delay(1000); // wait for getting sleep
  }
}


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

#endif // #ifndef MYLIB_ARDUINO

//***** Json Config ****************************************************************
// use ArduinoJson

JsonConfig jsonConfig;

bool JsonConfig::load() {
  const char empty[] = "{}"; // must be const, so that jsonBuffer will allocate copy of the string.

  if( !SPIFFS.begin() ) DebugOut.println("Error: fail to mount SPIFFS");

  if( buffer ) delete buffer; // for multiple call of this function

  // open file
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    DebugOut.println("Failed to open config file. Set empty json data.");
    buffer = new JsonConfigBuffer(empty);
    return false;
  }
  // check memory
  size_t size = configFile.size();
  if ( size > JSON_CONFIG_BUF_SIZE - JSON_CONFIG_BUF_SLACK_SIZE ) {
    DebugOut.println("Config file size is too large. Set empty json data.");
    buffer = new JsonConfigBuffer(empty);
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  //sada: I think String is ok since in that case, jsonBuffer will allocate copy of string internally.

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  buffer = new JsonConfigBuffer(buf.get());

  if (!buffer->json.success()) {
    DebugOut.println("Failed to parse config file. Set empty json data.");
    delete buffer;
    buffer = new JsonConfigBuffer(empty);
    return false;
  }

  DebugOut.print("load json: "); buffer->json.printTo(DebugOut); DebugOut.println("");

  return true;
}

bool JsonConfig::loadRtcMem() {
  const char empty[] = "{}"; // must be const, so that jsonBuffer will allocate copy of the string.

  if( buffer ) delete buffer; // for multiple call of this function

  String mem = ESP_rtcUserMemoryRead();

  // check memory
  if ( mem.length() > JSON_CONFIG_BUF_SIZE - JSON_CONFIG_BUF_SLACK_SIZE ) {
    DebugOut.println("Json read size is too large. Set empty json data.");
    buffer = new JsonConfigBuffer(empty);
    return false;
  }

  buffer = new JsonConfigBuffer(mem.c_str());

  if (!buffer->json.success()) {
    DebugOut.println("Failed to parse config file. Set empty json data.");
    delete buffer;
    buffer = new JsonConfigBuffer(empty);
    return false;
  }

  DebugOut.print("load json from rtcmem: "); buffer->json.printTo(DebugOut); DebugOut.println("");

  return true;
}


bool JsonConfig::save() { // TODO: if not changed, not save (for flash lifetime)
  if( ! buffer ) {
    DebugOut.println("jsonConfig: save() called before load()");
    return false;
  }
  DebugOut.print("save json: "); jsonConfig.obj().printTo(DebugOut); DebugOut.println("");

  DebugOut.println(String("json buffer size: ")+buffer->jsonBuffer.size());

  if( remainedCapacity() < JSON_CONFIG_BUF_SLACK_SIZE ) {
    DebugOut.println("ERROR: json buffer is almost full. Save empty.");
    DebugOut.println(buffer->jsonBuffer.capacity());
    DebugOut.println(buffer->jsonBuffer.size());
    fileCopy("/config.json","/config.json.bac");
    fileDelete("/config.json");
    fileAppend("/config.json","{\"SaveError\":0}");
    return false;
  }

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    DebugOut.println("Failed to open config file for writing.");
    return false;
  }

  jsonConfig.obj().printTo(configFile);
  configFile.close();

  return true;
}


bool JsonConfig::saveRtcMem() {
  String err = "{\"SaveError\":0}";

  if( ! buffer ) {
    DebugOut.println("jsonConfig: save() called before load()");
    return false;
  }
  DebugOut.print("save json to rtcmem: "); jsonConfig.obj().printTo(DebugOut); DebugOut.println("");

  DebugOut.println(String("json buffer size: ")+buffer->jsonBuffer.size());

  if( remainedCapacity() < JSON_CONFIG_BUF_SLACK_SIZE ) {
    DebugOut.println("ERROR: json buffer is almost full. Save empty.");
    DebugOut.println(buffer->jsonBuffer.capacity());
    if( ! ESP_rtcUserMemoryWrite(err) )
      DebugOut.println("Error: fail to write RTC mem.");
    return false;
  }

  String mem;
  jsonConfig.obj().printTo(mem);

  if( mem.length() > 512-1 ) {
    DebugOut.println("Error: json size is too long to write to RTC mem. save empty.");
    if( ! ESP_rtcUserMemoryWrite(err) )
      DebugOut.println("Error: fail to write RTC mem.");
    return false;
  }
  if( ! ESP_rtcUserMemoryWrite(mem) ) {
    DebugOut.println("Error: fail to write RTC mem.");
    return false;
  }
  return true;
}


//**************** IR Remote ESP8266 ***********************************************

#ifndef MYLIB_ARDUINO

IRsend::IRsend(int IRsendPin) {
	IRpin = IRsendPin;
}

void IRsend::begin() {
	pinMode(IRpin, OUTPUT);
}

void IRsend::enableIROut(int khz) {
  // Enables IR output.  The khz value controls the modulation frequency in kilohertz.
  carrierLowTime =  666/khz;
  carrierHighTime = 333/khz;
}

void IRsend::enableIROut(int cLowTime, int cHighTime) {
  // Enables IR output.  The khz value controls the modulation frequency in kilohertz.
  carrierLowTime =  cLowTime;
  carrierHighTime = cHighTime;
}

void IRsend::sendRaw(unsigned int buf[], int len, int hz)
{
  enableIROut(hz);
  sendRaw(buf, len);
}

void IRsend::sendRaw(unsigned int buf[], int len)
{
  // debug print
  DebugOut.print(String("sendRaw: ")+carrierLowTime + "/" + carrierHighTime + " : ");
  for (int i = 0; i < len; i++) {
    DebugOut.print((int)(buf[i]*pulseTimeMult));
    DebugOut.print(" ");
  }
  DebugOut.println();

  for (int i = 0; i < len; i++) {
    if (i & 1) {
      space((int)(buf[i]*pulseTimeMult));
    } 
    else {
      mark((int)(buf[i]*pulseTimeMult));
    }
  }
  space(0); // set Low at the end

}

void IRsend::mark(int time) {
  // Sends an IR mark for the specified number of microseconds.
  // The mark output is modulated at the PWM frequency.
  long beginning = micros();
  while(micros() - beginning < time){
    digitalWrite(IRpin, HIGH);
    delayMicroseconds(carrierHighTime);
    digitalWrite(IRpin, LOW);
    delayMicroseconds(carrierLowTime);
  }
}

/* Leave pin off for time (given in microseconds) */
void IRsend::space(int time) {
  // Sends an IR space for the specified number of microseconds.
  // A space is no output, so the PWM output is disabled.
  digitalWrite(IRpin, LOW);
  if (time > 0) delayMicroseconds(time);
}

#endif


#endif

