#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>

const int speaker_pin  = 13; // output
const int led_pin      = 2; // output on-board LED

CheckInterval CI(5000);

int beep_start = 0;
int beep_stop = 0;

ADC_MODE(ADC_VCC);

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("start ESP");

  // pin setting
  pinMode(speaker_pin, OUTPUT); // digital out, or PWM out
  analogWrite(speaker_pin, 0); // stop PWM
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW); // on

  // load json config
  jsonConfig.load();
  //jsonConfigFlush();
  JsonObject &conf = jsonConfig.obj();

  // setup STA mode
  WiFiConnect();
  printSystemInfo();
  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  setupMyOTA();
  SET_THIS_SKETCH();
  addMyCockpit("/beepStart", 1, []() {
    String f = server.arg(0);
    beep_start = f.toInt();
    beep_stop = 0;
    server.send(200, "text/plain", f + ", ok");
  });
  addMyCockpit("/beepStop", 0, []() {
    beep_stop = 1;
    server.send(200, "text/plain", "ok");
  });
  setupMyCockpit();
  digitalWrite(led_pin, HIGH); // off
}

void loop() {
  loopMyOTA();
  loopMyCockpit();
  
  // for test
  if ( beep_start ) {
    beepStart(speaker_pin, beep_start);
    beep_start = 0;
  }
  if ( beep_stop ) {
    beepStop(speaker_pin);
    beep_stop = 0;
  }
  
  delay(1);
}


void beep(int pin, float hz, int ms) {
  beepStart(pin, hz);
  delay(ms);
  beepStop(pin);
}

void beepStart(int pin, float hz) {
  analogWriteFreq((int)hz);
  analogWrite(pin, 512); // half duty?
}

void beepStop(int pin) {
  analogWrite(pin, 0);
}

