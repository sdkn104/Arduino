//


extern "C" {
#include <user_interface.h> // for sleep mode
}

#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>

CheckInterval CI(10000);

ADC_MODE(ADC_VCC); // for use of getVcc. ADC pin must be open

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
  addHtmlMyCockpit(String("Sketch: ")+THIS_SKETCH+"<BR><BR>");
  setupMyCockpit();
}

void loop() {
  loopMyOTA();
  loopMyCockpit();

  if( Serial.available() ){
    byte hdr[11];
    byte data[256];    
    Serial.readBytes(hdr,11);
    Serial.readBytesUntil('\r',data,251);
    uint8_t *mac = hdr;
    uint8_t len = hdr[6];
    data[len-4] = '\0';
    String dat = String((char *)data);
    DebugOut.println(macAddress2String(mac)+" "+len+" "+dat);
  }

  if( CI.check() ) {
    Serial.println(getDateTimeNow());
  }
  delay(10);
}

