/*
   Auto Amplifier Power ON/OFF by GY-273 board (3-axis Magnetometer module)

   Connection:　Arduino <-> GY-273
      5V-VCC, GND-GND, SCL-A5, SDA-A4
*/

/* Usage:
  1. Power off the Electric Equipment (TV, etc.)
    (to keep the state of power off)
      You had better power on then power off, since equipment may
      have get into ultra-low power mode after a long time power-off state.
  2. power on the Arduino
     -> start green LED blinking 60 times (about 1 minutes)
        (measuring sensor level of power-off state)
  3. after stop the LED blinkng, you may power on the equipment.
*/

#include <Wire.h> //I2C Arduino Library
#include <Statistic.h>
#include <MyLib.h>

const int I2CAddr = 0x1E; //I2C Address for The HMC5883
const int relayPin = 6;   // D6, digital output for relay
const int internalLedPin = 13; // D13

int state;
int error;

// number of samples for initial study
#define  NUM_STUDY_SAMPLES  60
float threshUpper;
float threshLower;

// number of samples for variation calculation
#define  NUM_SAMPLES  20

void setup() {
  // start Serial
  Serial.begin(9600);
  getResetFlag();
  DebugOut.setToSerial();

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

  // measure sensor level
  DebugOut.println("measure sensor level and set threshold");
  Statistic stx; //stx.clear(0.0, 1.0);
  Statistic sty;
  Statistic stz;
  Statistic st(0.0, 0.1);
  for (int n = 0; n < NUM_STUDY_SAMPLES; n++) {
    for (int i = 0; i < NUM_SAMPLES; i++) {
      int x, y, z; //triple axis data
      readGY273(&x, &y, &z);
      //Serial.println(x);
      stx.add(x);
      sty.add(y);
      stz.add(z);
      delay(13); // for 75Hz
    }
    double stdev = stx.stdev() + sty.stdev() + stz.stdev();
    DebugOut.print("total stdev: ------------------------ ");
    DebugOut.println(stdev);
    String a = String("ave:   ") + stx.average() + ", " + sty.average() + ", " + stz.average();
    String d = String("stdev: ") + stx.stdev() + ", " + sty.stdev() + ", " + stz.stdev();
    //DebugOut.println(a);
    //DebugOut.println(d);

    st.add(stdev);
    delay(500);
  }
  DebugOut.println(st.summary());

  // decide threshold
  threshLower = st.average() + st.stdev() * 3 + 10.0;
  threshUpper = threshLower + 10.0;
  DebugOut.print("lower threshold: "); DebugOut.println(threshLower);
  DebugOut.print("upper threshold: "); DebugOut.println(threshUpper);

  //
  DebugOut.setToNull();
}


void loop() {
  // measure magnetic
  Statistic stx; //stx.clear(0.0, 1.0);
  Statistic sty;
  Statistic stz;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    int x, y, z; //triple axis data
    readGY273(&x, &y, &z);
    //Serial.println(x);
    stx.add(x);
    sty.add(y);
    stz.add(z);
    delay(13); // for 75Hz
  }
  double stdev = stx.stdev() + sty.stdev() + stz.stdev();
  DebugOut.print("total stdev: ------------------------ ");
  DebugOut.println(stdev);
  String a = String("ave:   ") + stx.average() + ", " + sty.average() + ", " + stz.average();
  String d = String("stdev: ") + stx.stdev() + ", " + sty.stdev() + ", " + stz.stdev();
  DebugOut.println(a);
  DebugOut.println(d);
  //stx.dump();

  // judge and relay
  static unsigned int upperIdx = 0;
  static unsigned int lowerIdx = 0;

  if ( stdev < threshLower ) { // under lower threashold
    if ( state == 1 ) {
      // toggle Amp
      digitalWrite(relayPin, HIGH);
      delay(1000);
      digitalWrite(relayPin, LOW);
      delay(1000);
      state = 0;
    }
  } else if (stdev > threshUpper ) { // over upper threashold
    if ( state == 0 ) {
      // toggle Amp
      digitalWrite(relayPin, HIGH);
      delay(1000);
      digitalWrite(relayPin, LOW);
      delay(1000);
      state = 1;
    }
  } else {
  }

  // error check
  if ( stx.minimum() == -4096 ) error = 1;
  if ( sty.minimum() == -4096 ) error = 1;
  if ( stz.minimum() == -4096 ) error = 1;

  DebugOut.println(String("thresh: ") + threshLower + " " + threshUpper);
  DebugOut.print(String("state: ") + state);
  DebugOut.println(String("   error: ") + error);
  if ( error ) for (int i = 0; i < 10; i++) {
      digitalWrite(internalLedPin, HIGH);
      delay(100);
      digitalWrite(internalLedPin, LOW);
      delay(100);
    }

  if ( state == 1 ) digitalWrite(internalLedPin, HIGH);
  else              digitalWrite(internalLedPin, LOW);

  // wait
  delay(500);

  // interupt by Serial (debug)
  if (Serial.available()) {
    char in = Serial.read();
    if ( in == 's' ) {
      DebugOut.setToSerial();
      Serial.println("set output to Serial");
    } else if ( in == 'n' ) {
      DebugOut.setToNull();
      Serial.println("set output to Null");
    } else {
      Serial.println("illegal serial input");
    }
    while (Serial.available()) Serial.read();
    Serial.print("waiting next input ");
    for (int i = 40; i > 0 && (!Serial.available()); i--) {
      Serial.print(".");
      delay(500);
    }
    Serial.println("");
    while (Serial.available()) Serial.read();
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
  if (6 <= Wire.available()) {
    *x = Wire.read() << 8; //MSB  x
    *x |= Wire.read(); //LSB  x
    *z = Wire.read() << 8; //MSB  z
    *z |= Wire.read(); //LSB z
    *y = Wire.read() << 8; //MSB y
    *y |= Wire.read(); //LSB y
  }
}

// from http://forum.arduino.cc/index.php?topic=246359.0
void getResetFlag() {
  Serial.print("MCUSR: ");
  Serial.println(MCUSR);

  if (MCUSR & _BV(EXTRF)) {
    // Reset button or otherwise some software reset
    Serial.println("Reset button was pressed.");
  }
  if (MCUSR & (_BV(BORF) | _BV(PORF))) {
    // Brownout or Power On
    Serial.println("Power loss occured!");
  }
  if (MCUSR & _BV(WDRF)) {
    //Watchdog Reset
    Serial.println("Watchdog Reset");
  }
  // Clear all MCUSR registers immediately for 'next use'
  MCUSR = 0;
}

