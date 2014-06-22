/*
  new_mathematics.cpp implements replacements with much better performance for some trigonometric functions
  sin,  cos,  tan, atan2  and sqrt 
  can now be replaced with
  fsin, fcos, ftan, fatan2 and fsqrt.

  Thanks to Oscar Liang for his work on this (source: http://blog.oscarliang.net/enhanced-arduino-c-custom-math-library/).
  Integration into Marlin, transformation to RAD-parameters and math_tables by Robert Kuhlmann.

  June, 2014, RobertKuhlmann, Germany
*/

#ifndef new_mathematics_h
#define new_mathematics_h

float fsin(float rad); 
float fcos(float rad);
float facos(float num);
float fatan2(float opp, float adj);
float fsqrt(float num);
//double square(float f);

#define square(x) ((x)*(x))

#endif

