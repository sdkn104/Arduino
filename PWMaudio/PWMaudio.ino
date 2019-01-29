#include <Arduino.h>
// #include <ESP8266WiFi.h>
#include "i2s.h"

#include "wavspiffs.h"

//#include "data/output.h"
//#include "tone.h"
//#include "sweep.h"
const uint16 sweep_wav[] PROGMEM = {1};

#include <NTP.h>
#include <MyOTA.h>
#include <MyLib.h>
#include <MyCockpit.h>


const int led_pin      = 2; // output on-board LED

wavFILE_t wf;
int16_t wBuffer[1];

const uint16_t *sweep_wav_data = sweep_wav;
int audio_data;

//Array with 32-bit values which have one bit more set to '1' in every consecutive array index value
/*const uint32_t ICACHE_RODATA_ATTR _i2s_to_pwm[32] = {
  0x00000010, 0x00000410, 0x00400410, 0x00400C10, 0x00500C10, 0x00D00C10, 0x20D00C10, 0x21D00C10, 0x21D80C10, 0xA1D80C10,
  0xA1D80D10, 0xA1D80D30, 0xA1DC0D30, 0xA1DC8D30, 0xB1DC8D30, 0xB9DC8D30, 0xB9FC8D30, 0xBDFC8D30, 0xBDFE8D30, 0xBDFE8D32,
  0xBDFE8D33, 0xBDFECD33, 0xFDFECD33, 0xFDFECD73, 0xFDFEDD73, 0xFFFEDD73, 0xFFFEDD7B, 0xFFFEFD7B, 0xFFFFFD7B, 0xFFFFFDFB,
  0xFFFFFFFB, 0xFFFFFFFF
};*/

const uint32_t ICACHE_RODATA_ATTR _i2s_to_pwm[32] = {
  0x00000001,0x00010001,0x00010101,0x01010101,
  0x01010111,0x01110111,0x01111111,0x11111111,0x11111115,
  0x11151115,0x11151515,0x15151515,0x15151555,0x15551555,
  0x15555555,0x55555555,0x55555557,0x55575557,0x55575757,
  0x57575757,0x57575777,0x57775777,0x57777777,0x77777777,
  0x7777777F,0x777F777F,0x777F7F7F,0x7F7F7F7F,0x7F7F7FFF,
  0x7FFF7FFF,0x7FFFFFFF,0xFFFFFFFF
};

static void i2s_write_pwm(int s) { // assume int16
  //Instead of having a nice PCM signal, we fake a PWM signal here.
  static int err = 0;
  int samp = s;
  samp = (samp + 32768); //to unsigned
  samp -= err;    //Add the error we made when rounding the previous sample (error diffusion)
  //clip value
  if (samp > 65535) samp = 65535;
  if (samp < 0) samp = 0;
  uint32_t ss = _i2s_to_pwm[samp >> 11]; //send pwm value for sample value
  err = (ss & 0x7ff); //Save rounding error.
  i2s_write_sample(ss); // arg:uint32
  //DebugOut.println(ss);
}

int wavReadOne(int16_t *out) {
    const int buffsize = 64; // 1:bad, 64:better, 1024:=64, 5000:=1024
    static int16_t buff[buffsize];
    static int pos = 0;
    static int buffnum = 0;
    if( pos == buffnum ) { // empty
      delay(0);
      buffnum = wavRead(&wf, buff, sizeof(buff)) / sizeof(buff[0]);
      delay(0);
      pos = 0;
    }
    if( pos == buffnum ) return 0;
    *out = buff[pos++];
    return 1;
}

void setup() {
  Serial.begin(115200);
  Serial.print("\n");
  // Serial.setDebugOutput(true);

  jsonConfig.load();
  //jsonConfig.setFlush(jsonConfigFlush);
  //jsonConfig.flush();
  JsonObject &conf = jsonConfig.obj();

  pinMode(led_pin, OUTPUT);

  //wifi_set_sleep_type(LIGHT_SLEEP_T); // default=modem
  WiFiConnect();
  printSystemInfo();
  ntp_begin(2390);  // 2390 はローカルのUDPポート。空いている番号なら何番でもいいです。
  setupMyOTA();
  SET_THIS_SKETCH();
  setupMyCockpit();

  for (int i = 0; i < 200; i++) {
    loopMyOTA();
    loopMyCockpit();
    delay(50);
    digitalWrite(led_pin, (i / 10) % 2);
  }
  digitalWrite(led_pin, HIGH); // off
}

void loop() {
  JsonObject &conf = jsonConfig.obj();

  loopMyOTA();
  loopMyCockpit();

  // VOLUME: 0-10
#define  MKWAVE(MOD, VOLUME) ( ( (i%(MOD))*256*256/((MOD)-1) - 256*128 ) * (VOLUME)/10 )
#define  MKWAVE2(MOD, VOLUME) ( (i%(MOD) >= (MOD)/2 ? 1 : -1) *256*128 * (VOLUME)/10 )

  if ( conf["audioLen"] > 0 ) {
    int len = conf.containsKey("audioLen") ? conf["audioLen"] : 0;
    //len = len > sweep_wav_len ? sweep_wav_len : len;
    int freq = conf.containsKey("audioFreq") ? conf["audioFreq"] : 44200;
    int mod = conf.containsKey("audioMod") ? conf["audioMod"] : 100;
    int volume = conf.containsKey("audioVolume") ? conf["audioVolume"] : 10;
    conf["audioLen"] = 0;

    wavProperties_t wProps;
    int rc;
    if ( conf["audioData"] == 9 ) {
      if ( rc = wavOpen("/test.wav", &wf, &wProps) ) DebugOut.println(String("open error ") + rc + "\n");
      DebugOut.printf("audioFormat %d\r\n", wProps.audioFormat);
      DebugOut.printf("numChannels %d\r\n", wProps.numChannels);
      DebugOut.printf("sampleRate %d\r\n", wProps.sampleRate);
      DebugOut.printf("byteRate %d\r\n", wProps.byteRate);
      DebugOut.printf("blockAlign %d\r\n", wProps.blockAlign);
      DebugOut.printf("bitsPerSample %d\r\n", wProps.bitsPerSample);
    }

    delay(500);
    i2s_begin();
    i2s_set_rate(freq);
    DebugOut.println(String("start i2s ")+micros());
    for (int i = 0; i < len; i++)
    {
      if ( conf["audioData"] == 1 ) {
        uint16_t t = pgm_read_word(sweep_wav_data + i);
        int16_t tt = t;
        audio_data = (int)tt;
        audio_data = audio_data * volume/10;
      } else if ( conf["audioData"] == 2 ) {
        audio_data = MKWAVE(mod, volume);
      } else if ( conf["audioData"] == 3 ) {
        audio_data = MKWAVE2(mod, volume);
      } else if ( conf["audioData"] == 4 ) {
        audio_data = volume*256*256/32 - 256*128;
      } else if ( conf["audioData"] == 9 ) {
        if ( !wavReadOne(wBuffer) ) {
           DebugOut.println(String("read nothing (EOF) @") + i + "\n");
           break;
        }
//        fileAppend("/dump2.txt",String(wBuffer[0]).c_str());fileAppend("/dump2.txt","\n");
        audio_data = wBuffer[0] * volume / 10;
      } else {
        audio_data = 0;
      }

      //DebugOut.printf("%x\n",audio_data);
      //delay(100);
      if( i%1==0) delay(0);

      // write 16 bit sound values
      i2s_write_pwm(audio_data);
    }
    DebugOut.println(String("end i2s ")+micros());
    i2s_end();
    if ( conf["audioData"] == 9 ) {
      wavClose(&wf);
      DebugOut.println("closed");
    }
  }
  delay(50);
}
