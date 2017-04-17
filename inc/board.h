/***************************************************************************
 **
 **    This file defines the board specific definition
 **
 **    Used with ARM IAR C/C++ Compiler and Assembler.
 **
 **    (c) Copyright IAR Systems 2005
 **
 **    $Revision: 28 $
 **
 ***************************************************************************/
#include "arm_comm.h"

#ifndef __BOARD_H
#define __BOARD_H
/* OSC [MHz] */
#define FOSC                12MHZ

/* Core clk [MHz]
   When USB engine is used the Core clk must be more that 18MHz */
#define FCCLK               60MHZ
#define PCLK                60MHZ
#define MAM_STATE           TRUE
#define MAM_CONFIG          MAM_FULL
#define MAM_CYCLES          MAMCycle6

/* Time Precision time [us] */
#define TIMER_PRECISION     1

/* Sys timer tick per seconds */
#define TICK_PER_SECOND   100

#endif /* __BOARD_H */
