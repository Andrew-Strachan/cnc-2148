#include "motor.hpp"

void flash()
{
    if (FIO0PIN_bit.P0_8 & 1)
      FIO0CLR_bit.P0_8 = 1;
    else 
      FIO0SET_bit.P0_8 = 1;
}

__irq __arm void TMR1_IRQHandler(void) 
{
 if (T1IR_bit.MR0INT)
 {
   flash();
 }
 
 T1IR_bit.MR1INT = 1;
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
  
  // Configure Timer to toggle output and interrupt to call flash()
  T1TCR_bit.CE = 1;
  T1MCR_bit.MR1INT = 1; // Generate an interrupt
  T1MCR_bit.MR1RES = 1; // Reset TC
  T1EMR_bit.EM1 = 1; // Enable the output pin for MAT0.1/MAT1.1
  T1EMR_bit.EMC1 = 3; // Toggle the output pin
  T1PR = 0x0000ffff;
  T1MR1 = 0x80;  
  
  VICIntSelect = 0;
  VICIntEnable_bit.INT5 = 1;
  VICVectAddr8 = (unsigned long)TMR1_IRQHandler;
  VICVectCntl8 = 0x20 | 5;
  VICIntSelect_bit.INT5 = 1;
  
  __enable_interrupt();
  
  while(true)
  {
    if (FIO0PIN_bit.P0_22 & 1)
      FIO0CLR_bit.P0_22 = 1;
    else 
      FIO0SET_bit.P0_22 = 1;
    
    int j = 0;
    for (int i = 0; i < 0xffff; ++i)
    {
      j = i + i * j;
    }
    
    j = j - 1;
  }
  
  
  return 0;
}
