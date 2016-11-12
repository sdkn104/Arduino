
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
  SPIFFS.info(fs_info);
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

// ***** Timer ********************************************************************************

CheckInterval::CheckInterval(unsigned long interval) {
  _interval = interval;
  _prev = -1;
  DebugOut.print("init interval ");
  DebugOut.println(interval);
}

bool CheckInterval::check() {
//  DebugOut.print("check interval ");
//  DebugOut.println(_interval);
//  DebugOut.println(_prev);

  unsigned long c = millis() / _interval;
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


// return current Unix Time (seconds from Unix Epoch 1970/1/1 0:00 GMT)
time_t getNow() {
  return now(); // based on Time.h
}


// unix time to local time string
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

  SPIFFS.begin(); // it is safe to duplicate call this ??
  File f = SPIFFS.open(String("/")+fileName, "w");//TODO: FS.h required???
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

  SPIFFS.begin(); // it is safe to duplicate call this ??
  File f = SPIFFS.open(String("/")+fileName, "r");//TODO: FS.h required???
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
  if(!file) return false;
  file.close();
  return true;
}

bool fileDelete(const char *path) {
  if( ! SPIFFS.exists(path) ) 
    return true;
  else
    return SPIFFS.remove(path);
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

#endif


// LogFile Class
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

size_t LogFile::write(uint8_t c) {
#ifdef MYLIB_ARDUINO
  Serial.write(c);
#else
  if( _size >= _maxSize ) {
    String fd = String(_fileName) + "_prev";
    if( SPIFFS.exists(fd) ) { SPIFFS.remove(fd); }
    fileCopy(_fileName.c_str(), fd.c_str());
    SPIFFS.remove(_fileName);
    _size = 0;
  }

  File fs = SPIFFS.open(_fileName, "a");
  if(!fs){ 
      if( _fileName == DebugOut.FileName() ) {
        Serial.println(String("ERROR: fail to open file: ")+_fileName); // dont use DebugOut, avoid recursive loop
      } else {
        DebugOut.println(String("ERROR: fail to open file: ")+_fileName);
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
    write(buffer[i]);
  }
  return size;
}




//***** DebugOut *****************************************************************


DebugOutClass DebugOut;


size_t DebugOutClass::write(uint8_t c) {
  if(_type == 0 ) { return Serial.print((char)c); }
  else if( _type == 1 ) { return 0; }
#ifndef MYLIB_ARDUINO
  else if( _type == 2 ) { SPIFFS.begin(); return _logFile.write(c); }
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
  log = log + "\r\nftpopen: " + ftp.open("192.168.1.100", "admin", "password");
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

// TODO: fix value is ok?
#define WIFI_DEFAULT_CHANNEL 1

// buffer for esp-now packet received
EspNowStatus espNowStatus;

// default send callback
void  default_send_cb(uint8_t* macaddr, uint8_t status) {
  DebugOut.println("send_cb");
  // store
  espNowStatus.send_cb = true;
  espNowStatus.sendStatus = status;
  // debug
  DebugOut.println(" mac address: " + macAddress2String(macaddr));
  DebugOut.println(" status = " + String(status)); //0:success
}

// default receive call back
void default_recv_cb(uint8_t *macaddr, uint8_t *data, uint8_t len) {
  DebugOut.println("recv_cb");
  // debug
  DebugOut.println(" mac address: " + macAddress2String(macaddr));
  DebugOut.println(" data: "+ sprintEspNowData(data,len));
  // store
  espNowStatus.recv_cb = true;
  for (int i = 0; i < 6; i++) {
    espNowStatus.recvMac[i] = macaddr[i];
  }
  for (int i = 0; i < len; i++) {
    espNowStatus.recvData[i] = data[i];
  }
  espNowStatus.recvDataLen = len;
}

// receive callback for controller (implement wake up packet handling)
void cont_recv_cb(uint8_t *macaddr, uint8_t *data, uint8_t len) {
  DebugOut.println("recv_cb");
  // debug
  DebugOut.println(" mac address: " + macAddress2String(macaddr));
  DebugOut.println(" data: "+ sprintEspNowData(data,len));
  // wakeup
  if( data[0]==0xFF && data[1]==0xFF && len==2 ) { // wakeup
      DebugOut.println("get wakeup packet. exit esp-now mode...");
      //uint8_t wakeupAck[] = {0xFF, 0xFF, 0x1};
      //esp_now_send(NULL, wakeupAck, 3); // send wakeup ack
      jsonConfig.obj()["mode"] = "STA";
      jsonConfig.save();
      ESP.restart();
  }
  // store
  espNowStatus.recv_cb = true;
  for (int i = 0; i < 6; i++) {
    espNowStatus.recvMac[i] = macaddr[i];
  }
  for (int i = 0; i < len; i++) {
    espNowStatus.recvData[i] = data[i];
  }
  espNowStatus.recvDataLen = len;
}

// receive call back for slave (implement sending ack packet)
void slave_recv_cb(uint8_t *macaddr, uint8_t *data, uint8_t len) {
    DebugOut.println("recv_cb");
    // debug
    DebugOut.println(" mac address: " + macAddress2String(macaddr));
    DebugOut.println(" data: "+sprintEspNowData(data,len));

    if(data[0] == 0xFF && data[1] == 0xFF && len == 2 ) { // poll packet
      uint8_t wakeup[] = {0xFF, 0xFF};
      esp_now_send(NULL, wakeup, 2); // send wakeup
    } else { // data packet
      // store to file
      char buf[251]; // max payload + '\0'
      memcpy(buf, data, len);
      buf[len] = '\0';
      fileAppend("/espNowRcvData.txt", getDateTimeNow().c_str());
      fileAppend("/espNowRcvData.txt", ", ");
      fileAppend("/espNowRcvData.txt", buf);
      fileAppend("/espNowRcvData.txt", "\r\n");
      // reply
      DebugOut.println(" reply...");
      char ack[] = "ACK";
      esp_now_send(NULL, (uint8_t *)ack, 3);
    }
}

// setup ESP NOW.
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
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  r = esp_now_register_recv_cb(recv_cb);
  r = esp_now_register_send_cb(send_cb);
  r = esp_now_add_peer(mac, (uint8_t)ESP_NOW_ROLE_SLAVE, (uint8_t)WIFI_DEFAULT_CHANNEL, NULL, 0);
}


// send string thru ESP NOW
bool sendEspNow(String message) {
  sendEspNow((uint8_t *)message.c_str(), message.length());
}

// send data thru ESP NOW (waiting ack packet)
bool sendEspNow(uint8_t *message, int len) {
  DebugOut.println("send esp-now expecting reply...");
  // check
  if ( WiFi.isConnected() )
    DebugOut.println("error: WiFi is connected. esp-now send will fail.");
  DebugOut.println("data: "+ sprintEspNowData(message,len));
  // send
  espNowStatus.send_cb = false;
  espNowStatus.recv_cb = false;
  bool success = false;
  for (int retry = 0; retry < 3 && !success; retry++) {
    DebugOut.println("esp-now send trial #" + String(retry));
    esp_now_send(NULL, message, len);
    for (int i = 0; i < 30; i++) {
      if ( espNowStatus.send_cb == true ) {
        DebugOut.println("alredy sent");
      }
      if ( espNowStatus.recv_cb == true ) {
        DebugOut.println("received");
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

//***** Json Config ****************************************************************
// use ArduinoJson

JsonConfig jsonConfig;

bool JsonConfig::load() {
  const char empty[] = "{}"; // must be const, so that jsonBuffer will allocate copy of the string.

  SPIFFS.begin();

  // open file
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    DebugOut.println("Failed to open config file. Set empty json data.");
    buffer = new JsonConfigBuffer(empty);
    return true;
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
    buffer = new JsonConfigBuffer(empty);
    return false;
  }

  DebugOut.print("load json: "); buffer->json.printTo(DebugOut); DebugOut.println("");

  return true;
}


bool JsonConfig::save() {
  DebugOut.print("save json: "); jsonConfig.obj().printTo(DebugOut); DebugOut.println("");

  DebugOut.println(String("json buffer size: ")+buffer->jsonBuffer.size());

  if( remainedCapacity() < JSON_CONFIG_BUF_SLACK_SIZE ) {
    DebugOut.println("ERROR: json buffer is almost full. Save empty.");
    DebugOut.println(buffer->jsonBuffer.capacity());
    DebugOut.println(buffer->jsonBuffer.size());
    fileCopy("/config.json","/config.json.bac");
    fileDelete("/config.json");
    fileAppend("/config.json","{}");
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

#endif

