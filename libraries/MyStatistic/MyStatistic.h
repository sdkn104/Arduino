#ifndef MyStatistic_h
#define MyStatistic_h

//
// This is a wrapper for Statistic (https://github.com/RobTillaart/Arduino/tree/master/libraries/Statistic).
//

#include "Statistic.h"

class MyStatistic : public Statistic {
public:
    MyStatistic( double center, double precision ) {} // for backward compatibility
    MyStatistic() {}

    float stdev() { return pop_stdev(); }

    String summary() {
        String out = String("ave ")+average()+", min "+minimum()+", max "+maximum()+", cnt "+count();
#ifdef STAT_USE_STDEV
        out += String(", stdev ")+stdev();
#endif
        return out;
    }
};

#endif

