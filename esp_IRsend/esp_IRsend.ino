/*
   IRremote:
   An IR LED must be connected to pin GPIO4
*/

extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>

CheckInterval CI(1000 * 60 * 5);

ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open

//#include <IRremoteESP8266.h>

IRsend irsend(4); // send by GPIO 4 pin

#include "sharpAquosData.h"
#include "FujitsuAirConData.h"
const char **irName[] = { irName0, irName1 };
const size_t irSize[] = { sizeof(irName0), sizeof(irName1) };
const uint16_t *irData[] = { irData0, irData1 };
const int irLen[] = { irLen0, irLen1 };

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP......................");
  //DebugOut.setToFile();
  //Serial.setDebugOutput(true);
  //WiFi.printDiag(DebugOut);

  wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
  WiFiConnect();
  printSystemInfo();

  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  setupMyOTA();
  SET_THIS_SKETCH();
  
  addHtmlMyCockpit(IRSendHtml(0));
  addHtmlMyCockpit(IRSendHtml(1) + "<BR><BR>");

  addHtmlMyCockpit("<a href='/rimokon.htm'>rimokon.htm</a>");

  addMyCockpit("/IRremote", 2, []() {
    int i = server.arg(0).toInt();
    String n = server.arg(1);
    sendAnyDec(irData[i] + irLen[i] * n.toInt());
    server.send(200, "text/plain", String("type:")+i+", no:"+ n + ", ok");
  });
  addMyCockpit("/carrierLowHigh", 2, []() {
    String low  = server.arg(0);
    String high = server.arg(1);
    int carrierLow  = low.toInt();
    int carrierHigh = high.toInt();
    irsend.enableIROut(carrierLow, carrierHigh);
    server.send(200, "text/plain", low + " " + high + ", ok");
  });
  addMyCockpit("/pulseMult", 1, []() {
    String n  = server.arg(0);
    irsend.setPulseTimeMult(n.toFloat());
    server.send(200, "text/plain", n + ", ok");
  });
  addMyCockpit("/khz", 1, []() {
    String n  = server.arg(0);
    int carrierKhz  = n.toInt();
    irsend.enableIROut(carrierKhz);
    server.send(200, "text/plain", n + ", ok");
  });
  setupMyCockpit();

  irsend.begin();

}

void loop() {
  loopMyOTA();
  loopMyCockpit();

  // for test
  if( SPIFFS.exists("/a.txt") ) {
    delay(1000);
    HttpGet("http://thermo.sada.org/setConfig?key=a&val=1");
    SPIFFS.remove("/a.txt");
    //sendAnyDec(irData0);
  }

  // send alive
  if( CI.check() ) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    int id = getIdOfMacAddrSTA(mac);
    String url = String("http://svrsta.sada.org/alive?arg0=")+id;
    HttpGet(url.c_str(), NULL);
  }

  delay(100);
}

// generate html menu
String IRSendHtml(int id) {
  String html = String() + "<form action=\"/IRremote\" class=\"form-inline\">"
              + "<label>Type"+id+" </label>"
              + "<div class=\"input-group\"><input type=\"hidden\" name=\"arg\" class=\"form-control\" value=\""+id+"\">"
              + "<select name=\"arg\" class=\"form-control\">\r\n";
  int len = irSize[id]/sizeof(char *); //extract array size
  for(int i=0; i<len; i++) {
    html += String() + "<option value=\"" + i + "\">" + irName[id][i] + "</option>\r\n";
  }
  html += "</select><span class=\"input-group-btn\"><button type=\"submit\" class=\"btn btn-default\">IRremote</button></span></div></form>\r\n\r\n";
  return html;
}


// ir send
unsigned int raw[1024]; // should be global. system unstable if be local variable
void sendAnyDec(const uint16_t *decData) {
  //unsigned int raw[1024];
  unsigned int len = convToRaw(decData, raw);
  irsend.sendRaw(raw, len);
}

int convToRaw(const uint16_t *compData, unsigned int *raw) {
  unsigned int len  = pgm_read_word_near(compData + 0);
  unsigned int low  = pgm_read_word_near(compData + 5);
  unsigned int high = pgm_read_word_near(compData + 6);
  DebugOut.println(String("convToRaw: len=")+len);
  // set raw
  raw[0] = pgm_read_word_near(compData + 1);
  raw[1] = pgm_read_word_near(compData + 2);
  raw[2] = pgm_read_word_near(compData + 3);
  for (int i = 3; i < len - 1; i++) {
    int pos = i - 3;
    int bit = pos % 16;
    int idx = pos / 16;
    unsigned short t = pgm_read_word_near(compData + 7 + idx);
    //Serial.print(String(" ")+i+" "+idx+" "+bit+" ");Serial.println(t,BIN);
    raw[i] = ( (t >> bit) & 1 ) ? high : low;
  }
  raw[len - 1] = pgm_read_word_near(compData + 4);

  // debug print
  DebugOut.print("convToRaw: ");
  for (int i = 0; i < len; i++) {
    DebugOut.print(raw[i]);
    DebugOut.print(" ");
  }
  DebugOut.println("");
  return (int)len;
}

/*
// convert irData to Json : not used since it require big memory
void irSaveToJson(){
  Serial.println("testing");
  delay(5000);
  StaticJsonBuffer<5000> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& index = root.createNestedArray("index");
  JsonObject& data = root.createNestedObject("data");
#if 0
  for (int i = 0; i < 10; i++) {
    unsigned int raw[200];
    const uint16_t  *decData = irData + i * 15;
    unsigned int len = convToRaw(decData, raw);
    JsonArray& arr = data.createNestedArray(String(i));
    index.add(String(i));
    for(int j=0; j<len; j++){
      arr.add(raw[j]);
    }
    delay(0);
  }
#elseif 0
  for (int i = 0; i < 10; i++) {
    JsonArray& arr = data.createNestedArray(String(i));
    const uint16_t  *decData = irData + i * 15;
    index.add(String(i));
    for(int j=0; j<15; j++){
      unsigned int d  = pgm_read_word_near(decData + j);
      arr.add(d);
    }
    Serial.println(".");
    delay(0);
  }
#endif
  Serial.println(jsonBuffer.capacity());
  Serial.println(jsonBuffer.size());
  Serial.println(root.printTo(Serial));
  String js;
  root.printTo(js);
  Serial.println(999);
  delay(10000);
  //fileAppend("/ir.json",js.c_str());
}
*/


