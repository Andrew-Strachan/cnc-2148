#ifndef __MOVEMENT_HPP__
#define __MOVEMENT_HPP__

#include <map>
#include "MotorConfig.hpp"

class CMovement {
private:
  CMotorConfig& m_motorConfig;

  typedef std::map<MotorId, int> MotorStepMap;
  MotorStepMap m_stepDefinition;
  MotorStepMap m_ticksPerStep;
  MotorStepMap m_tickCount;

  bool m_stopped;

public:
  // Constructor
  // Takes the motor configuration.
  // Any steps added will be validated against the motor configuration.
  CMovement(const CMotorConfig& motorConfiguration);

  // AddLinearMove adds a specific move to this movement.
  // It is an error to add more than one move for a specific motor to any
  // movement.
  // Returns 0 on success
  //        -ve on failure
  int AddLinearMove(MotorId motorId, int steps);

  // Begin calculates all the internal state required for the movement.
  // Returns 0 on success
  //        -ve on failure
  int Begin();

  // Tick executes a single tick within the movement.
  // Returns 0 on success
  //         1 on completion
  //        -ve on failure
  int Tick();

  // Cancel stops the movement and handles any resets required.
  // Parameters:
  //    emergencyStop: if True then Emergency stop - just stop executing.
  //                   if False then stop, but follow the rules for decceleration
  void Cancel(bool emergencyStop);
}

#endif // __MOVEMENT_HPP__