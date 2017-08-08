MyOTA
=====

Macro functions for using [ESP8266 OTA (Over the Air) Update](https://github.com/esp8266/Arduino/tree/master/doc/ota_updates), 
including my usual setting of OTA

## Example and Usage

```c++
#include <MyOTA.h>

setup() {
  ... establish WiFi connection ...
  setupMyOTA();
  ..
}

loop() {
  loopMyOTA();
  ....
}
```

* `setupMyOTA()` should be called once, after WiFi connection is established.
* `loopMyOTA()` must be called after `setupMyOTA()` is called. 
  It should be called repeatedly in short interval, to avoid losing OTA accesses.

