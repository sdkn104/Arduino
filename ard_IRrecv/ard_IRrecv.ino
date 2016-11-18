//
// Arduino receive IR remote controller signal and dump info to Serial
//

#include <IRremote.h>

//------------------------------------------------------------------------------
// Tell IRremote which Arduino pin is connected to the IR Receiver (TSOP4838)
//
int recvPin = 11;
IRrecv irrecv(recvPin);

//+=============================================================================

#include <Statistic.h>

void  setup ( )
{
  Serial.begin(9600);   // Status message will be sent to PC at 9600 baud
  irrecv.enableIRIn();  // Start the receiver
  Serial.println("Start ESP IRrecv");
  Serial.flush();
}

void  loop()
{
  decode_results  results;        // Somewhere to store the results

  if (irrecv.decode(&results)) {  // Grab an IR code
    Serial.println("decoded:");           // Blank line between entries
    //dumpInfo(results);           // Output the results
    dumpRaw(&results);            // Output the results in RAW format
    //dumpCode(&results);           // Output the results as source code
    decodeAny(&results);           // Output code
    Serial.println("");           // Blank line between entries
    irrecv.resume();              // Prepare for the next value
  }
}

/*
//+=============================================================================
// Display IR code
//
void  ircode (decode_results *results)
{
  // Panasonic has an Address
  if (results->decode_type == PANASONIC) {
    Serial.print(results->address, HEX);
    Serial.print(":");
  }

  // Print Code
  Serial.print(results->value, HEX);
}

//+=============================================================================
// Display encoding type
//
void  encoding (decode_results *results)
{
  switch (results->decode_type) {
    default:
    case UNKNOWN:      Serial.print("UNKNOWN");       break ;
    case NEC:          Serial.print("NEC");           break ;
    case SONY:         Serial.print("SONY");          break ;
    case RC5:          Serial.print("RC5");           break ;
    case RC6:          Serial.print("RC6");           break ;
    case DISH:         Serial.print("DISH");          break ;
    case SHARP:        Serial.print("SHARP");         break ;
    case JVC:          Serial.print("JVC");           break ;
    case SANYO:        Serial.print("SANYO");         break ;
    case MITSUBISHI:   Serial.print("MITSUBISHI");    break ;
    case SAMSUNG:      Serial.print("SAMSUNG");       break ;
    case LG:           Serial.print("LG");            break ;
    case WHYNTER:      Serial.print("WHYNTER");       break ;
    case AIWA_RC_T501: Serial.print("AIWA_RC_T501");  break ;
    case PANASONIC:    Serial.print("PANASONIC");     break ;
    case DENON:        Serial.print("Denon");         break ;
  }
}

//+=============================================================================
// Dump out the decode_results structure.
//
void  dumpInfo (decode_results *results)
{
  // Check if the buffer overflowed
  if (results->overflow) {
    Serial.println("IR code too long. Edit IRremoteInt.h and increase RAWLEN");
    return;
  }

  // Show Encoding standard
  Serial.print("Encoding  : ");
  encoding(results);
  Serial.println("");

  // Show Code & length
  Serial.print("Code      : ");
  ircode(results);
  Serial.print(" (");
  Serial.print(results->bits, DEC);
  Serial.println(" bits)");
}
*/
//+=============================================================================
// Dump out the raw data of received code
//
void  dumpRaw (decode_results *results)
{
  // Check if the buffer overflowed
  if (results->overflow) {
    Serial.println("IR code too long. Edit IRremoteInt.h and increase RAWLEN");
    return;
  }

  // Print Raw data
  Serial.print("RawTiming[");
  Serial.print(results->rawlen-1, DEC);
  Serial.println("]: ");

  for (int i = 1;  i < results->rawlen;  i++) {
    unsigned long  x = results->rawbuf[i] * USECPERTICK;
    if (!(i & 1)) {  // even
      Serial.print("-");
      if (x < 1000)  Serial.print(" ") ;
      if (x < 100)   Serial.print(" ") ;
      Serial.print(x, DEC);
    } else {  // odd
      Serial.print("     ");
      Serial.print("+");
      if (x < 1000)  Serial.print(" ") ;
      if (x < 100)   Serial.print(" ") ;
      Serial.print(x, DEC);
      if (i < results->rawlen-1) Serial.print(", "); //',' not needed for last one
    }
    if (!(i % 8))  Serial.println("");
  }
  Serial.println(" ;");                    // Newline
}

/*
//+=============================================================================
// Dump out the decode_results structure.
//
void  dumpCode (decode_results *results)
{
  // Start declaration
  Serial.print("unsigned int  ");          // variable type
  Serial.print("rawData[");                // array name
  Serial.print(results->rawlen - 1, DEC);  // array size
  Serial.print("] = {");                   // Start declaration

  // Dump data
  for (int i = 1;  i < results->rawlen;  i++) {
    Serial.print(results->rawbuf[i] * USECPERTICK, DEC);
    if ( i < results->rawlen-1 ) Serial.print(","); // ',' not needed on last one
    if (!(i & 1))  Serial.print(" ");
  }

  // End declaration
  Serial.print("};");  // 

  // Comment
  Serial.print("  // ");
  encoding(results);
  Serial.print(" ");
  ircode(results);

  // Newline
  Serial.println("");

  // Now dump "known" codes
  if (results->decode_type != UNKNOWN) {

    // Some protocols have an address
    if (results->decode_type == PANASONIC) {
      Serial.print("unsigned int  addr = 0x");
      Serial.print(results->address, HEX);
      Serial.println(";");
    }

    // All protocols have data
    Serial.print("unsigned int  data = 0x");
    Serial.print(results->value, HEX);
    Serial.println(";");
  }
}
*/

// Commpress received results and dump the compressed data to Serial
//   compressed data is stored in array of uint16_t of length specified with declen
long decodeAny(decode_results *results) {
  const int declen = 45;        // length of the data array !!!! please set size !!!!
  const int blen = declen - 7;
  static int id = 0;
  Serial.print("decoding data "); Serial.println(results->rawlen-1);  

  // Check if the raw data buffer overflowed
  if (results->overflow) {
    Serial.println("IR code too long. Edit IRremoteInt.h and increase RAWLEN");
    return false;
  }

  // check if too short
  if (results->rawlen < 6) { // < 5data, no data
    Serial.println("error: too short");
    return false ;
  }
  
  // analyze data bit width
  Statistic ST(500,1);
  for (int i = 4;  i < results->rawlen - 1;  i++) { // omit first 3, last 1
    int  x = results->rawbuf[i] * USECPERTICK;
    //if( true ) Serial.println(i);
    //if( true ) Serial.println(x);
    ST.add(x);
  }  
  double th = (ST.minimum() + ST.maximum() ) / 2.0;
  ST.clear();
    Serial.print("th:"); Serial.println(th);
  Statistic ST0(500,1);
  Statistic ST1(500,1);
  for (int i = 4;  i < results->rawlen - 1;  i++) { // omit first 3, last 1
    int  x = results->rawbuf[i] * USECPERTICK;
    if( x < th ) ST0.add(x);
    else         ST1.add(x);
  }
  Serial.print("low:"); Serial.println(ST0.summary());
  Serial.print("high:"); Serial.println(ST1.summary());

  // dump array size
  Serial.print("uint16_t irLen = ");   // array size
  Serial.print(declen);
  Serial.println(" ;");

  // dump data code
  //     dump header
  Serial.print("uint16_t ");          // variable type
  Serial.print("irData[");                // array name
  Serial.println("] = {");                   // Start declaration
  Serial.println("// len, 1st,2nd,3rd,last, low,high, 4th-(encoded),...");
  Serial.print(String("/* ") + (id++) + " */ ");

  //     dump data : 1,2,3,last
  Serial.print(results->rawlen - 1); Serial.print(", "); // raw length
  Serial.print(results->rawbuf[1] * USECPERTICK, DEC); Serial.print(","); // no1
  Serial.print(results->rawbuf[2] * USECPERTICK, DEC); Serial.print(","); // no1
  Serial.print(results->rawbuf[3] * USECPERTICK, DEC); Serial.print(","); // no1
  Serial.print(results->rawbuf[results->rawlen - 1] * USECPERTICK, DEC); Serial.print(", "); // last
  Serial.print(ST0.average(), 0); Serial.print(","); // low val
  Serial.print(ST1.average(), 0); Serial.print(", "); // high val

  //     dump data : encoded 4th to last-1
  int idx;
  unsigned short b[blen];
  for(int i=0; i<blen; i++) b[i]=0; // init
  int bits = (results->rawlen - 1 - 4); 
  for (int i = 4;  i < results->rawlen - 1;  i++) { // omit first 3, last 1
    int bit = (i-4)%16;
    idx = (i-4)/16;    
    unsigned short  x = results->rawbuf[i] * USECPERTICK;
    unsigned short f = ((unsigned short)0x1 << bit);
    if( x < th ) b[idx] = b[idx];
    else         b[idx] = b[idx] | f;
  }
  for (int i = 0;  i < blen;  i++) {
    Serial.print(b[i], DEC);  Serial.print(", ");
  }  

  //    dump footer
  Serial.println("");  // 
  Serial.println("};");  // 
  Serial.print("buffer len : used= "); Serial.print(idx+1+7);
  Serial.print(" / buff= "); Serial.println(declen);
  if( idx >= blen ) Serial.println("ERROR: too long length. overflow");  

  // dump info for debug (binary format)
  Serial.print("Binary(4th-): ");
  for (int i = 0;  i <= (bits-1)/16;  i++) {
    for(int p=0; p<16; p++) {
      Serial.print((b[i]>>p)&0x1);
    }
    Serial.print(" ");
  }
  Serial.println(";");

  return true;
}

