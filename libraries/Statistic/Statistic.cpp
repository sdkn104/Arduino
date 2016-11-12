// Modified by T. Sadakane
// - change variance and standard deviation methods
//    use exact algorithm at first, and then use approx. algorithm.

//
//    FILE: Statistic.cpp
//  AUTHOR: Rob dot Tillaart at gmail dot com
//          modified at 0.3 by Gil Ross at physics dot org
// VERSION: 0.3.3
// PURPOSE: Recursive statistical library for Arduino
//
// NOTE: 2011-01-07 Gill Ross
// Rob Tillaart's Statistic library uses one-pass of the data (allowing
// each value to be discarded), but expands the Sum of Squares Differences to
// difference the Sum of Squares and the Average Squared. This is susceptible
// to bit length precision errors with the float type (only 5 or 6 digits
// absolute precision) so for long runs and high ratios of
// the average value to standard deviation the estimate of the
// standard error (deviation) becomes the difference of two large
// numbers and will tend to zero.
//
// For small numbers of iterations and small Average/SE th original code is
// likely to work fine.
// It should also be recognised that for very large samples, questions
// of stability of the sample assume greater importance than the
// correctness of the asymptotic estimators.
//
// This recursive algorithm, which takes slightly more computation per
// iteration is numerically stable.
// It updates the number, mean, max, min and SumOfSquaresDiff each step to
// deliver max min average, population standard error (standard deviation) and
// unbiassed SE.
// -------------
//
// HISTORY:
// 0.1 - 2010-10-29 initial version
// 0.2 - 2010-10-29 stripped to minimal functionality
// 0.2.01 - 2010-10-30
//   added minimim, maximum, unbiased stdev,
//   changed counter to long -> int overflows @32K samples
// 0.3 - 2011-01-07
//   branched from 0.2.01 version of Rob Tillaart's code
// 0.3.1 - minor edits
// 0.3.2 - 2012-11-10
//   minor edits
//   changed count -> unsigned long allows for 2^32 samples
//   added variance()
// 0.3.3 - 2015-03-07
//   float -> double to support ARM (compiles)
//   moved count() sum() min() max() to .h; for optimizing compiler
//
// Released to the public domain
//

// RESTRICTION:
// asume that long is signed 32bit (9-10 decimal digits)
//            double is 4byts floating point (6-7 decimal digits of precision,
//                                           (range from -3.4028235E+38 to 3.4028235E+38)
// internal overflow:
//   internal value (= (added value - center) / precision) should be within long type range 
// accuracy:
//   internal sum is double type, so
//      precision of average, variance, deviation are the same as double type
//   variation calculation uses approx algorithm for large number of samples,
//     uses approx algorithm when and after 
//        internal sum overflow : sum of internal values > max of long type
//            or
//        internal sum of square of internal values > max of long type
//        -> recomend the internal vale should be less than 4000 and larger than -4000


#include "Statistic.h"

#include "Arduino.h"

#define SLONG_MAX 2147483647  /*  2**31-1, max positive value in signed long */
#define SQRT_SLONG_MAX 46340  /*  floor(sqrt(SLONG_MAX)) */



Statistic::Statistic()
{
    clear();
}

Statistic::Statistic(double center, double precision)
{
    clear(center, precision);
}

// resets all counters
void Statistic::clear() {
    _cnt = 0;
    _sum = 0.0;
    _min = 0.0;
    _max = 0.0;
#ifdef STAT_USE_STDEV
    _center = 0;
    _precision = 1.0;
    _overflow_cnt = 0;
    _sum_i = 0;
    _ssq = 0;	    // sum of square 
    _ssqdif = 0.0;  // sum of square differences
#endif
}

//
// 
//
void Statistic::clear(double center, double precision) {
    clear();
#ifdef STAT_USE_STDEV
    _center = center;
    _precision = precision;
#endif
}

// adds a new value to the data-set
void Statistic::add(double value)
{
    long val = conv2internal(value);

    if (_cnt == 0) {
        _min = val;
        _max = val;
    } else {
        if (val < _min) _min = val;
        else if (val > _max) _max = val;
    }
    _sum += val;
    _cnt++;

#ifdef STAT_USE_STDEV
    if( _overflow_cnt == 0 && 
          val <= SQRT_SLONG_MAX && val >= -SQRT_SLONG_MAX &&
          val * val <= SLONG_MAX - _ssq && 
          _sum_i + val <= SQRT_SLONG_MAX && _sum_i + val >= -SQRT_SLONG_MAX ) { // not to be overflow
        _ssq += val * val;
        _sum_i += val;
        _ssqdif = _ssq - _sum_i*_sum_i/_cnt;
//        _ssqdif = _ssq - (_sum_i/_cnt*_sum_i + (_sum_i%_cnt)*_sum_i/_cnt);
    } else {
        if( _overflow_cnt == 0 ) { // just overflow
            _overflow_cnt = _cnt;
//Serial.print("overflow -- ");
//Serial.println(_cnt);
        }
        double _store = (_sum / _cnt - val);
        _ssqdif += _store * _store;
    }
#endif
}

void Statistic::dump() {
  Serial.print("_sum: "); Serial.println(_sum);
  Serial.print("_cnt: "); Serial.println(_cnt);
  Serial.print("_min: "); Serial.println(_min);
  Serial.print("_max: "); Serial.println(_max);
  Serial.print("_sum_i: "); Serial.println(_sum_i);
  Serial.print("_ssq: "); Serial.println(_ssq);
  Serial.print("_ssqdif: "); Serial.println( _ssqdif);
  Serial.print("_overflow_cnt: "); Serial.println(_overflow_cnt);
}



// returns the average of the data-set added sofar
double Statistic::average()
{
    if (_cnt == 0) return NAN; // original code returned 0
    return conv2val(_sum / _cnt);
}

#ifdef STAT_USE_STDEV

// variance of sample
//    this is biased to variance of population
double Statistic::variance() {
    if (_cnt == 0) return NAN; // otherwise DIV0 error
    return conv2var(_ssqdif / _cnt);
}

// unbiased sample variance (in us-wiki)
//    expected value of this equals to the variance of the entire population
double Statistic::unbiased_variance() {
    if (_cnt <= 1) return NAN; // otherwise DIV0 error
    return conv2var(_ssqdif / (_cnt-1));
}

// standard deviation of sample
double Statistic::stdev() {
    if (_cnt == 0) return NAN; // otherwise DIV0 error
    return (sqrt( variance() ));
}

// sample standard deviation
//    this has a little bias
double Statistic::biased_stdev()
{
    if (_cnt <= 1) return NAN; // otherwise DIV0 error
    return (sqrt( unbiased_variance() ));
}

// nearly-unbiased standard deviation
double Statistic::unbiased_stdev() {
    if (_cnt <= 1) return NAN; // otherwise DIV0 error
    return conv2dif(sqrt( _ssqdif / (_cnt - 1.5)));
}

// return summary string
String Statistic::summary() {
  String out = String("ave ")+average()+", min "+minimum()+", max "+maximum()+", cnt "+count();
#ifdef STAT_USE_STDEV
  out += String(", stdev ")+stdev();
#endif
  return out;
}


#endif
// END OF FILE
