#include "NTP.h"

// added by T.Sadakane
#include "MyLib.h"
#define Serial DebugOut

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

const char* timeServer = "ntp.nict.jp";

// 初期化
// 2390
int ntp_begin(unsigned int localPort)
{
  udp.begin(localPort);

  Serial.println("Start NTP sync on port " + String(localPort) + ", server " + String(timeServer)); //added by sada
 
  setSyncProvider(getNtpTime);
  setSyncInterval(300); // NTP同期間隔を変更 (デフォルト: 300秒)
}

// 9 (JST)
time_t localtime(time_t t, int timeZone)
{
  return t + timeZone * SECS_PER_HOUR;
}

// ntp.nict.jp
void setTimeServer(const char* _timeServer)
{
  timeServer = _timeServer;
}



const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

// send an NTP request to the time server at the given address
void sendNTPpacket(const char* address)
{
  Serial.print("sendNTPpacket : ");
  Serial.println(address);

  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0]  = 0b11100011;   // LI, Version, Mode
  packetBuffer[1]  = 0;     // Stratum, or type of clock
  packetBuffer[2]  = 6;     // Polling Interval
  packetBuffer[3]  = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

time_t readNTPpacket() {
  Serial.println("Receive NTP Response");
  udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
  unsigned long secsSince1900 = 0;
  // convert four bytes starting at location 40 to a long integer
  secsSince1900 |= (unsigned long)packetBuffer[40] << 24;
  secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
  secsSince1900 |= (unsigned long)packetBuffer[42] <<  8;
  secsSince1900 |= (unsigned long)packetBuffer[43] <<  0;
  return secsSince1900 - 2208988800UL;
}

time_t getNtpTime()
{
  while (udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);

  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      return readNTPpacket();
    }
   }

  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

