/*
  new_mathematics.cpp implements replacements with much better performance for some trigonometric functions
  sin,  cos,  tan, atan2  and sqrt 
  can now be replaced with
  fsin, fcos, ftan, fatan2 and fsqrt.

  Thanks to Oscar Liang for his work on this (source: http://blog.oscarliang.net/enhanced-arduino-c-custom-math-library/).
  Integration into Marlin, transformation to RAD-parameters and math_tables by Robert Kuhlmann.

  June, 2014, RobertKuhlmann, Germany
*/

/*
  Useful stuff

  radians = (degrees * 71) / 4068

  degrees = (radians * 4068) / 71

  With floating point this is more accurate than using the "original" values
*/

//#include "math_tables.h"
#include <math.h>
#include "new_mathematics.h"

//float fsin(float rad)
//{
//	int deg = int((rad * 40680) / 71);
//	return SIN_TABLE[deg];
//} //float fsin(float rad)

//float fcos(float rad)
//{
//	int deg = int((rad * 40680) /71);
//	return COS_TABLE[deg];
//} //float fcos(float rad)

//float ftan(float rad)
//{
//	int deg = int((rad * 40680) /71);
//	return TAN_TABLE[deg];
//}//float ftan(float rad)

//float facos(float num)
//{
//	return ACOS_TABLE[2048*(num+1)];
//}//float facos(float pipart)

float fatan2(float opp, float adj)
{

	float hypt = fsqrt(adj * adj + opp * opp);
	float rad = acos(adj/hypt);

	if(opp < 0)
		rad = -rad;

	return rad;
}//float fatan2(float opp, float adj)

float fsqrt(float x)
{
    long val_int = *(long*)&x; /* Same bits, but as an int */
 
    val_int -= 127L << 23; /* Subtract 2^m. */
    val_int >>= 1; /* Divide by 2. */
    val_int += 127L << 23; /* Add ((b + 1) / 2) * 2^m. */
 
    return *(float*)&val_int; /* Interpret again as float */
}//float fsqrt(float x)

//double square(float f)
//{
//  return  ((double)f)*f;
//}
