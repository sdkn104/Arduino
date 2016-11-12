
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
#include <FS.h>

#include <MyLib.h>


extern ESP8266WebServer server;
extern String customHtml;

void addMyCockpit(const char* uri, ESP8266WebServer::THandlerFunction handler);
void addMyCockpit(const char* uri, int argnum, ESP8266WebServer::THandlerFunction handler);
void addHtmlMyCockpit(String html);
void setupMyCockpit();
void loopMyCockpit();

