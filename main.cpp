#include "includes.h"
#include "flags.h"
#include "motor.h"
#include "movement.h"
#include "jogmovement.h"
#include "includes.h"

typedef enum {
  ExecutionNone = 0x0,
  Tick = 0x1,
  Stop = 0x2,
  ConfigUpdated = 0x4,
  Go = 0x8,
  Running = 0x16
} ExecutionFlags;
//typedef CFlags<ExecutionFlagValues> ExecutionFlags;

volatile ExecutionFlags g_flags = ExecutionNone;

void DefaultInterruptHandler(void)
{
return;
}

void Timer0_Handler(void)
{
  g_flags = Tick;
  //FIO0CLR_bit.P0_23 = 1;
  //T1MCR_bit.MR1INT = 0;

  if (T0IR_bit.MR1INT)
  {

  }
  if (T0IR_bit.MR0INT)
  {

  }

  T0IR = 0x3;
}

int main()
{
  Boolean CdcConfigureStateHold;

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
  FIO0DIR_bit.P0_18 = 1;  
  FIO0DIR_bit.P0_19 = 1;
  FIO0DIR_bit.P0_9 = 1;
  FIO0DIR_bit.P0_10 = 1;  
  FIO0DIR_bit.P0_11 = 1;

  FIO0DIR_bit.P0_30 = 0;
  
  // Disable interrupts while we set things up
  VICIntSelect = 0;
  VICIntEnClear = 0xFFFFFFFF;
  VICVectAddr = 0x0;

  PLLCON_bit.PLLE = 1;
  
  PLLCFG_bit.MSEL = 0x4;
  PLLCFG_bit.PSEL = 0x1;

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
  T0MCR = 0;
  T0TCR = 0;
  T0TC = 0;
  T0PC = 0;
  T0IR = 0x3;
  T0TCR_bit.CR = 1;  // Reset and hold the TC and PC at 0
  T0TCR_bit.CE = 1;
  T0PR = 0x40;
  T0MR0 = 1000;
  T0MR1 = 50;
  T0MCR_bit.MR1INT = 0;
  T0MCR_bit.MR1RES = 0;
  T0MCR_bit.MR1STOP = 0;
  T0MCR_bit.MR0INT = 1;
  T0MCR_bit.MR0RES = 1;
  T0MCR_bit.MR0STOP = 0;
  T0EMR = 0;
  T0EMR_bit.EM1 = 1;
  T0EMR_bit.EMC1 = 0x3;
  T0EMR_bit.EM0 = 1;
  T0EMR_bit.EMC0 = 0x3;

  T1TCR_bit.CE = 0;

  // Setup interrupts
  VICDefVectAddr = (unsigned)DefaultInterruptHandler;
  VICVectAddr4 = (unsigned)Timer0_Handler;
  VICVectCntl4 = 0x20 | 4; // Timer0 will trigger Int Vector 4


  VICIntEnClear = 0x10;
  VICIntEnable |= (1 << VIC_TIMER0);

  

  // Setup USB communication
  UsbCdcInit();

  // Enable interrupts 
  __enable_irq();

  // Soft connection enable
  USB_ConnectRes(TRUE);
  
  CdcConfigureStateHold = !IsUsbCdcConfigure();
 
  // Initialize motor drivers
  CMotor *motor = new CMotor(10, 1000, 10, 5, 2, &FIO0PIN, 14, &FIO0PIN, 15, &FIO0PIN, 16);
  motor->SetDirection(false);
  CMotor *yMotor = new CMotor(10, 200, 10, 5, 2, &FIO0PIN, 17, &FIO0PIN, 18, &FIO0PIN, 19);
  motor->SetDirection(false);
  CMotor *zMotor = new CMotor(10, 200, 10, 5, 2, &FIO0PIN, 9, &FIO0PIN, 10, &FIO0PIN, 11);
  motor->SetDirection(false);  

  int xLength = -5000;
  int xLengthChange = 2000;

  CMotorConfig motorConfig;
  motorConfig.AddMotor(X_Axis, motor);
  motorConfig.AddMotor(Y_Axis, yMotor);  
  motorConfig.AddMotor(Z_Axis, zMotor);
  
  CJogMovement *jogMovement = new CJogMovement(motorConfig);
  CMovement *movement = jogMovement;

  unsigned speedMultiplier = 0;
  movement->Begin(&speedMultiplier);
  
  const unsigned baseSpeed = 5;
  
  T0MR0 = baseSpeed * speedMultiplier;
  
  T0TCR_bit.CR = 0; // Release TC and PC 
  
  
  // Initialize LCD drivers - if applicable

  // Initialize SD card drivers and filesystem

  int i = 0;
  int tickCount = 0;
  long push = 1;
  int debounceCount = 0;
  bool direction = 0;
  while (true)
  {
    if (FIO0PIN_bit.P0_30 == push)
    {
      if (++debounceCount > 50)
      {
        if (push)
        {
          direction = !direction;
          motor->SetDirection(direction);

          FIO0PIN_bit.P0_17 = direction;
          
          movement->Begin(&speedMultiplier);
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
      g_flags = g_flags & ~Tick;

      // Perform the next tick.  This just means that our timer went off, it doesn't necessarily
      // indicate that a step is due for any particular axis/motor.
      if (movement)
      {
        int result = movement->Tick(&speedMultiplier);
        if (result == S_OK)
        {
          T0MR0 = baseSpeed * speedMultiplier;
          
          if (++i > 0x20)
          {
            i = 0;
            if (FIO0PIN_bit.P0_21 & 1)
            {
              FIO0CLR_bit.P0_21 = 1;
            }
            else
            {
              FIO0SET_bit.P0_21 = 1;
            }
          }
        }
        else if (result == S_FALSE)
        {
          // Movement is finished.
          T0TCR_bit.CR = 1;
          FIO0SET_bit.P0_21 = 1;
          
          // TODO: We should be running a program so 
          // get the next step in the program

          // movement = new CMovement(motorConfig);

          //movement->AddLinearMove(X_Axis, xLength);
          //movement->AddLinearMove(Y_Axis, 3000);  
          //movement->AddLinearMove(Z_Axis, 2500);  
          
          //movement->Begin(&speedMultiplier);
          
          //T0MR0 = baseSpeed * speedMultiplier;
          
          //T0TCR_bit.CR = 0;
        }
        else
        {
          // We hit an error
          g_flags |= Stop;
          movement = jogMovement = new CJogMovement(motorConfig);
          
        }
      }
      
      if (g_flags & Tick != 0)
      {
        // We took too long to calculate the next tick period
        //g_flags |= Stop;
      }
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
    
    char controlMessageBuffer[64];
    int bytesRead = UsbCdcRead((Int8U*)controlMessageBuffer, sizeof(controlMessageBuffer)/sizeof(controlMessageBuffer[0]));
    if (bytesRead > 0)
    {
      // Check for a valid message
      if (controlMessageBuffer[0] == 'J' &&
          controlMessageBuffer[1] == 'O' &&
          controlMessageBuffer[2] == 'G' &&
          controlMessageBuffer[3] == ' ')
      {
          JogMoveFlags move = (JogMoveFlags)controlMessageBuffer[4];
          
          jogMovement->SetAxes(move);
      }
      
      UsbCdcWrite((Int8U*)controlMessageBuffer, 1);
    }
  }
}

