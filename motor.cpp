#include "motor.hpp"

CMotor::CMotor(
         uint maxStepsPerSecond, 
         uint maxAcceleration, 
         uint ticksPerPulse, 
         uint dutyTicks,
         Port directionPort, 
         uint directionBit, 
         Port stepPort, 
         uint stepBit) : 
            m_maxStepsPerSecond(maxStepsPerSecond),
            m_maxAcceleration(maxAcceleration),
            m_ticksPerPulse(ticksPerPulse),
            m_dutyTicks(dutyTicks),
            m_directionPort(directionPort),
            m_directionBit(directionBit),
            m_stepPort(stepPort),
            m_stepBit(stepBit),
            m_pulseTick(0)
{
}

void CMotor::SetDirection(bool positive)
{
  m_directionPort[m_directionBit] = positive;
}

int CMotor::Tick(bool step)
{
  ++m_pulseTick;
  
  if (step)
  {
    if (m_pulseTick < m_ticksPerPulse)
    {
      return E_OVERLAPPING_PULSES;
    }
    
    m_pulseTick = 0;
    m_stepPort[m_stepBit] = 1;
  }
  else
  {
    if (m_pulseTick >= m_dutyTicks)
    {
      m_stepPort[m_stepBit] = 0;
    }
  }
  
  return S_OK;
}
