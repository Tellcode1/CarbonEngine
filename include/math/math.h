#ifndef __MATH_H__
#define __MATH_H__

// You wanna know why everything has an m in front of it?
// msdfgen doesn't compile without them because it replaces msdfgen's clamp, etc with ours.
// lovely

const double mPI = 3.1415926535897932385;
const double mDEG2RAD_CONSTANT = 0.017453292519943295; // 2Pi / 360.0
const double mRAD2DEG_CONSTANT = 57.29577951308232; // 1.0 / DEG2RAD_CONSTANT -> rad / DEG2RAD_CONSTANT

#define mlerp(a, b, t) (a + ((b - a) * t))
#define mclamp(x, min, max) ((x < min) ? min : ((x > max) ? max : x))
#define mdeg2rad(f) (f * mDEG2RAD_CONSTANT)
#define mrad2deg(f) (f * mRAD2DEG_CONSTANT)

#endif//__MATH_H__