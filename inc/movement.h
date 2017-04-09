#ifndef __MOVEMENT_HPP__
#define __MOVEMENT_HPP__

#include "MotorConfig.h"

class CMovement {
private:
  typedef struct _MotorStepData
  {
    _MotorStepData(uint steps, CMotor *pMotor) : 
      StepDefinition(steps),
      Motor(pMotor) { }
    
    int StepDefinition;
    int TicksPerStep;
    int TickCount;
    int StepCount;
    int AccelerationStepLimit;

    CMotor *Motor;
  } MotorStepData;
  
  CMotorConfig& m_motorConfig;

  MotorStepData** m_stepData;

  uint m_allocatedMotorSlots;
  uint m_usedMotorSlots;

  uint m_accumulatedTicks;
  bool m_stopped;

public:
  // Constructor
  // Takes the motor configuration.
  // Any steps added will be validated against the motor configuration.
  CMovement(CMotorConfig& motorConfiguration);

  // AddLinearMove adds a specific move to this movement.
  // It is an error to add more than one move for a specific motor to any
  // movement.
  // Returns 0 on success
  //        -ve on failure
  int AddLinearMove(MotorId motorId, int steps);

  // Begin calculates all the internal state required for the movement.
  // Returns 0 on success
  //        -ve on failure
  // Begin should return something that indicates to the caller what the initial
  // timer match register value should be.

  // If we consider a value of 1 to be the fastest speed we can travel at, then we'll have a
  // multiplier that is used to slow down the speed (i.e. increase the period).
  int Begin(uint* pPeriodMultipler);

  // Tick executes a single tick within the movement.
  // Tick has to handle the acceleration - it needs to return something that will enable
  // the caller to modify the timer match register to support acceleration.  The motor ticks
  // will just be called to get the movement correct - the timer match register will be used
  // to control the speed (and acceleration) of the movement.
  // See Begin for details on the period multiplier
  // Returns 0 on success
  //         1 on completion
  //        -ve on failure
  int Tick(uint* pPeriodMultipler);

  // Cancel stops the movement and handles any resets required.
  // Parameters:
  //    emergencyStop: if True then Emergency stop - just stop executing.
  //                   if False then stop, but follow the rules for decceleration
  void Cancel(bool emergencyStop);
};

#endif // __MOVEMENT_HPP__