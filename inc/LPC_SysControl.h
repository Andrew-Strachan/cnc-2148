/*************************************************************************
 *
 *    Used with ICCARM and AARM.
 *
 *    (c) Copyright IAR Systems 2003
 *
 *    File name   : LPC_SysControl.h
 *    Description :
 *
 *    History :
 *    1. Date:  August 17, 2004
 *       Author: Shawn Zhang
 *       Description: Create the basic function
 *
 *    2. Date        : Oct 7, 2004
 *       Author      : Stanimir Bonev
 *       Description : Modify some function and interface
 *
 *    $Revision: 28 $
 **************************************************************************/

#include "includes.h"

#ifndef __LPC_SYSCONTROL_H
#define __LPC_SYSCONTROL_H

/* Fosc range: 10MHz ~ 25MHz */
#define Fosc_MIN            10000000L
#define Fosc_MAX            25000000L

/* Fcclk range: 10MHz ~ MCU allowed frequency */
#define Fcclk_MIN           10000000L
#define Fcclk_MAX           60000000L

/* Fcco range: 156MHz ~ 320MHz */
#define Fcco_MIN            156000000L
#define Fcco_MAX            320000000L

#define PLLFEED_DATA1       0xAA
#define PLLFEED_DATA2       0x55

/* PLL PLLCON register bit descriptions */
#define PLLCON_ENABLE_BIT   0
#define PLLCON_CONNECT_BIT  1

/* PLL PLLSTAT register bit descriptions */
#define PLLSTAT_ENABLE_BIT  8
#define PLLSTAT_CONNECT_BIT 9
#define PLLSTAT_LOCK_BIT    10

/* PM Peripheral Type */
#define PC_TIMER0           (1UL <<  1)
#define PC_TIMER1           (1UL <<  2)
#define PC_UART0            (1UL <<  3)
#define PC_UART1            (1UL <<  4)
#define PC_PWM0             (1UL <<  5)
#define PC_I2C0             (1UL <<  7)
#define PC_SPI0             (1UL <<  8)
#define PC_RTC              (1UL <<  9)
#define PC_PCSPI1           (1UL << 10)
#define PC_ADC0             (1UL << 12)
#define PC_I2C1             (1UL << 19)
#define PC_ADC1             (1UL << 20)
#define PC_USB              (1UL << 31)


// External interrupt
typedef enum {
    EXTINT0 = 0,
    EXTINT1,
    EXTINT2,
    USBWAKE,
    BODWAKE,
    RTCWAKE,
} LPC_SysControl_ExtInt_Channel_t;

// Memory Mapping Control Mode
typedef enum {
    BOOT_LOADER = 0,
    USER_FLASH,
    USER_RAM
} LPC_SysControl_RemapMode_t;

// Memory Accelerator Module Operating Mode
typedef enum {
    VPBDIV4 = 0,  // Pclk = 1/4 Pcclk
    VPBDIV1,      // Pclk = Pcclk
    VPBDIV2       // Pclk = 1/2 Pcclk
} LPC_SysControl_VPBDiv_t;

// Memory Accelerator Module Operating Mode
typedef enum {
    MAM_PARTIALLY = 1,
    MAM_FULL
} LPC_SysControl_MAMMode_t;

// Power Manager Mode
typedef enum {
    PM_STANDARD = 0,
    PM_IDLE,
    PM_POWERDOWN
} LPC_SysControl_PMMode_t;

// External interrupt trigger type
typedef enum {
    LEVEL_LOW = 0,
    LEVEL_HIGH,
    EDGE_RISING,
    EDGE_FALLING
} LPC_SysControl_EXTINTTriggerType_t;

// MAM fetch cycles
// SysClock<20MHz: MAMTIM can be 001
// 20MHz<SysClock<40MHz: suggest 2 cycle
// 40MHz<SysClock: suggest 3 cycle
typedef enum {
    MAMCycle1 = 1,
    MAMCycle2,
    MAMCycle3,
    MAMCycle4,
    MAMCycle5,
    MAMCycle6,
    MAMCycle7
} LPC_SysControl_MAMCycle_t;

typedef struct {
  LPC_SysControl_MAMMode_t Mode;
  LPC_SysControl_MAMCycle_t Cycle;
} LPC_SysControl_MAMConfig_t;

typedef struct {
  unsigned long Fosc;   // Crystal Oscillator Frequency
  unsigned long Fcclk;    // PLL output frequency (also processor clock frequency)
  unsigned long Fpclk;    // Peripheral frequency

  LPC_SysControl_RemapMode_t RemapMode;
  LPC_SysControl_VPBDiv_t VPBDivider;

  bool MAMEnable;
  LPC_SysControl_MAMConfig_t MAMConfig;
} LPC_Syscontrol_Config_t;

// Declare API functions
int SYS_Init (unsigned long Fosc, unsigned long Fcclk,
                LPC_SysControl_VPBDiv_t VPBDivider,
                LPC_SysControl_RemapMode_t RemapMode,
                unsigned long PortDir0, unsigned long Port0,
                unsigned long PortDir1, unsigned long Port1);
unsigned int SYS_GetFpclk (void);

int PM_SetMode (LPC_SysControl_PMMode_t Mode);
int PM_OpenPeripheral (unsigned int DevType);
int PM_ClosePeripheral (unsigned int DevType);
int EXTINT_Init (LPC_SysControl_ExtInt_Channel_t ExtIntNum,
                          bool WakeupEnable);

#endif //__LPC_SYSCONTROL_H
