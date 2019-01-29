#ifndef _NTP_h
#define _NTP_h

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <Time.h>
#include <TimeLib.h>

time_t getNtpTime();

// 2390
int ntp_begin(unsigned int localPort);

// 9 (JST)
time_t localtime(time_t t, int timeZone);

// ntp.nict.jp
void setTimeServer(const char* _timeServer);

#endif

