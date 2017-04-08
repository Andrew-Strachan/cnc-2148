#ifndef __MOTOR_HPP__
#define __MOTOR_HPP__

#include "ll_helper.h"
#include "errors.h"

class CMotor
{
private:
    // Configuration Parameters
    uint m_ticksPerPulse;
    uint m_dutyTicks;
    uint m_minSpeedMultiplier;
    uint m_initialSpeedMultiplier;
    uint m_maxAcceleration;

    // PIN Parameters
    Port m_directionPort;
    uint m_directionBit;
    Port m_stepPort;
    uint m_stepBit;
    Port m_mirrorPort;
    uint m_mirrorBit;
    
    // Operating properties
    uint m_pulseTick;
    uint m_currentSpeedMultiplier;

public:
  CMotor(uint minSpeedMultiplier, uint initialSpeedMultiplier, uint maxAcceleration, uint ticksPerPulse, uint dutyTicks, Port directionPort, uint directionBit, Port stepPort, uint stepBit, Port mirrorPort = NULL, uint mirrorBit = 0);

  // Need to be able to configure when to step, how long the step pulse is.  Maybe how long the
  // direction pulse needs to be held after a step pulse starts.
  // [Should be able to set the entire set of motor pins in one go.]

  inline uint Acceleration() { return m_maxAcceleration; }
  inline uint MinSpeedMultiplier() { return m_minSpeedMultiplier; }
  inline uint InitialSpeedMultiplier() { return m_initialSpeedMultiplier; }
  inline uint CurrentSpeedMultiplier() { return m_currentSpeedMultiplier; }
  inline uint MinTicksPerStep() { return m_ticksPerPulse; }


  // SetDirection sets the direction for subsequent steps
  void SetDirection(bool positive);
  
  // SetCurrentSpeedMultiplier sets the current speed multiplier - used for storing
  // the current speed during execution.
  void SetCurrentSpeedMultiplier(uint speedMultiplier);

  // A Tick is one duty step.  i.e. If the duty cycle is 50% then one tick is
  // half the duty cycle.  If the duty cycle is 20% then one tick is 1/5th of the
  // duty cycle.
  // Parameters:
  //    step: Whether to initiate a step
  // Returns:
  //    HRESULT semantics:
  //            0 if the function succeeded.
  //            -ve if an error occurred.
  //            +ve if the function succeeded, but a return value is required.
  //
  // Notes:
  //    Tick will be called multiple times per step.  When step is true, a new pulse
  //    should be started (i.e. the step PIN should be set high).  When dutyTicks
  //    number of calls to Tick have occurred, the step PIN should be set low.
  //    If we get called with step == True before ticksPerPulse calls after a previous
  //    call with step == True then an error will be returned.
  int Tick(bool step);
};

#endif // __MOTOR_HPP__