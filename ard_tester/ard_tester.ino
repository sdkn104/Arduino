//
//
//

#include <MyLib.h>
#include <Statistic.h>

int probePin = A1;   //アナログ1番ピン
int internalLedPin = 13;  //digital 13番ピン

//**** SETTING Vref ****//
const long vrefx10 = 50; // 50 or 11

// globals
CheckInterval interval(1000);

void setup() {
//  Serial.begin(115200);           //シリアルモニタに表示する為の設定
  Serial.begin(230400);           //シリアルモニタに表示する為の設定
  Serial.println("arduino starting");

  // setup I/O pins
  if( vrefx10 == 11 ) {
    analogReference(INTERNAL); // DEFAULT:5V, INTERNAL:1.1V
  } else {
    analogReference(DEFAULT); // DEFAULT:5V, INTERNAL:1.1V
  }
  pinMode(internalLedPin, OUTPUT); // initialize digital pin 13 as an output.

  delay(500);
  Serial.println(String("Vref=")+(vrefx10/10.0));
  Serial.println("arduino invoked.");
}

void loop() {
  //----- handling command from host
  if ( Serial.available() > 0 ) {
    char rd = Serial.read();
    String arg = Serial.readStringUntil('\r');
    Serial.println(String("arduino get command ")+rd+" "+arg);
    if ( rd == '?' ) { // help
    } else if(rd == 'i') {
      interval.set(arg.toInt());
    } else {
      Serial.println("arduino warning illegal command syntax");
    }
  }

  // -----probe read
  if( interval.get()==0 || interval.check() ){
    //Statistic stt;
    long value = analogRead(probePin);
    float volt = value*vrefx10/10.0/1024;
    Serial.println(getDateTimeNow()+" "+value+" "+volt);
    //Serial.print(millis());
    //Serial.println(String(", ")+volt*1000/2.5);
  }
  delay(0);
}

