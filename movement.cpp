#include "movement.hpp"

// Constructor
// Takes the motor configuration.
// Any steps added will be validated against the motor configuration.
CMovement::CMovement(const CMotorConfig& motorConfiguration) :
  m_motorConfiguration(motorConfiguration)
{
}

// AddMove adds a specific move to this movement.
// It is an error to add more than one move for a specific motor to any
// movement.
// Returns 0 on success
//        -ve on failure
int CMovement::AddMove(MotorId motorId, int steps)
{
  if (m_stepDefinition.find(motorId) != m_stepDefinition.end())
  {
    return E_DUPLICATE_RESOURCE_MOVE;
  }

  m_stepDefinition[motorId] = steps;

  return S_OK;
}

// Begin calculates all the internal state required for the movement.
// Returns 0 on success
//        -ve on failure
int CMovement::Begin()
{
  // Work out what the slowest motor will be, taking into account both the number
  // of steps and the acceleration/deceleration of the motor.
  uint maxTicks = 0;
  for (MotorStepMap::iterator iter = m_stepDefinition.getIterator(); iter != m_stepDefinition.end(); ++iter)
  {
    const CMotor *pMotor;
    if (!m_motorConfiguration.getMotor(iter->first, *pMotor))
    {
      uint distance = (uint)abs(iter->second);
      uint acceleration = pMotor->Acceleration();
      uint speed = pMotor->MaxStepsPerSecond();
      uint ticks = pMotor->MinTicksPerStep();

      // Work out how long it'll take to perform the number of steps we have
      // How many steps does it take to reach maximum speed (and then deccelerate to 0 again)
      // acceleration is steps/second/second
      // speed is steps/second
      uint halfway = (distance + 1) \ 2;

      uint distanceInTime;
      uint t = 0;
      do
      {
        ++t;

        distance = t * t * acceleration + (t > (speed / acceleration)) ? (t - (speed / acceleration)) * speed : 0;

      } while (distanceInTime < halfway);

      // Store the maximum number of ticks that are required
      maxTicks = max(t * ticks, maxTicks);
    }
    else
    {
      return E_INVALID_MOTOR_ID;
    }
  }

  // maxTicks is the largest number of ticks to get to the end of the movement
  // everything else needs to be scaled relative to maxTicks.
  for (MotorStepMap::iterator iter = m_stepDefinition.getIterator(); iter != m_stepDefinition.end(); ++iter)
  {
    m_ticksPerStep[iter->first] = maxTicks / iter->second;

    // Set the direction for this movement for this motor
    const CMotor *pMotor;
    if (!m_motorConfiguration.getMotor(iter->first, *pMotor))
    {
      pMotor->setDirection(iter->second > 0);
    }
  }

  return S_OK;
}

// Tick executes a single tick within the movement.
// Returns 0 on success
//         1 on completion
//        -ve on failure
int CMovement::Tick()
{
  if (!m_stopped)
  {
    for (MotorStepMap::iterator iter = m_ticksPerStep.getIterator(); iter != m_ticksPerStep.end(); ++iter)
    {
      int tickCount = ++m_tickCount[iter->first];

      const CMotor *pMotor;
      if (!m_motorConfiguration.getMotor(iter->first, *pMotor))
      {
        if (tickCount >= iter->second)
        {
          pMotor->Tick(true);
          m_tickCount[iter->first] = 0;
        }
        else
        {
          pMotor->Tick(false);
        }
      }
    }
  }
}

// Cancel stops the movement and handles any resets required.
// Parameters:
//    emergencyStop: if True then Emergency stop - just stop executing.
//                   if False then stop, but follow the rules for decceleration
void CMovement::Cancel(bool emergencyStop)
{
  // For now, just perform an emergency stop always
  m_stopped = true;
}
