/*
 * Auto Amplifier Power ON/OFF by GY-273 board (3-axis Magnetometer module)
 * 
 * Connection:　Arduino <-> GY-273 
 *    5V-VCC, GND-GND, SCL-A5, SDA-A4
 */

#include <Wire.h> //I2C Arduino Library
#include <Statistic.h>
#include <MyLib.h>

const int I2CAddr = 0x1E; //I2C Address for The HMC5883
const int relayPin = 6;   // D6, digital output for relay
const int internalLedPin = 13; // D13

int state;
int error;

void setup(){
  // start Serial
  Serial.begin(9600);
  
  // setup I2C, GY273
  Wire.begin();
  Wire.beginTransmission(I2CAddr); //start talking
  Wire.write(0x00); //select config A register
  Wire.write(0x18); // measurment rate 10:15Hz,14:30Hz,18:75Hz
  Wire.endTransmission();
  Wire.beginTransmission(I2CAddr); //start talking
  Wire.write(0x02); //select mode register
  Wire.write(0x00); //continuous measurement mode
  Wire.endTransmission();
  Wire.beginTransmission(I2CAddr); //start talking
  Wire.write(0x01); //select gain register
  Wire.write(0x80); //gain (00,20,40,60,80,A0,C0,E0; larger, lower gain)
  Wire.endTransmission();
  delay(100); // wait for GY273 ready
  
  // setup Digital Pins
  pinMode(relayPin, OUTPUT);
  pinMode(internalLedPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  digitalWrite(internalLedPin, LOW);

  // init
  state = 0;
  error = 0;

  //
  DebugOut.setToNull();
}  

// number of samples for variation calculation
#define  NUM_SAMPLES  20

void loop(){  
  // measure magnetic
  Statistic stx; //stx.clear(0.0, 1.0);
  Statistic sty;
  Statistic stz;  
  for(int i=0; i<NUM_SAMPLES; i++) {
    int x,y,z; //triple axis data
    readGY273(&x, &y, &z);
    //Serial.println(x);
    stx.add(x);
    sty.add(y);
    stz.add(z);
    delay(13); // for 75Hz
  }
  double stdev = stx.stdev()+sty.stdev()+stz.stdev();
  DebugOut.print("total stdev: ------------------------ ");
  DebugOut.println(stdev);
  String a = String("ave:   ") + stx.average() + ", " + sty.average() + ", " + stz.average();
  String d = String("stdev: ") + stx.stdev() + ", " + sty.stdev() + ", " + stz.stdev();
  DebugOut.println(a);
  DebugOut.println(d);
  //stx.dump();

  // judge and relay
  if( stdev < 30.0 ) { // under lower threashold
    if( state == 1 ) {
      // toggle Amp
      digitalWrite(relayPin, HIGH);
      delay(1000);
      digitalWrite(relayPin, LOW);          
      state = 0;
    }
  } else if(stdev > 50.0 ) { // over upper threashold
    if( state == 0 ) {
      // toggle Amp
      digitalWrite(relayPin, HIGH);
      delay(1000);
      digitalWrite(relayPin, LOW);          
      state = 1;
    }    
  } else {
  }
  DebugOut.println(String("state: ") + state);

  // error check
  if( stx.minimum() == -4096 ) error = 1;
  if( sty.minimum() == -4096 ) error = 1;
  if( stz.minimum() == -4096 ) error = 1;
  DebugOut.println(String("error: ") + error);
  if( error ) digitalWrite(internalLedPin, HIGH);

  // wait
  delay(500);

  // interupt by Serial (debug)
  if (Serial.available()) {
    char in = Serial.read();
    if( in == 's' ) {DebugOut.setToSerial(); Serial.println("set output to Serial"); }
    if( in == 'n' ) {DebugOut.setToNull(); Serial.println("set output to Null"); }
    while(Serial.available()) Serial.read();
    Serial.println("waiting next input...");
    while(!Serial.available()) delay(100);
    while(Serial.available()) Serial.read();
  }
}

// read GY273
void readGY273(int *x, int *y, int *z) {
  //Tell the HMC what regist to begin writing data into
  Wire.beginTransmission(I2CAddr);
  Wire.write(0x03); //select register 3, X MSB register
  Wire.endTransmission();
   
 //Read the data.. 2 bytes for each axis.. 6 total bytes
  Wire.requestFrom(I2CAddr, 6);
  if(6<=Wire.available()){
    *x = Wire.read()<<8; //MSB  x 
    *x |= Wire.read(); //LSB  x
    *z = Wire.read()<<8; //MSB  z
    *z |= Wire.read(); //LSB z
    *y = Wire.read()<<8; //MSB y
    *y |= Wire.read(); //LSB y
  }
}

