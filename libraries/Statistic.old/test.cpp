//
// Statistic library -- TEST module
//


#include "Statistic.h"

String Statistic_test() {
  String log = "";
  log += "------- START test of Statistic class ---------\r\n";

  // print parameters
  log += "LONG_MAX: "; log += LONG_MAX; log += "\r\n";
  //log += "DBL_MAX:  "; log += DBL_MAX; log += "\r\n";
  long SQRT_LONG_MAX = (long) floor(sqrt(LONG_MAX)-0.1); log += "\r\n";
  log += "SQRT_LONG_MAX:  "; log += SQRT_LONG_MAX); log += "\r\n";


  // check default value of center and precision (constructor)
  Statistic stat1;
  stat1.add(1.2);
  stat1.add(1.3);
  if( stat1.sum() != 2.0 ) log += "TEST NG: 1a\r\n";
  if( stat1.average() != 1.0 ) log += "TEST NG: 1b\r\n";
  stat1.add(SQRT_LONG_MAX-2);
  if( stat1.overflow_count() != 0 ) log += "TEST NG: 1c\r\n";
  stat1.add(1);
  if( stat1.overflow_count() != 1 ) log += "TEST NG: 1d\r\n";

  // check center and precision specified by constructor
  Statistic stat2(100, 0.1);
  stat2.add(101.22);
  stat2.add(101.21);
  if( stat2.sum() != 202.4 ) log += "TEST NG: 2a\r\n";
  if( stat2.average() != 101.2 ) log += "TEST NG: 2b\r\n";
  stat2.add(SQRT_LONG_MAX-100-12*12*2);
  if( stat2.overflow_count() != 0 ) log += "TEST NG: 2c\r\n";
  stat2.add(101);
  if( stat2.overflow_count() != 1 ) log += "TEST NG: 2d\r\n";
  
  // check center and precision specified by clear
  stat1.clear(100, 0.1);
  stat1.add(101.22);
  stat1.add(101.21);
  if( stat1.sum() != 202.4 ) log += "TEST NG: 3a\r\n";
  if( stat1.average() != 101.2 ) log += "TEST NG: 3b\r\n";
  stat1.add(SQRT_LONG_MAX-100-12*12*2);
  if( stat1.overflow_count() != 0 ) log += "TEST NG: 3c\r\n";
  stat1.add(101);
  if( stat1.overflow_count() != 1 ) log += "TEST NG: 3d\r\n";
  
  // check default center and precision by clear
  stat1.clear();
  stat1.add(1.2);
  stat1.add(1.3);
  if( stat1.sum() != 2.0 ) log += "TEST NG: 4a\r\n";
  if( stat1.average() != 1.0 ) log += "TEST NG: 4b\r\n";
  stat1.add(SQRT_LONG_MAX-2);
  if( stat1.overflow_count() != 0 ) log += "TEST NG: 4c\r\n";
  stat1.add(1);
  if( stat1.overflow_count() != 1 ) log += "TEST NG: 4d\r\n";

  // check rounding
  stat1.clear(100, 0.1);
  stat1.add(102.123);
  if( stat1.sum() != 102.1 ) log += "TEST NG: 5a\r\n";
  stat1.clear(100, 0.1);
  stat1.add(102.19);
  if( stat1.sum() != 102.2 ) log += "TEST NG: 5b\r\n";
  stat1.clear(100, 0.1);
  stat1.add(1.11);
  if( stat1.sum() != 1.2 ) log += "TEST NG: 5c\r\n";
  stat1.clear(100, 0.1);
  stat1.add(1.19);
  if( stat1.sum() != 1.2 ) log += "TEST NG: 5d\r\n";
  stat1.clear(100, 0.1);
  stat1.add(-1.11);
  if( stat1.sum() != -1.2 ) log += "TEST NG: 5e\r\n";
  stat1.clear(100, 0.1);
  stat1.add(-1.19);
  if( stat1.sum() != -1.2 ) log += "TEST NG: 5f\r\n";

  // check count, sum, average, max, min
  stat1.clear(100, 0.1);
  stat1.add(101.11);
  stat1.add(-10.11);
  if( stat1.count() != 2 ) log += "TEST NG: 6a\r\n";
  if( stat1.sum() != 111 ) log += "TEST NG: 6b\r\n";
  if( stat1.average() != 55 ) log += "TEST NG: 6c\r\n";
  if( stat1.minimum() != -10.1 ) log += "TEST NG: 6d\r\n";
  if( stat1.maximum() != 101.1 ) log += "TEST NG: 6e\r\n";
  stat1.add(0);
  if( stat1.count() != 3 ) log += "TEST NG: 6f\r\n";
  if( stat1.sum() != 111 ) log += "TEST NG: 6g\r\n";
  

  log += "------- END   test of Statistic class ---------\r\n";

  return log;
}

