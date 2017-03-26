#include "nxp\iolpc2148.h"

__arm __irq void IRQ_Handler(void) 
{
  unsigned long VectoredHandler = VICVectAddr;
  
  (*(void(*)())VectoredHandler)();
  
  VICVectAddr = 0x0;
  return;
}