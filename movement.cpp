#include "movement.h"
#include "ll_helper.h"

// Constructor
// Takes the motor configuration.
// Any steps added will be validated against the motor configuration.
CMovement::CMovement(CMotorConfig& motorConfiguration) :
  m_motorConfig(motorConfiguration),
  m_stopped(false),
  m_accumulatedTicks(0)
{
}

// AddLinearMove adds a specific move to this movement.
// It is an error to add more than one move for a specific motor to any
// movement.
// Returns 0 on success
//        -ve on failure
int CMovement::AddLinearMove(MotorId motorId, int steps)
{
  if (m_stepData.find(motorId) != m_stepData.end())
  {
    return E_DUPLICATE_RESOURCE_MOVE;
  }

  m_stepData[motorId] = new MotorStepData(steps);

  return S_OK;
}

// Begin calculates all the internal state required for the movement.
// Returns 0 on success
//        -ve on failure
int CMovement::Begin(uint* pPeriodMultipler)
{
  // We need to work out what the Period Multiplier should be initially - it will change
  // to support acceleration of the head.  1 implies that the head is travelling as fast as possible.
  // Given the starting speed (ss), and knowing the top speed (ts) of a motor, the Period Multiplier
  // will be ts/ss

  // If we assume the acceleration is in terms of change in speed per period for a motor then we should be
  // able to recalculate the acceptable multiplier for a given motor every time we send the motor a Tick(true).

  // We should then return the largest multiplier for all the motors involved in the movement.

  // We only work in multiples of the base speed.  We make the base speed way too fast and run at something like 10 times slower than the base speed.  Using a multiplier of 16000 or something to get down to a slow speed.
  // e.g. with a timer frequency of 5.76kHz, 72 steps per rotation and 8x micro-stepping, a pitch of 5mm/rotation we can go from a speed of 10mm/s with a multiplier of 1 and 0.001mm/s with a multiplier of 10000.  By altering the timer frequency and the micro-steps we could easily change the top speed.  i.e. Changing to not using micro-stepping, we would immediately change the
  // top speed to 80mm/s.

  // Work out what the slowest motor will be, taking into account both the number
  // of steps and the acceleration/deceleration of the motor.
  uint maxTicks = 0;
  uint maxSpeedMultiplier = 0;
  for (MotorStepMap::iterator iter = m_stepData.begin(); iter != m_stepData.end(); ++iter)
  {
    CMotor *pMotor;
    if (m_motorConfig.GetMotor(iter->first, &pMotor) == S_OK)
    {
      uint distance = (uint)abs(iter->second->StepDefinition);
      uint acceleration = pMotor->Acceleration();
      uint speedMultiplier = pMotor->InitialSpeedMultiplier();
      uint ticks = pMotor->MinTicksPerStep();

      // Store the maximum number of ticks that are required
      maxTicks = max(distance * ticks, maxTicks);
      maxSpeedMultiplier = max(speedMultiplier, maxSpeedMultiplier);
      
      iter->second->AccelerationStepLimit = min((speedMultiplier - pMotor->MinSpeedMultiplier()) / acceleration, (distance * ticks) / 2);
    }
    else
    {
      return E_INVALID_MOTOR_ID;
    }
  }

  // maxTicks is the largest number of ticks to get to the end of the movement
  // everything else needs to be scaled relative to maxTicks.
  for (MotorStepMap::iterator iter = m_stepData.begin(); iter != m_stepData.end(); ++iter)
  {
    iter->second->TicksPerStep = maxTicks / iter->second->StepDefinition;
    iter->second->StepCount = 0;
    iter->second->TickCount = 0;

    // Set the direction for this movement for this motor
    CMotor *pMotor;
    if (m_motorConfig.GetMotor(iter->first, &pMotor) == S_OK)
    {
      pMotor->SetDirection(iter->second > 0);
      
      pMotor->SetCurrentSpeedMultiplier(pMotor->InitialSpeedMultiplier());
    }
  }
  
  *pPeriodMultipler = maxSpeedMultiplier;

  m_accumulatedTicks = 0;

  return S_OK;
}

// Tick executes a single tick within the movement.
// Returns 0 on success
//         1 on completion
//        -ve on failure
int CMovement::Tick(uint* pPeriodMultipler)
{
  // Start out assuming the movement is complete.
  int result = S_FALSE;
  
  if (!m_stopped)
  {
    uint maxSpeedMultiplier = 0;
    for (MotorStepMap::iterator iter = m_stepData.begin(); iter != m_stepData.end(); ++iter)
    {
      int tickCount = ++iter->second->TickCount;

      CMotor *pMotor;
      if (iter->second->StepCount >= iter->second->StepDefinition)
      {
        // This motor is complete
      }
      else if (m_motorConfig.GetMotor(iter->first, &pMotor) == S_OK)
      {
        // At least one motor is still going, so indicate the movement is still
        // in progress
        result = S_OK;
        
        if (tickCount >= iter->second->TicksPerStep)
        {
          ++iter->second->StepCount;
          iter->second->TickCount = 0;
          
          pMotor->Tick(true);

          // Should we start slowing down?
          if (iter->second->StepCount >= (iter->second->StepDefinition - iter->second->AccelerationStepLimit))  
          {
            // Time to start slowing down 
            uint initialSpeedMultiplier = pMotor->InitialSpeedMultiplier();
            uint speedMultiplier = pMotor->CurrentSpeedMultiplier();
            if (speedMultiplier < initialSpeedMultiplier)
            {
              speedMultiplier = min(initialSpeedMultiplier, speedMultiplier + pMotor->Acceleration());
              
              pMotor->SetCurrentSpeedMultiplier(speedMultiplier);
              
              maxSpeedMultiplier = max(initialSpeedMultiplier, maxSpeedMultiplier);
            }
            else
            {
              maxSpeedMultiplier = max(pMotor->CurrentSpeedMultiplier(), maxSpeedMultiplier);
            }
          }
          else 
          {
            uint minSpeedMultiplier = pMotor->MinSpeedMultiplier();
            uint speedMultiplier = pMotor->CurrentSpeedMultiplier();
            
            // Are we still accelerating?
            if (speedMultiplier > minSpeedMultiplier)
            {
              // Accelerating
              speedMultiplier = max(minSpeedMultiplier, speedMultiplier - pMotor->Acceleration());
              
              pMotor->SetCurrentSpeedMultiplier(speedMultiplier);
              
              maxSpeedMultiplier = max(minSpeedMultiplier, maxSpeedMultiplier);
            }
            else
            {
              maxSpeedMultiplier = max(pMotor->CurrentSpeedMultiplier(), maxSpeedMultiplier);
            }
          }
        }
        else
        {
          pMotor->Tick(false);
          iter->second->TickCount = tickCount;
          
          maxSpeedMultiplier = max(pMotor->CurrentSpeedMultiplier(), maxSpeedMultiplier);
        }
      }
    }
    
    *pPeriodMultipler = maxSpeedMultiplier;
  }

  return result;
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
