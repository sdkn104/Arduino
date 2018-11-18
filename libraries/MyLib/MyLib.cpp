
#include <MyLib.h>

#ifdef MYLIB_ESP8266
extern "C" {
#include <user_interface.h> // for sleep mode
}
#include <ESP8266HTTPClient.h>
#include <WakeOnLan.h>
#endif

// **** MAC Address ******************************************************************************
#ifdef MYLIB_ESP8266

uint8_t macAddrSTA[numMacAddr][6] = {
 {0,0,0,0,0,0}, // No.0
 {0x18,0xFE,0x34,0xD5,0xC7,0xA7}, // No.1
 {0,0,0,0,0,0},
 {0x5C,0xCF,0x7F,0x17,0xB0,0x99}, // No.3
 {0x5C,0xCF,0x7F,0x17,0xC0,0xB2}, // No.4
 {0x5C,0xCF,0x7F,0x17,0xB6,0x1F}, // No.5
 {0x5C,0xCF,0x7F,0x16,0xDE,0x9F}, // No.6
 {0x5C,0xCF,0x7F,0xD6,0x50,0xB4}, // No.7
 {0x5C,0xCF,0x7F,0xD6,0x4F,0x0D}, // No.8
 {0x5C,0xCF,0x7F,0x2C,0x83,0x9E}, // No.9
 {0x5C,0xCF,0x7F,0x90,0xB6,0x36}, // No.10
 {0xA0,0x20,0xA6,0x0A,0xC8,0xAF}  // No.11
};

uint8_t macAddrAP[numMacAddr][6] = {
 {0,0,0,0,0,0}, // No.0
 {0x1A,0xFE,0x34,0xD5,0xC7,0xA7}, // No.1
 {0,0,0,0,0,0},
 {0x5E,0xCF,0x7F,0x17,0xB0,0x99}, // No.3
 {0x5E,0xCF,0x7F,0x17,0xC0,0xB2}, // No.4
 {0x5E,0xCF,0x7F,0x17,0xB6,0x1F}, // No.5
 {0x5E,0xCF,0x7F,0x16,0xDE,0x9F}, // No.6
 {0x5E,0xCF,0x7F,0xD6,0x50,0xB4}, // No.7
 {0x5E,0xCF,0x7F,0xD6,0x4F,0x0D}, // No.8
 {0x5E,0xCF,0x7F,0x2C,0x83,0x9E}, // No.9
 {0x5E,0xCF,0x7F,0x90,0xB6,0x36}, // No.10
 {0xA2,0x20,0xA6,0x0A,0xC8,0xAF}  // No.11
};

int getIdOfMacAddrSTA(uint8_t *mac) {
  for(int id=0; id<numMacAddr; id++){
    if( macAddress2String(mac) == macAddress2String(macAddrSTA[id]) )
      return id;
  }
  return -1;
}
int getIdOfMacAddrAP(uint8_t *mac) {
  for(int id=0; id<numMacAddr; id++){
    if( macAddress2String(mac) == macAddress2String(macAddrAP[id]) )
      return id;
  }
  return -1;
}

String macAddress2String(uint8_t* macaddr) {
#ifdef OLDxxxx
  String out = "";
  for (int i = 0; i < 6; i++) {
    if( macaddr[i] < 8 ) out += "0";
    out += String(macaddr[i], HEX);
    if (i < 5) out +=":";
  }
  return out;
#else
  char s[18];
  sprintf(s,"%02X:%02X:%02X:%02X:%02X:%02X",macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
  return String(s);
#endif
}

/*
uint8_t *macAddr2Arr(String mac) {
  static uint8_t m[6];
  m[0] = mac.substring(0,1).toInt();
  m[1] = mac.substring(3,4).toInt();
  m[2] = mac.substring(6,7).toInt();
  m[3] = mac.substring(9,10).toInt();
  m[4] = mac.substring(12,13).toInt();
  m[5] = mac.substring(15,16).toInt();
  return m;
}
*/

#endif // MYLIB_ESP8266

// **** Utils *************************************************************************************

#ifdef MYLIB_ESP8266

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

#endif

// **** RTC User memory (ESP8266) **********************************************************
#ifdef MYLIB_ESP8266

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

#endif

// **** DDNS *************************************************************************************
#ifdef MYLIB_ESP8266

void updateDDNS(){
  //String addr = "http://dynupdate.no-ip.com/nic/update?hostname=sdkn104.hopto.org";
  //  http://sdkn104:gorosan@dynupdate.no-ip.com/nic/update?hostname=sdkn104.hopto.org
  HTTPClient http;
  http.begin("http://dynupdate.no-ip.com/nic/update?hostname=sdkn104.hopto.org");
  http.setAuthorization(PRIVATE_DDNS_ID, PRIVATE_DDNS_PASS);
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

String thisSketch;

String setThisSketch(const char *src, const char *date, const char *time){
  String the_path = src;
  int slash_loc = the_path.lastIndexOf('\\');
  String out = the_path.substring(slash_loc+1);
  thisSketch = out + " (compiled on: " + date + " at " + time + ")";
  return thisSketch;
}

String getThisSketch(){
  return thisSketch;
}


#ifdef MYLIB_ESP8266

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
  o += String("analogRead(A0): ") + analogRead(A0)
     + String("\r\ngetVcc: ") + ESP.getVcc() + "  " + ESP.getVcc()/1024.0 + "V  -- valid only if TOUT pin is open and ADC_MODE(ADC_VCC) called"
     + "\r\ndigitalRead(0): " + String(digitalRead(0))
     + "\r\ncurrent time: " + getDateTimeNow();
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

// unix time to UTC  string in ISO format
String getDateTimeISOUTC(time_t tm){
  char s[20];
  const char* format = "%04d-%02d-%02dT%02d:%02d:%02dZ";
  time_t t = tm; // UTC
  sprintf(s, format, year(t), month(t), day(t), hour(t), minute(t), second(t));
  return String(s);
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

#ifdef MYLIB_ESP8266

bool WiFiConnect() {
  return WiFiConnect(0);
}

bool WiFiConnect(int id) {
  const char* ssid = PRIVATE_WIFI_SSID;
  const char* password = PRIVATE_WIFI_PASS; // wifi password
  return WiFiConnect(ssid, password, id);
}

bool WiFiConnect(const char *ssid, const char *password) {
  return WiFiConnect(ssid, password, 0);
}

bool WiFiConnect(const char *ssid, const char *password, int id) {
  DebugOut.print("connecting to WiFi ");
  WiFi.mode(WIFI_STA); // default: WIFI_AP_STA, or load from flash (persistent function)
  WiFi.begin(ssid, password);
  if( id > 0 ) { // static IP address
    IPAddress staticIP(192,168,1,100+id);
    IPAddress gateway(192,168,1,1);
    IPAddress subnet(255,255,255,0);
    WiFi.config(staticIP, gateway, subnet);
  }
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

#ifdef MYLIB_ESP8266

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
    uint8_t c = f.read();  // TODO: buffering?
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

#ifdef MYLIB_ESP8266

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

String jsonFileList(const char *path) {  
  Dir dir = SPIFFS.openDir(path);
  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name());
    output += "\",\"size\":";
    output += String(entry.size());
    output += "}";
    entry.close();
  }
  output += "]";
  return output;
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
#ifdef MYLIB_ESP8266
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

#ifdef MYLIB_ESP8266

void triggerIFTTT(String event, String value1, String value2, String value3){
    DebugOut.println("triggerIFTTT: "+event);
    String key = PRIVATE_IFTTT_KEY;
    String url = String("http://maker.ifttt.com/trigger/")+URLEncode(event)+"/with/key/"+key
       + "?value1=" + URLEncode(value1) + "&value2=" + URLEncode(value2) + "&value3=" + URLEncode(value3) 
       + " HTTP/1.1\r\n";

    String resp = HttpGet(url.c_str());
    DebugOut.println(" response: "+resp);
}

#endif

//***** Ubidots *****************************************************************

#ifdef MYLIB_ESP8266

void triggerUbidots(String device, String json){
    String token = PRIVATE_UBIDOTS_TOKEN; // user key
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

//***** M2X *****************************************************************

void triggerM2X(String device, String stream, String json){
    String token = "986af386e86d7e7de57e3bd3d28d79c4"; //PRIVATE_UBIDOTS_TOKEN; // master user key
    String deviceID = device == "espnow3" ? "2a88b48ac300692cc51fa8fc9fb61574" :
                      device == "espnow5" ? "32ac4bbc890c28cff300c27320e5f956" :
                      device == "basic"   ? "680e9954ce06e550afe900116f3e264d" : "";

    String url = "http://api-m2x.att.com/v2/devices/"+deviceID+"/streams/"+stream+"/value";
//    String url = "http://api-m2x.att.com/v2/devices/"+deviceID;
    DebugOut.println(String("triggerM2X:")+url+json);

    HTTPClient http;
    http.begin(url.c_str());
    http.addHeader("Content-Type","application/json",false, false);
    http.addHeader("X-M2X-KEY",token,false, false);
    //int httpCode = http.PUT(json); // not available yet in arduino/esp8266 2.3.0
    int httpCode = http.sendRequest("PUT",json);
    //int httpCode = http.GET();

    if(httpCode > 0) {
        DebugOut.printf(" code: %d\n", httpCode);
        if(httpCode == HTTP_CODE_OK) {
          DebugOut.println(http.getString());
        }
    } else {
        DebugOut.printf(" failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

//***** BigQuery *****************************************************************

int triggerBigQuery(String table, String value1, String value2, String value3, String value4){
    //String url = String("http://192.168.1.203/cgi-bin/bqInsertRcvTable.py?table=")+ URLEncode(table)
    //  + "&value1=" + URLEncode(value1)+"&value2="+URLEncode(value2) + "&value3=" + URLEncode(value3)
    //  + "&value4=" + URLEncode(value4);
    String url = String("http://")+PRIVATE_GCP_PROJECT_ID+".appspot.com/"+PRIVATE_GCP_PROJECT_APP
      +"/insertBQ?table="+ URLEncode(table)
      + "&value1=" + URLEncode(value1)+"&value2="+URLEncode(value2) + "&value3=" + URLEncode(value3) 
      + "&value4=" + URLEncode(value4);

    DebugOut.println(String("triggerBigQuery:")+url);

    int code;
    String resp = HttpGet(url.c_str(), &code);
    DebugOut.println(" response: "+resp);
    DebugOut.println(" status code: "+code);
    return code;
}

#endif

//***** Trigger GAS *****************************************************************

// failed. cannot access https ???
void triggerGASGmail(String subject, String body) {
  String url = "https://script.google.com/macros/s/AKfycbw4q2y_m6_-XpgpP6krw6lwVkuSdpZDd0yeMkIvqsGmmyQxzJg/exec?subject="+subject+"&body="+body;
  String resp = HttpGet(url.c_str());
  DebugOut.println("response: "+resp);
}

//***** Trigger send Gmail with OrangePI *****************************************************************

void triggerSendGmail(String subject, String body) {
  DebugOut.println("triggerSendGmail: "+subject);
  String url = String("http://orangepione.sada.org/cgi-bin/sendgmail.py?subject=") + URLEncode(subject) +
               "&body="+URLEncode(body);
  String resp = HttpGet(url.c_str());
  DebugOut.println(" response: "+resp);
}

//***** FS refresh *****************************************************************

#ifdef MYLIB_ESP8266

String refreshFS(String tmpDir){
  tmpDir = "disk1/share/sadakane/FTP/refresh";
  DebugOut.setToNull();
  //DebugOut.setToFile("/ftplog.txt");
  String log = "";
  String files = "";
  int status = 1;

  FTPClient ftp;
  log = log + "\r\nftpopen: " + ftp.open(PRIVATE_FTP_ADDR, PRIVATE_FTP_ID, PRIVATE_FTP_PASS);
  delay(0);
  log = log + "\r\nftpcd:   " + ftp.cd(tmpDir);
  delay(0);
  Dir dir = SPIFFS.openDir("/");
  delay(0);
  while(dir.next()){
    File entry = dir.openFile("r");
    String fname = entry.name();
    files += fname + "|";
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
    int e = files.indexOf("|");
    if( e < 0 ) break;
    String file = files.substring(0,e);
    files.replace(file+"|","");
    int s = ftp.get(file);
    delay(0);
    status *= s;
    log = log + "\r\nftpget: " + file + " " + s;
  }

  log = log + "\r\nftpbye: " + ftp.bye();
  delay(0);

  DebugOut.setToPrevious();
  return log;
}

// fileNames : file names delimited by "|". it must end with "|"
String ftpGetInitFiles(String ftpDir, String fileNames){
  ftpDir = "disk1/share/sadakane/FTP";
  //DebugOut.setToNull();
  //DebugOut.setToFile("/ftplog.txt");
  String log = "";
  int status = 1;

  FTPClient ftp;
  log = log + "\r\nftpopen: " + ftp.open(PRIVATE_FTP_ADDR, PRIVATE_FTP_ID, PRIVATE_FTP_PASS);
  delay(0);
  log = log + "\r\nftpcd:   " + ftp.cd(ftpDir);
  delay(0);

  while( fileNames != "" ) {
    int e = fileNames.indexOf("|");
    if( e < 0 ) break;
    String file = fileNames.substring(0,e);
    fileNames.replace(file+"|","");
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

String HttpGet(const char *url, int *code) { // code is optional, default is declared in xxx.h
        String payload = "";
        HTTPClient http;

        DebugOut.print(String("[HTTP] GET ")+url+ " -> ");
        //http.begin("https://192.168.1.12/test.html", "7a 9c f4 db 40 d3 62 5a 6e 21 bc 5c cc 66 c8 3e a1 45 59 38"); //HTTPS
        http.begin(url); //HTTP
        int httpCode = http.GET();
        if(code) *code = httpCode;

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



//***** ESP-Now ****************************************************************
#ifdef MYLIB_ESP8266

#include <espnowLib.h>

// loop function for espnow controller
//   - call userFunc() in interval conf["interval"]
//   - send polling request packet to slave slaveMac[] in interval conf["interval"]/conf["numPoll"]
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
  sendEspNowReq(slaveMac, enPOLL);
  delay(500); // wait poll actions

  // re-action for request
  espNowBuffer.processAllReq(reqReaction);

  // change mode
  if ( conf["mode"] == "STA" ) {
    jsonConfig.save();
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

#endif //MYLIB_ARDUINO

//***** Json Config ****************************************************************

// use ArduinoJson

JsonConfig jsonConfig;

bool JsonConfig::load() {
  if ( ESP_rtcUserMemoryRead().startsWith("{") && jsonConfig.loadRtcMem() ) {
    ESP_rtcUserMemoryWrite(""); // delete
    return true;
  } else {
    return jsonConfig.loadFile();
  }
}


bool JsonConfig::loadFile() {
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


bool JsonConfig::save() {
  bool r1 = saveFile();
  bool r2 = saveRtcMem();
  return r1 && r2;
}

bool JsonConfig::saveFile() { // TODO: if not changed, not save (for flash lifetime)
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

#ifdef MYLIB_ESP8266

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

//**************** Alive Check ***********************************************

#ifdef MYLIB_ESP8266

void AliveCheck::init() {
  time_t nw = getNow();
  for(int i=0; i<AliveCheckDeviceNum; i++) {
    data[i].lastAliveTime = nw;
    data[i].enable = false;
  }
  // set devId = macAddrId for ESP8266 Devices
  data[5].enable = false;
  data[5].name = "Espnow5";
  data[5].timeout = 60*20; // in seconds
  data[7].enable = false;
  data[7].name = "Thermo";
  data[7].timeout = 60*20; // in seconds
  data[9].enable = false;
  data[9].name = "IRremote";
  data[9].timeout = 60*60; // in seconds
  data[10].enable = false;
  data[10].name = "Espnow3";
  data[10].timeout = 60*20; // in seconds
  data[0].enable = false;
  data[0].name = "OrangePi";
  data[0].timeout = 60*20; // in seconds
}

void AliveCheck::registerAlive(int devId) { // register alive signal get from the device of the id
  if( devId >= AliveCheckDeviceNum ) return;
  data[devId].lastAliveTime = getNow();
}

bool AliveCheck::checkAlive(){                // check alive for all ids
  String log_all = "";
  bool res = true;
  for(int i=0; i<AliveCheckDeviceNum; i++){
    if( data[i].enable ) {
      res = checkAlive(i) && res;
      log_all += log + "\r\n";
    }
  }
  log = log_all;
  return res;
}

bool AliveCheck::checkAlive(int devId) {          // check alive for the device id
  log = "";
  if( devId >= AliveCheckDeviceNum ) return false;
  time_t nw = getNow();
  time_t lat = data[devId].lastAliveTime;  
  log = String("ID=")+devId+"("+data[devId].name+") "+getDateTime(nw)+" - "+getDateTime(lat)+" vs "+data[devId].timeout;
  if( data[devId].enable == false ) return true;
  if( nw - data[devId].lastAliveTime <= data[devId].timeout ) return true;

  triggerIFTTT("NotAlive", data[devId].name, getDateTime(nw), getDateTime(data[devId].lastAliveTime));
  triggerSendGmail("NotAlive: "+data[devId].name, URLEncode(data[devId].name+" is not Alive at "+getDateTime(nw)+", last alive at "+getDateTime(data[devId].lastAliveTime)));
  return false;
}

#endif

//**************** EMAIL SMTP CLIENT ***********************************************

byte SmtpClient::sendEmail(String fromAddr, String toAddr, String subject, String body) {
  byte thisByte = 0;
  byte respCode;

  if (client.connect(server.c_str(), port) == 1) {
    DebugOut.println(F("SMTP connected"));
  } else {
    DebugOut.println(F("SMTP connection failed"));
    return 0;
  }
  if (!eRcv()) return 0;

  DebugOut.println(F("Sending EHLO"));
  client.println("EHLO www.example.com");
  if (!eRcv()) return 0;
  DebugOut.println(F("Sending auth login"));
  client.println("auth login");
  if (!eRcv()) return 0;
  DebugOut.println(F("Sending User"));
  // Change to your base64, ASCII encoded user
  client.println(user); //<---------User
  if (!eRcv()) return 0;
  DebugOut.println(F("Sending Password"));
  // change to your base64, ASCII encoded password
  client.println(pass);//<---------Passw
  if (!eRcv()) return 0;
  DebugOut.println(F("Sending From"));
  // change to your email address (sender)
  client.print(F("MAIL From: ")); client.println(fromAddr);
  if (!eRcv()) return 0;
  // change to recipient address
  DebugOut.println(F("Sending To"));
  client.print(F("RCPT To: ")); client.println(toAddr);
  if (!eRcv()) return 0;
  DebugOut.println(F("Sending DATA"));
  client.println(F("DATA"));
  if (!eRcv()) return 0;
  DebugOut.println(F("Sending email"));
  // change to recipient address
  client.print(F("To: ")); client.println(toAddr);
  // change to your address
  client.print(F("From: ")); client.println(fromAddr);
  client.print(F("Subject: ")); client.println(subject);
  client.println(body);

  client.println(F("."));
  if (!eRcv()) return 0;
  DebugOut.println(F("Sending QUIT"));
  client.println(F("QUIT"));
  if (!eRcv()) return 0;
  client.stop();
  DebugOut.println(F("disconnected"));
  return 1;
}

byte SmtpClient::eRcv() {
  byte respCode;
  byte thisByte;
  int loopCount = 0;

  while (!client.available()) {
    delay(1);
    loopCount++;
    // if nothing received for 10 seconds, timeout
    if (loopCount > 10000) {
      client.stop();
      DebugOut.println(F("\r\nTimeout"));
      return 0;
    }
  }

  respCode = client.peek();
  while (client.available())
  {
    thisByte = client.read();
    DebugOut.write(thisByte);
  }

  if (respCode >= '4')
  {
    //  efail();
    return 0;
  }
  return 1;
}



#endif

