
# sketchLibraryNTP


- original: https://github.com/exabugs/sketchLibraryNTP
- modified by T.Sadakane


ESP8266用のNTPライブラリ

Timeライブラリ の setSyncProvider で使用可能

```cpp
#include <ESP8266WiFi.h>
#include <NTP.h>

void setup() {
  Serial.begin(115200);
  Serial.println();

  // 自分のネットワークに合わせてください
  WiFi.begin("your-ssid", "your-password");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");

  // 初期化
  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  ntp_begin(2390);

  // NTPサーバを変更 (デフォルト: ntp.nict.jp)
  //setTimeServer("s2csntp.miz.nao.ac.jp");

  // NTP同期間隔を変更 (デフォルト: 300秒)
  //setSyncInterval(10);
}


void display(){

  time_t n = now();
  time_t t;

  char s[20];
  const char* format = "%04d-%02d-%02d %02d:%02d:%02d";

  // JST
  t = localtime(n, 9);
  sprintf(s, format, year(t), month(t), day(t), hour(t), minute(t), second(t));
  Serial.print("JST : ");
  Serial.println(s);

  // UTC
  t = localtime(n, 0);
  sprintf(s, format, year(t), month(t), day(t), hour(t), minute(t), second(t));
  Serial.print("UTC : ");
  Serial.println(s);
}

void loop() {
  display();
  delay(2000);
}
```
