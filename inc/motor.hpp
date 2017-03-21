#ifndef __MOTOR_HPP__
#define __MOTOR_HPP__

#include "nxp\iolpc2148.h"
#include "ll_helper.h"

class CMotor
{
private:
  Port m_stepPort;
  uint m_stepPin;
  Port m_dirPort;
  uint m_dirPin;
  uint m_ticksPerStep;
  uint m_dutyCycleTicks;
  
public:
  CMotor(Port stepPort, uint stepPin, Port dirPort, uint dirPin, uint ticksPerStep, uint dutyCycleTicks);
  
  void SetDirection(bool positive);
  
  void Tick();
};

#endif