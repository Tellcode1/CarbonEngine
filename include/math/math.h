#ifndef __MATH_H__
#define __MATH_H__

const double cmPI = 3.1415926535897932385;
const double cm2PI = 6.283185307179586;
const double cmDEG2RAD_CONSTANT = 0.017453292519943295; // 2Pi / 360.0
const double cmRAD2DEG_CONSTANT = 57.29577951308232; // 1.0 / DEG2RAD_CONSTANT -> rad / DEG2RAD_CONSTANT

#define cmlerp(a, b, t) (a + ((b - a) * t))

#define cmmax(a, b) ((a) > (b) ? (a) : (b))
#define cmmin(a, b) ((a) < (b) ? (a) : (b))

#define cmclamp(x, min, max) (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))
#define cmclamp01(x) cmclamp((x), 0, 1)

#define cmdeg2rad(x) ((x) * cmDEG2RAD_CONSTANT)
#define cmrad2deg(x) ((x) * cmRAD2DEG_CONSTANT)

#endif//__MATH_H__