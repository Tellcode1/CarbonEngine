#ifndef __MATH_H__
#define __MATH_H__

static const double cmPI = 3.1415926535897932385;
static const double cm2PI = 6.283185307179586;
static const double cmDEG2RAD_CONSTANT = 0.017453292519943295; // 2Pi / 360.0
static const double cmRAD2DEG_CONSTANT = 57.29577951308232; // 1.0 / DEG2RAD_CONSTANT -> rad / DEG2RAD_CONSTANT

#define cmlerp(a, b, t) (a + ((b - a) * t))

#define cmmax(a, b) ((a) > (b) ? (a) : (b))
#define cmmin(a, b) ((a) < (b) ? (a) : (b))

#define cmclamp(x, min, max) (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))
#define cmclamp01(x) cmclamp((x), 0, 1)

// The constants aren't used because for static const variables, they can cause minor issues like not compiling at all.
#define cmdeg2rad(x) ((x) * 0.017453292519943295)
#define cmrad2deg(x) ((x) * 57.29577951308232)

#endif//__MATH_H__