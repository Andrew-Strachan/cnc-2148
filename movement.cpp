#include "movement.h"
#include "ll_helper.h"

// Constructor
// Takes the motor configuration.
// Any steps added will be validated against the motor configuration.
CMovement::CMovement(CMotorConfig& motorConfiguration) :
  m_motorConfig(motorConfiguration),
  m_stopped(false),
  m_accumulatedTicks(0),
  m_allocatedMotorSlots(3),
  m_usedMotorSlots(0)
{
  m_stepData = (MotorStepData**)malloc(m_allocatedMotorSlots * sizeof(MotorStepData*));
  
  memset(m_stepData, 0, m_allocatedMotorSlots * sizeof(MotorStepData*));
}

// AddLinearMove adds a specific move to this movement.
// It is an error to add more than one move for a specific motor to any
// movement.
// Returns 0 on success
//        -ve on failure
int CMovement::AddLinearMove(MotorId motorId, int steps)
{
  CMotor *pMotor;
  if (m_motorConfig.GetMotor(motorId, &pMotor) != S_OK) 
  {
      return E_INVALID_MOTOR_ID;
  }

  for (uint index = 0; index < m_usedMotorSlots; ++index)
  {
    if (m_stepData[index]->Motor == pMotor)
    {
      return E_DUPLICATE_RESOURCE_MOVE;
    }
  }

  if (m_usedMotorSlots == m_allocatedMotorSlots)
  {
    m_allocatedMotorSlots += 3;

    m_stepData = (MotorStepData**)realloc(m_stepData, m_allocatedMotorSlots * sizeof(MotorStepData*));
  }

  m_stepData[m_usedMotorSlots++] = new MotorStepData(steps, motorId, pMotor);

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
  for (uint index = 0; index < m_usedMotorSlots; ++index)
  {
      uint distance = (uint)abs(m_stepData[index]->StepDefinition);
      uint acceleration = m_stepData[index]->Motor->Acceleration();
      uint speedMultiplier = m_stepData[index]->Motor->InitialSpeedMultiplier();
      uint ticks = m_stepData[index]->Motor->MinTicksPerStep();

      // Store the maximum number of ticks that are required
      maxTicks = max(distance * ticks, maxTicks);
      maxSpeedMultiplier = max(speedMultiplier, maxSpeedMultiplier);
      
      m_stepData[index]->AccelerationStepLimit = min((speedMultiplier - m_stepData[index]->Motor->MinSpeedMultiplier()) / acceleration, (distance * ticks) / 2);
  }

  // maxTicks is the largest number of ticks to get to the end of the movement
  // everything else needs to be scaled relative to maxTicks.
  for (uint index = 0; index < m_usedMotorSlots; ++index)
  {
    m_stepData[index]->TicksPerStep = maxTicks / abs(m_stepData[index]->StepDefinition);
    m_stepData[index]->StepCount = 0;
    m_stepData[index]->TickCount = 0;

    // Set the direction for this movement for this motor
    m_stepData[index]->Motor->SetDirection(m_stepData[index]->StepDefinition > 0);
    
    m_stepData[index]->Motor->SetCurrentSpeedMultiplier(m_stepData[index]->Motor->InitialSpeedMultiplier());
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
    for (uint index = 0; index < m_usedMotorSlots; ++index)
    {
      MotorStepData *pStepData = m_stepData[index];

      int tickCount = ++pStepData->TickCount;

      if (pStepData->StepCount >= abs(pStepData->StepDefinition))
      {
        // This motor is complete
      }
      else
      {
        // At least one motor is still going, so indicate the movement is still
        // in progress
        result = S_OK;
        
        if (tickCount >= pStepData->TicksPerStep)
        {
          ++pStepData->StepCount;
          pStepData->TickCount -= pStepData->TicksPerStep;
          
          pStepData->Motor->Tick(true);

          // Should we start slowing down?
          if (pStepData->StepCount >= (abs(pStepData->StepDefinition) - pStepData->AccelerationStepLimit))  
          {
            maxSpeedMultiplier = Decelerate(pStepData, maxSpeedMultiplier);
          }
          else 
          {
            maxSpeedMultiplier = Accelerate(pStepData, maxSpeedMultiplier);
          }
        }
        else
        {
          pStepData->Motor->Tick(false);
          pStepData->TickCount = tickCount;
          
          maxSpeedMultiplier = max(pStepData->Motor->CurrentSpeedMultiplier(), maxSpeedMultiplier);
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

uint CMovement::Accelerate(MotorStepData *pStepData, uint maxSpeedMultiplier)
{
  uint minSpeedMultiplier = pStepData->Motor->MinSpeedMultiplier();
  uint speedMultiplier = pStepData->Motor->CurrentSpeedMultiplier();
  
  // Are we still accelerating?
  if (speedMultiplier > minSpeedMultiplier)
  {
    // Accelerating
    speedMultiplier = max(minSpeedMultiplier, speedMultiplier - pStepData->Motor->Acceleration());
    
    pStepData->Motor->SetCurrentSpeedMultiplier(speedMultiplier);
    
    maxSpeedMultiplier = max(speedMultiplier, maxSpeedMultiplier);
  }
  else
  {
    maxSpeedMultiplier = max(pStepData->Motor->CurrentSpeedMultiplier(), maxSpeedMultiplier);
  }

  return maxSpeedMultiplier;
}

uint CMovement::Decelerate(MotorStepData *pStepData, uint maxSpeedMultiplier)
{
  // Time to start slowing down 
  uint initialSpeedMultiplier = pStepData->Motor->InitialSpeedMultiplier();
  uint speedMultiplier = pStepData->Motor->CurrentSpeedMultiplier();
  if (speedMultiplier < initialSpeedMultiplier)
  {
    speedMultiplier = min(initialSpeedMultiplier, speedMultiplier + pStepData->Motor->Acceleration());
    
    pStepData->Motor->SetCurrentSpeedMultiplier(speedMultiplier);
    
    maxSpeedMultiplier = max(speedMultiplier, maxSpeedMultiplier);
  }
  else
  {
    maxSpeedMultiplier = max(initialSpeedMultiplier, maxSpeedMultiplier);

    pStepData->Motor->SetCurrentSpeedMultiplier(maxSpeedMultiplier);
  }

  return maxSpeedMultiplier;
}