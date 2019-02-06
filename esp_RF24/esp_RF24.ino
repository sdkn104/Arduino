/*
  Getting Started example sketch for nRF24L01+ radios
  This is a very basic example of how to send data from one node to another
  Updated: Dec 2014 by TMRh20
*/
//#include <ESP8266WiFi.h>

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>

CheckInterval CI(5000);
int sswitch;

//#include <SPI.h>
#include "RF24.h"
/****************** User Config ***************************/
RF24 radio(4, 5); // (CEpin, CSpin)
bool radioNumber = 1;      // 0:receiver, 1:sender
byte addressSVR[6] = "svr01";
byte addressDEV[6] = "dev01";
/**********************************************************/
struct RF24Payload {
  uint8_t cmd;
  uint8_t deviceId; 
  float temperature;
  float hummidity;
};

void setup() {
  Serial.begin(115200);
  Serial.println(F("RF24/examples/GettingStarted"));
  jsonConfig.load();

  WiFiConnect();
  printSystemInfo();
  setupMyOTA();
  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  SET_THIS_SKETCH();
  addMyCockpit("/switch", 1, []() {
    String n = server.arg(0);
    sswitch = n.toInt();
    server.send(200, "text/plain", n + ", ok");
  });
  setupMyCockpit();

  if( radioNumber == 0 ) { // receiver
    setupRF24(addressSVR, addressDEV);
  } else {  
    setupRF24(addressDEV, addressSVR);
  }
  // Start the radio listening for data
  radio.startListening(); // get into RX mode
  delay(10);
}

void setupRF24(byte *addressRX, byte *addressTX) {
  // powerOn and get into Standby-I mode
  bool r = radio.begin();
  DebugOut.println(String("radio.begin ") + (r + 0));
  if( !r ) { return; }

  // Set Power Amplifier (PA) level to one of four levels: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX(default)
  // The power levels correspond to the following output levels respectively: NRF24L01: -18dBm, -12dBm,-6dBM, and 0dBm
  //radio.setPALevel(RF24_PA_LOW);
  // set retry (setting big value might cause WDT time out ??)
  radio.setRetries(15, 3); // (delay, count) = (retry interval in 250us, max 15,  max retry count, max 15)
  radio.setAutoAck(true);
  radio.setDataRate( RF24_1MBPS ); // 1Mbps is most reliable (default)
  
  // Open a writing and reading pipe
  radio.openWritingPipe(addressTX);
  radio.openReadingPipe(1, addressRX);
}

void loop() {
  loopMyOTA();
  loopMyCockpit();

  int CIcheck = CI.check();
  if ( CIcheck )  {
    DebugOut.println(getDateTimeNow() + " " + radioNumber + " " + sswitch);
    DebugOut.print("connected "); DebugOut.println(radio.isChipConnected());
  }
  
  /****************** Sender ***************************/
  if ( CIcheck )  {
    if (radioNumber == 1 && sswitch )  {
      delay(0); // necesary. if not stopListening() would abort.
      radio.stopListening(); // get into Standby-I mode
      delay(0); // necesary. if not WDT would timed out
      DebugOut.println("stop listening");
      unsigned long start_time = micros();
      DebugOut.println(getDateTimeNow() + " Now sending " + start_time);
      if (!radio.write( &start_time, sizeof(unsigned long) )) { // get into TX mode, write, wait for receiving ack or exceeding max retries, get into standby-I 
        DebugOut.println(F("failed"));
      }
    }
  }
  /****************** Receiver ***************************/
  if ( radioNumber == 0 && sswitch ) {
    unsigned long got_time;
    if ( radio.available()) {
      while (radio.available()) {                                   // While there is data ready
        radio.read( &got_time, sizeof(unsigned long) );             // Get the payload
      }
      DebugOut.print(getDateTimeNow() + " Received ");
      DebugOut.println(got_time);
    }
  }
  delay(10);
} // Loop

