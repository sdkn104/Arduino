#ifndef Statistic_h
#define Statistic_h

// This is the Original header---
//    FILE: Statistic.h
//  AUTHOR: Rob dot Tillaart at gmail dot com
//          modified at 0.3 by Gil Ross at physics dot org
// VERSION: 0.3.3
// PURPOSE: Recursive Statistical library for Arduino
// HISTORY: See Statistic.cpp
//
// Released to the public domain
//

// the standard deviation increases the lib size
// it can be in/excluded by un/commenting next line
#define STAT_USE_STDEV

#include <math.h>
#include <Arduino.h>

#define STATISTIC_LIB_VERSION "0.3.3"

// 3.4028235E+38 and as low as -3.4028235E+38

class Statistic
{
public:
    Statistic();
    Statistic(double center, double precision);
    void clear();
    void clear(double center, double precision);
    void add(double);
    void dump(); // for debug

    // returns the number of values added
    unsigned long count()   { return _cnt; }; // zero if empty
    double sum()            { return conv2dif(_sum) + _center*_cnt; }; // zero if empty
    double minimum()        { return conv2val(_min); }; // zero if empty
    double maximum()        { return conv2val(_max); }; // zero if empty
    double average();
    String summary();

#ifdef STAT_USE_STDEV
    double variance();
    double unbiased_variance();
    double stdev();
    double biased_stdev();
    double unbiased_stdev();
#endif

protected:
    unsigned long _cnt;
    double _store;           // store to minimise computation
    double _sum;
    double _min;
    double _max;
    double _center;
    double _precision;
    long  _sum_i;
#ifdef STAT_USE_STDEV
    unsigned long _overflow_cnt;
    unsigned long    _ssq;		    // sum of squares
    double _ssqdif;		    // sum of squares difference
#endif
    // convert to/from internal representation of sample value
    long conv2internal(double value) { return long( ((value) - _center) / _precision ); }
    double conv2val(double val) { return ( (val) * _precision + _center ); }
    double conv2dif(double val) { return ( (val) * _precision ); }
    double conv2var(double val) { return ( (val) * _precision * _precision ); }
};

#endif
// END OF FILE
