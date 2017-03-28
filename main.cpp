#include "c/intrinsics.h"
#include "nxp/iolpc2148.h"
#include "motor.h"
#include "movement.h"

typedef enum {
  None = 0x0,
  Tick = 0x1,
  Stop = 0x2,
  ConfigUpdated = 0x4,
  Go = 0x8,
  Running = 0x16
} ExecutionFlagValues;
typedef CFlags<ExecutionFlagValues> ExecutionFlags;

volatile ExecutionFlags g_flags = None;

void DefaultInterruptHandler(void)
{
return;
}

void Timer1_Handler(void)
{
  g_flags = Tick;
  //FIO0CLR_bit.P0_23 = 1;
  //T1MCR_bit.MR1INT = 0;

  if (T1IR_bit.MR1INT)
  {

  }
  if (T1IR_bit.MR0INT)
  {

  }

  T1IR = 0x3;
}

int main()
{

  // Configure a pin with an LED on it as an OUTPUT and flash it
  SCS_bit.GPIO0M = 1;  // Enable fast GPIO on port 0

  // Configure P0.13 to be MAT1.1 output
  PINSEL0_bit.P0_13 = 0x2;
  PINSEL0_bit.P0_12 = 0x2;
  PINSEL1_bit.P0_22 = 0x3;

  // Configure outputs
  FIO0DIR_bit.P0_23 = 1;
  FIO0DIR_bit.P0_21 = 1;
  FIO0DIR_bit.P0_8 = 1;

  FIO0DIR_bit.P0_14 = 1;
  FIO0DIR_bit.P0_15 = 1;
  FIO0DIR_bit.P0_16 = 1;
  FIO0DIR_bit.P0_17 = 1;

  // Disable interrupts while we set things up
  VICIntSelect = 0;
  VICIntEnClear = 0xFFFFFFFF;
  VICVectAddr = 0x0;

  PLLCON_bit.PLLE = 1;
  PLLCFG = 0x24;

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
  PLLCON = 0x3;

  // Perform a feed to set the PLL parameters
  PLLFEED = 0xAA;
  PLLFEED = 0x55;

  VPBDIV = 0x01;

  // Initialize debug LED ports - flash LED during setup stage

  // Setup GPIO
  SCS_bit.GPIO0M = 1; // Enable FIO on port 0
  PINSEL0_bit.P0_12 = 0x2;  // Enable MAT1.1 on pin 0.12
  FIO0DIR_bit.P0_23 = 1;
  FIO0DIR_bit.P0_22 = 1;


  FIO0SET_bit.P0_23 = 1;

  // Setup timers
  T1MCR = 0;
  T1TCR = 0;
  T1TC = 0;
  T1PC = 0;
  T1IR = 0;
  T1TCR_bit.CR = 1;  // Reset and hold the TC and PC at 0
  T1TCR_bit.CE = 1;
  T1PR = 0x100;
  T1MR0 = 1000;
  T1MR1 = 50;
  T1MCR_bit.MR1INT = 1;
  T1MCR_bit.MR1RES = 1;
  T1MCR_bit.MR1STOP = 0;
  T1MCR_bit.MR0INT = 0;
  T1MCR_bit.MR0RES = 0;
  T1MCR_bit.MR0STOP = 0;
  T1EMR = 0;
  T1EMR_bit.EM1 = 1;
  T1EMR_bit.EMC1 = 0x3;
  T1EMR_bit.EM0 = 1;
  T1EMR_bit.EMC0 = 0x3;

  T0TCR_bit.CE = 0;

  // Setup interrupts
  VICDefVectAddr = (unsigned)DefaultInterruptHandler;
  VICVectAddr4 = (unsigned)Timer1_Handler;
  VICVectCntl4 = 0x20 | 5; // Timer1 will trigger Int Vector 4

  // Enable interrupts and start the timers
  __enable_irq();

  VICIntEnClear = 0x20;
  VICIntEnable = 0x20;

  T1TCR_bit.CR = 0; // Release TC and PC

  // Setup USB communication

  // Initialize motor drivers
  CMotor *motor = new CMotor(10, 1, 10, 2, &FIO0PIN, 14, &FIO0PIN, 15, &FIO0PIN, 16);
  motor->SetDirection(false);

  CMotorConfig motorConfig;
  motorConfig.AddMotor(X_Axis, motor);

  CMovement *movement = new CMovement(motorConfig);

  movement->AddLinearMove(X_Axis, 1000);

  movement->Begin();

  // Initialize LCD drivers - if applicable

  // Initialize SD card drivers and filesystem

  int i = 0;
  int tickCount = 0;
  long push = 1;
  int debounceCount = 0;
  bool direction = 0;
  while (true)
  {
    if (++i > 0x80000)
    {
      i = 0;
      if (FIO0PIN_bit.P0_21 & 1)
      {
        FIO0CLR_bit.P0_21 = 1;
      }
      else
        FIO0SET_bit.P0_21 = 1;

      direction = !direction;
    }
    if (FIO0PIN_bit.P0_30 == push)
    {
      if (++debounceCount > 50)
      {
        if (push)
        {
          direction = !direction;
          motor->SetDirection(direction);

          FIO0PIN_bit.P0_17 = direction;
        }
        else
        {
          // Just releasing the button.
        }

        push = !push;
      }
    }
    else
    {
      debounceCount = 0;
    }

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
      if (FIO0PIN_bit.P0_23 & 1)
        FIO0CLR_bit.P0_23 = 1;
      else
        FIO0SET_bit.P0_23 = 1;

      while(movement->Tick() != S_FALSE)
      {

      }

      g_flags = g_flags & ~Tick;
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
}

