MyStatistic
===========

MyStatistic is a wrapper class for Rob Tillaart's [Statistic](https://github.com/RobTillaart/Arduino/tree/master/libraries/Statistic),
which is a recursive statistical library for [Arduino IDE](https://www.arduino.cc/).   
This library adds only a few methods to the original.

## Description

* Data types

  This library uses type `float` to take sample values and return statistical values (average, sum, etc.).

    * Floating-point Data Types    
      In many MCU, type `float` and `double` have storage of 32 bits (4 bytes), 
      so the numbers can be as large as 3.4028235E+38 and as low as -3.4028235E+38,
      and have only 6-7 decimal digits of precision.   
      Please refer [Arduino Reference](https://www.arduino.cc/en/Reference/Double).   

    * Integer Data Types    
      In many MCU, type `long` have storage of only 32 bits (4 bytes), 
      so the numbers can be as large as 2,147,483,647 and as low as -2,147,483,648.   
      Please refer [Arduino Reference](https://www.arduino.cc/en/Reference/Long).   

* Algorithm for calculation of variance and standard deviation.

  The Statistic library uses a numarically stable, single-pass (online) algorithm like [Welford's method](https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance).

* [Numerical error](https://en.wikipedia.org/wiki/Numerical_error)

  * Input and output values has only 6-7 decimal digits of precision due to using `float` type (they have round-off error).
  * Results of sum, average, variance, and standard deviation are not free from the [accumulation of small errors](https://en.wikipedia.org/wiki/Kahan_summation_algorithm), 
    such that the round-off error grows propotional to sample size n in worst case, 
    when it sums sample values or squares of difference of sample value from the mean.


## API reference

#### Constructor

```
MyStatistic stat;             
MyStatistic stat(100.0, 0.1); // for backward compatibility. The arguments have no effect.
```

#### Clear

```
stat.clear();           
stat.clear(100.0, 0.1); // for backward compatibility. The arguments have no effect.
```

Remove all the sampled values.

#### Add sample value

```
stat.add(120.11); // value is sampled as float type
```

#### Calculation of statistics

```
unsigned long c = stat.count(); 
double su = stat.sum();
double mi = stat.minimum();
double ma = stat.maximum();
double av = stat.average();
```

Returns the number of samples, sum, minimum, maximum, and average value of the sampled values.

```
double s = stat.stdev();       // alias of pop_stdev()
double s = stat.pop_stdev();
```

`stdev()` retruns the standard deviation of the sampled values.
This is called as population standard deviation or as *uncorrected* or *biased* sample stadard deviation,
since this is a downward-biased estimate of the standard deviation of the population 
(its value tends to be lower than the population standard deviation when the sample size is small).
Please refer [wiki](https://en.wikipedia.org/wiki/Standard_deviation).

```
double v = stat.variance();
```

`variance()` retruns the variance of the sampled values.
This is a biased estimator for the variance of the population.
Please refer [wiki](https://en.wikipedia.org/wiki/Standard_deviation).

```
double s2 = stat.unbiased_stdev();
```

`unbiased_stdev()` retruns [(*unbiased*) sample standard deviation](https://en.wikipedia.org/wiki/Standard_deviation).
This is nearly-unbiased estimator for the standard deviation of the population.    

#### Misc.

```
String text = stat.summary();
```

`summary()` returns a formated text like  `ave 4.5, min 2.3, max 5.6, cnt 213, stdev 0.32`.   

## Examples

```
to be seen
```

## Technical note

To calculate variance and standard deviation in one pass (allowing each value to be discarded), 
the following formula is commonly used:

```
(1) variance(X) = E((X-E(X))^2) = E(X^2) - E(X)^2
```

However, calculation of the formula causes floating point calculation error (for small differece between large values) when sampling data is large and the variance is small.  
So, the following approximate iterative calculation formula can be used instead.

```
(2) VN[i+1] = VN[i] + (xi - mi)^2    (xi: i-th sample value, mi: avarage of first i sample values)
    variance = VN[N] / N            (N: number of samples)
```

This formula is numerically stable, but potantially includes error 
due to using mi (average of first i samples) instead of average of all the sampling values.

Gil Ross's one-path algorithm in original [Statistic](https://github.com/RobTillaart/Arduino/tree/master/libraries/Statistic)
library (=(2)) is numerically stable, but not so accurate when the number of samples is small as mentioned by himself.
Enhanced algorithm uses accurate calculation using (1) with `long` type representation for small number of iteration, 
and uses the iterative algorithm (2) with `double` type variables for long number of iterations.  
The algorighm is automatically switched just before internal overflow of `long` type variables.

