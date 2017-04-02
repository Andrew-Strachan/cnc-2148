#include "movement.h"
#include "ll_helper.h"

// Constructor
// Takes the motor configuration.
// Any steps added will be validated against the motor configuration.
CMovement::CMovement(CMotorConfig& motorConfiguration) :
  m_motorConfig(motorConfiguration),
  m_stopped(false),
  m_pStepData(NULL),
  m_stepDataCount(0),
	m_allocatedStepDataCount(0)
{
}

// AddLinearMove adds a specific move to this movement.
// It is an error to add more than one move for a specific motor to any
// movement.
// Returns 0 on success
//        -ve on failure
int CMovement::AddLinearMove(MotorId motorId, int steps)
{
	for (unsigned short index = 0; index < m_stepDataCount; ++index)
	{
		if (m_pStepData[index].MotorId == motorId)
		{
			return E_DUPLICATE_RESOURCE_MOVE;
		}
	}

	CMotor *pMotor;
	if (m_motorConfig.GetMotor(motorId, &pMotor) != S_OK)
	{
		return E_DUPLICATE_RESOURCE_MOVE;
	}

	if (m_allocatedStepDataCount <= m_stepDataCount)
	{
		m_pStepData = reinterpret_cast<CStepData*>(realloc(m_pStepData, m_allocatedStepDataCount += 3));
		memset(&m_pStepData[m_stepDataCount], 0, sizeof(m_pStepData[0]) * (m_allocatedStepDataCount - m_stepDataCount));
	}
  
	m_pStepData[m_stepDataCount].MotorId = motorId;
	m_pStepData[m_stepDataCount].StepDefinition = steps;
	m_pStepData[m_stepDataCount].pMotor = pMotor;

	m_stepDataCount++;
  return S_OK;
}

// Begin calculates all the internal state required for the movement.
// Returns 0 on success
//        -ve on failure
int CMovement::Begin(uint* pPeriodMultipler)
{
	/*
  We need to work out what the Period Multiplier should be initially - it will change
  to support acceleration of the head.  1 implies that the head is travelling as fast as possible.
  Given the starting speed (ss), and knowing the top speed (ts) of a motor, the Period Multiplier
  will be ts/ss

  If we assume the acceleration is in terms of change in speed per period for a motor then we should be
  able to recalculate the acceptable multiplier for a given motor every time we send the motor a Tick(true).

  We should then return the largest multiplier for all the motors involved in the movement.

  We only work in multiples of the base speed.  We make the base speed way too fast and run at something like 10 times slower than the base speed.  Using a multiplier of 16000 or something to get down to a slow speed.
  e.g. with a timer frequency of 5.76kHz, 72 steps per rotation and 8x micro-stepping, a pitch of 5mm/rotation we can go from a speed of 10mm/s with a multiplier of 1 and 0.001mm/s with a multiplier of 10000.  By altering the timer frequency and the micro-steps we could easily change the top speed.  i.e. Changing to not using micro-stepping, we would immediately change the
  top speed to 80mm/s.
*/
  // Work out what the slowest motor will be, taking into account both the number
  // of steps and the acceleration/deceleration of the motor.
  uint maxTicks = 0;
  for (unsigned short index = 0; index < m_stepDataCount; ++index)
  //for (MotorStepMap::iterator iter = m_stepDefinition.begin(); iter != m_stepDefinition.end(); ++iter)
  {
    CMotor *pMotor = m_pStepData[index].pMotor;
      uint distance = (uint)abs(m_pStepData[index].StepDefinition);
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

  // maxTicks is the largest number of ticks to get to the end of the movement
  // everything else needs to be scaled relative to maxTicks.
  for (unsigned short index = 0; index < m_stepDataCount; ++index)
	 //for (MotorStepMap::iterator iter = m_stepDefinition.begin(); iter != m_stepDefinition.end(); ++iter)
  {
	  m_pStepData[index].TicksPerStep = maxTicks / m_pStepData[index].StepDefinition;
    //m_ticksPerStep[iter->first] = maxTicks / iter->second;

    // Set the direction for this movement for this motor
    CMotor *pMotor = m_pStepData[index].pMotor;
    //if (m_motorConfig.GetMotor(iter->first, &pMotor) == S_OK)
    {
      pMotor->SetDirection(m_pStepData[index].StepDefinition > 0);
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
	  for (unsigned short index = 0; index < m_stepDataCount; ++index)
		  //for (MotorStepMap::iterator iter = m_ticksPerStep.begin(); iter != m_ticksPerStep.end(); ++iter)
    {
		  int tickCount = ++m_pStepData[index].TickCount;
      //int tickCount = ++m_tickCount[iter->first];

      CMotor *pMotor = m_pStepData[index].pMotor;
      //if (m_motorConfig.GetMotor(iter->first, &pMotor) == S_OK)
      {
		  if (tickCount > m_pStepData[index].TicksPerStep)
        //if (tickCount >= iter->second)
        {
          pMotor->Tick(true);
		  m_pStepData[index].TickCount = 0;
          //m_tickCount[iter->first] = 0;
        }
        else
        {
          pMotor->Tick(false);
		  m_pStepData[index].TickCount = tickCount;
		  //m_tickCount[iter->first] = tickCount;
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
