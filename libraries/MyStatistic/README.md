MyStatistic
===========

MyStatistic is a wrapper class for 
This is an enhanced version of Rob Tillaart's [Statistic](https://github.com/RobTillaart/Arduino/tree/master/libraries/Statistic) v0.3.3.
class library for [Arduino IDE](https://www.arduino.cc/).  
* This library provides a class to calculate average, min, max, variances, and standard deviations of sampled data.
* An *hybrid* calculation method with automatic switching is employed.  
   * When the number of samples is small, it performs accurate calculation using internal `long` variables.
   * When the number of samples is large, it uses internal `double` variables 
     and approximate algorithm for variances and standard deviations.

## Description

* Internal representation of values

  To avoid precesion error of floating-point data type, use `long` type internal representation of sampling values.  
  User can specify the parameters `center` and `precision` values as arguments of construnctor or method `clear()`,
  which defines the conversion function as follows.
  
  ```
  internal_value = round( ( sample_value - center ) / precision )
  ```

  For example, when center = 100, precision = 0.1, sampled value 120.11 is converted to internal value 201.  
  Please notice that **the sample value is rounded** with the specified precision.

* Approximation

  When the following condition is met, internal `long` variables overflow, 
  and instead, internal `double` variables are used and approximate calculation algrithm is used for variance and standard deviation.

    * The sum of the sample values is out of range of data type `long`
    * The sum of squares of the sample values is out of range of data type `long`

* Overflow

  When the following condtion is met, internal `double` variable overflow, and the results are not guaranteed (depend on environment).

    * The sum of the sample values is out of range of data type `double`
    * The sum of squares of differences of sample values from the mean of sample values is out of range of data type `double`

* Data types (informative)
  
    * Floating-point Data Types    
      In many MCU, type `float` and `double` have storage of 32 bits (4 bytes), 
      so the numbers can be as large as 3.4028235E+38 and as low as -3.4028235E+38,
      and have only 6-7 decimal digits of precision.   
      Please refer [Arduino Reference](https://www.arduino.cc/en/Reference/Double).   

    * Integer Data Types    
      In many MCU, type `long` have storage of only 32 bits (4 bytes), 
      so the numbers can be as large as 2,147,483,647 and as low as -2,147,483,648.   
      Please refer [Arduino Reference](https://www.arduino.cc/en/Reference/Long).   

    Recomended to fit the internal represention of sampling values between about -4000 and 4000.

## API reference

#### Constructor

```
Statistic stat;             // center=0.0, precision=1.0 (default)
Statistic stat(100.0, 0.1); // center=100.0, precision=0.1
```

To avoid internal overflow and inaccurate calculation, 
please set appropriate values of center and precision so as to fit the range of internal representation (see above) of possible sample values within small range.
Setting `precision` to too small value likely causes overflow.
Setting `center` to a middle value in range of possible sampling values lower the overflow possibility.

#### Clear

```
stat.clear();           // center=0.0, precision=1.0 (default)
stat.clear(100.0, 0.1); // center=100.0, precision=0.1
```

Remove all the sampled values, and set `center` and `precision` (see above "Constructor").

#### Add sample value

```
stat.add(120.11); // value is sampled as double type
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
double s = stat.stdev();
```

`stdev()` retruns the standard deviation of the sampled values.
This is also called as *uncorrected* or *biased* sample stadard deviation,
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
double s1 = stat.biased_stdev();
double s2 = stat.unbiased_stdev();
double v1 = stat.unbiased_variance();
```

`biased_stdev()` is a square root of the `unbiased_variance()`.
This is also called as [corrected sample standard deviation](https://en.wikipedia.org/wiki/Standard_deviation), 
but is still a biased estimator for the population standard deviation.    
`unbiased_stdev()` retruns [*unbiased* sample standard deviation](https://en.wikipedia.org/wiki/Standard_deviation).
This is nearly-unbiased estimator for the standard deviation of the population.    
`unbiased_variance()` retruns [*unbiased* sample variance](https://en.wikipedia.org/wiki/Standard_deviation).
This is an unbiased estimator for the variance of the population,
which is given by applying Bessel's correction (replacing N with N-1) to the variance `variance()`.

#### Misc.

```
String text = stat.summary();
stat.dump();
```

`summary()` returns a formated text like  `ave 4.5, min 2.3, max 5.6, cnt 213, stdev 0.32`.   
`dump()` is debug print of internal variables (using Serial.print).

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

## License

Released to public domain 
(following to original Rob Tillaart's [Statistic](https://github.com/RobTillaart/Arduino/tree/master/libraries/Statistic) )
