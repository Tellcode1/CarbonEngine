#ifndef __LUNA_ERR_H__
#define __LUNA_ERR_H__

typedef enum lerrc {
  LUNA_ERR_UNKOWN = -1,

  LUNA_ERR_NONE = 0,

  // An invalid parameter has been passed to a function
  LUNA_ERR_INVALID_PARAM,

  // Function returned invalid 
  LUNA_ERR_INVALID_RET,

  // Internal state error, most likely not the user's (developer's) fault
  LUNA_ERR_INTERNAL,

  // A memory allocation has failed. The OS is likely out of memory
  LUNA_ERR_MALLOC,

  // memory has been written to at invalid positions.
  // a buffer overflow basically
  LUNA_ERR_INVALID_MEMORY,
} lerrc;

#endif //__LUNA_ERR_H__