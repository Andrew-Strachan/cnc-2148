#ifndef __LL_HELPER_H__
#define __LL_HELPER_H__

//#include "nxp/iolpc2148.h"
#include "errors.h"

#ifndef NULL
  #define NULL (0) 
#endif
#ifndef __REG32
	#define __REG32 unsigned;
#endif

/*typedef union {
  struct {
    __REG32 P_0   : 1;
    __REG32 P_1   : 1;
    __REG32 P_2   : 1;
    __REG32 P_3   : 1;
    __REG32 P_4   : 1;
    __REG32 P_5   : 1;
    __REG32 P_6   : 1;
    __REG32 P_7   : 1;
    __REG32 P_8   : 1;
    __REG32 P_9   : 1;
    __REG32 P_10  : 1;
    __REG32 P_11  : 1;
    __REG32 P_12  : 1;
    __REG32 P_13  : 1;
    __REG32 P_14  : 1;
    __REG32 P_15  : 1;
    __REG32 P_16  : 1;
    __REG32 P_17  : 1;
    __REG32 P_18  : 1;
    __REG32 P_19  : 1;
    __REG32 P_20  : 1;
    __REG32 P_21  : 1;
    __REG32 P_22  : 1;
    __REG32 P_23  : 1;
    __REG32 P_24  : 1;
    __REG32 P_25  : 1;
    __REG32 P_26  : 1;
    __REG32 P_27  : 1;
    __REG32 P_28  : 1;
    __REG32 P_29  : 1;
    __REG32 P_30  : 1;
    __REG32 P_31  : 1;
  };
  unsigned long volatile P;
} PORT_BITS;*/
typedef volatile unsigned long *Port;
typedef unsigned int uint;

template <typename T> T max(const T &x, const T &y) { return (x < y) ? y : x; };

#endif