#include "movement.h"
#include "ll_helper.h"

// Constructor
// Takes the motor configuration.
// Any steps added will be validated against the motor configuration.
CMovement::CMovement(CMotorConfig& motorConfiguration) :
  m_motorConfig(motorConfiguration),
  m_stopped(false)
{
}

// AddLinearMove adds a specific move to this movement.
// It is an error to add more than one move for a specific motor to any
// movement.
// Returns 0 on success
//        -ve on failure
int CMovement::AddLinearMove(MotorId motorId, int steps)
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
int CMovement::Begin(uint* pPeriodMultipler)
{
  We need to work out what the Period Multiplier should be initially - it will change
  to support acceleration of the head.  1 implies that the head is travelling as fast as possible.
  Given the starting speed (ss), and knowing the top speed (ts) of a motor, the Period Multiplier
  will be ts/ss

  If we assume the acceleration is in terms of change in speed per period for a motor then we should be
  able to recalculate the acceptable multiplier for a given motor every time we send the motor a Tick(true).

  We should then return the largest multiplier for all the motors involved in the movement.

  // Work out what the slowest motor will be, taking into account both the number
  // of steps and the acceleration/deceleration of the motor.
  uint maxTicks = 0;
  for (MotorStepMap::iterator iter = m_stepDefinition.begin(); iter != m_stepDefinition.end(); ++iter)
  {
    CMotor *pMotor;
    if (m_motorConfig.GetMotor(iter->first, &pMotor) == S_OK)
    {
      uint distance = (uint)abs(iter->second);
      uint acceleration = pMotor->Acceleration();
      uint speed = pMotor->MaxStepsPerSecond();
      uint ticks = pMotor->MinTicksPerStep();

      // Work out how long it'll take to perform the number of steps we have
      // How many steps does it take to reach maximum speed (and then deccelerate to 0 again)
      // acceleration is steps/second/second
      // speed is steps/second
      uint halfway = (distance + 1) / 2;

      uint distanceInTime = 0;
      uint t = 0;
      do
      {
        ++t;

        distanceInTime = t * t * acceleration + ((t > (speed / acceleration)) ? (t - (speed / acceleration)) * speed : 0);

      } while (distanceInTime < halfway);

      // Store the maximum number of ticks that are required
      maxTicks = max(distance * ticks, maxTicks);
    }
    else
    {
      return E_INVALID_MOTOR_ID;
    }
  }

  // maxTicks is the largest number of ticks to get to the end of the movement
  // everything else needs to be scaled relative to maxTicks.
  for (MotorStepMap::iterator iter = m_stepDefinition.begin(); iter != m_stepDefinition.end(); ++iter)
  {
    m_ticksPerStep[iter->first] = maxTicks / iter->second;

    // Set the direction for this movement for this motor
    CMotor *pMotor;
    if (m_motorConfig.GetMotor(iter->first, &pMotor) == S_OK)
    {
      pMotor->SetDirection(iter->second > 0);
    }
  }

  return S_OK;
}

// Tick executes a single tick within the movement.
// Returns 0 on success
//         1 on completion
//        -ve on failure
int CMovement::Tick(uint* pPeriodMultipler)
{
  if (!m_stopped)
  {
    for (MotorStepMap::iterator iter = m_ticksPerStep.begin(); iter != m_ticksPerStep.end(); ++iter)
    {
      int tickCount = ++m_tickCount[iter->first];

      CMotor *pMotor;
      if (m_motorConfig.GetMotor(iter->first, &pMotor) == S_OK)
      {
        if (tickCount >= iter->second)
        {
          pMotor->Tick(true);
          m_tickCount[iter->first] = 0;
        }
        else
        {
          pMotor->Tick(false);
          m_tickCount[iter->first] = tickCount;
        }
      }
    }
  }

  return S_OK;
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
