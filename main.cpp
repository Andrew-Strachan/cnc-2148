#include "iolpc2148.h"
#include "motor.hpp"

typedef enum {
  None = 0x0,
  Tick = 0x1,
  Stop = 0x2,
  ConfigUpdated = 0x4,
  Go = 0x8,
  Running = 0x16
} Flags;

volatile Flags g_flags = None;

inline Flags operator|(Flags a, Flags b)
{return static_cast<Flags>(static_cast<int>(a) | static_cast<int>(b));}
inline Flags operator&(Flags a, Flags b)
{return static_cast<Flags>(static_cast<int>(a) & static_cast<int>(b));}
inline Flags operator&=(Flags a, Flags b)
{ a = a & b; return a; }
inline Flags operator~(Flags a)
{return static_cast<Flags>(~static_cast<int>(a));}

__irq __arm void DefaultInterruptHandler(void)
{
  FIO0SET_bit.P0_22 = !FIO0PIN_bit.P0_22;
}

__irq __arm void Timer1_Handler(void)
{
  FIO0SET_bit.P0_23 = !FIO0PIN_bit.P0_23;
}

int main()
{

  // Configure a pin with an LED on it as an OUTPUT and flash it
  SCS_bit.GPIO0M = 1;  // Enable fast GPIO on port 0
  
  // Configure P0.12 to be MAT1.0 output
  PINSEL0_bit.P0_12 = 0x2;
  
  // Configure outputs
  FIO0DIR_bit.P0_22 = 1;
  FIO0DIR_bit.P0_8 = 1;
  
  // Disable interrupts while we set things up
  VICIntSelect = 0;
  
  PLLCON_bit.PLLE = 1;
  PLLCFG_bit.MSEL = 5;
  PLLCFG_bit.PSEL = 1;
  
  // Perform a feed to set the PLL parameters
  PLLFEED = 0xAA;
  PLLFEED = 0x55;
  
  while (!PLLSTAT_bit.PLOCK)
  {
    // Wait
    for (int i = 0; i < 20; i += 2)
    {
      --i;
    }
  }

  // We have PLL lock, now connect the PLL as the clock source
  PLLCON_bit.PLLC = 1;

  // Perform a feed to set the PLL parameters
  PLLFEED = 0x00;
  PLLFEED = 0xAA;
  PLLFEED = 0x55;

  // Initialize debug LED ports - flash LED during setup stage
  
  // Setup GPIO
  SCS_bit.GPIO0M = 1; // Enable FIO on port 0
  PINSEL0_bit.P0_12 = 0x2;  // Enable MAT1.1 on pin 0.12
  FIO0DIR_bit.P0_23 = 1;
  FIO0DIR_bit.P0_22 = 1;
  
  // Setup timers
  T1MCR = 0;
  T1TCR = 0;
  T1TC = 0;
  T1PC = 0;
  T1IR = 0;
  T1IR_bit.MR1INT = 1;
  T1TCR_bit.CR = 1;  // Reset and hold the TC and PC at 0
  T1TCR_bit.CE = 1;
  T1PR = 0x10;
  T1MR1 = 0x00001000;
  T1MCR_bit.MR1INT = 1;
  T1MCR_bit.MR1RES = 1;
  T1EMR = 0;
  T1EMR_bit.EM1 = 1;
  T1EMR_bit.EMC1 = 0x3;
  
  // Setup interrupts
  VICDefVectAddr = (unsigned long)DefaultInterruptHandler;
  VICVectAddr8 = (unsigned long)Timer1_Handler;
  VICVectCntl8 = 0x20 | 5; // Timer1 will trigger Int Vector 8
  VICIntEnable = 0x20;

  // Enable interrupts and start the timers
  VICIntSelect = 0x20;
  
  T1TCR_bit.CR = 0; // Release TC and PC
  
  // Setup USB communication
  
  // Initialize motor drivers
  
  // Initialize LCD drivers - if applicable
  
  // Initialize SD card drivers and filesystem

  while (true)
  {
    if (FIO0PIN_bit.P0_22 & 1)
      FIO0CLR_bit.P0_22 = 1;
    else 
      FIO0SET_bit.P0_22 = 1;
      
    if (g_flags & Stop)
    {
      // If we have received a Stop (which either comes from the user, or we hit a limit switch)
      // then we don't want to continue stepping.  Cancel the rest of the program.
      g_flags &= ~Tick;

      if (g_flags & Running)
      {
        // We're running so should stop.
        g_flags &= ~Running;
      }
    }
    if (g_flags & Tick)
    {
      // Perform the next tick.  This just means that our timer went off, it doesn't necessarily
      // indicate that a step is due for any particular axis/motor.
      //ExecuteTick();
    }
    if (g_flags & ConfigUpdated)
    {
      if (g_flags & Running)
      {
        // We're running but received updated configuration, so should stop.
        g_flags &= ~Running;
      }
    }
    if (g_flags & Go)
    {
      // If we have valid configuration then start the sequence
    }
  }

  return 0;
}

