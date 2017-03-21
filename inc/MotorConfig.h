#ifndef __MOTORCONFIG_HPP__
#define __MOTORCONFIG_HPP__

#include "motor.hpp"

typedef tagMotorId {
  X_Axis = 1,
  Y_Axis = 2,
  Z_Axis = 3,
  Rotate_Axis = 4,
  FilamentFeed_1 = 5,
  FilamentFeed_2 = 6,
  FilamentFeed_3 = 7
} MotorId;

class CMotorConfig
{
  int AddMotor(MotorId motorId, CMotor &motor);
  
  int GetMotor(MotorId motorId, CMotor *pMotor);
}

#endif // __MOTORCONFIG_HPP__
