#include "motor.h"


CMotor::CMotor(
         uint maxStepsPerSecond, 
         uint maxAcceleration, 
         uint ticksPerPulse, 
         uint dutyTicks,
         Port directionPort, 
         uint directionBit, 
         Port stepPort, 
         uint stepBit, 
         Port mirrorPort, 
         uint mirrorBit) : 
            m_maxStepsPerSecond(maxStepsPerSecond),
            m_maxAcceleration(maxAcceleration),
            m_ticksPerPulse(ticksPerPulse),
            m_dutyTicks(dutyTicks),
            m_directionPort(directionPort),
            m_directionBit(directionBit),
            m_stepPort(stepPort),
            m_stepBit(stepBit),
            m_mirrorPort(mirrorPort),
            m_mirrorBit(mirrorBit),
            m_pulseTick(0)
{
}

void CMotor::SetDirection(bool positive)
{
  if (positive)
  {
    *m_directionPort = *m_directionPort | (1 << m_directionBit);
  }
  else
  {
    *m_directionPort = *m_directionPort & ~(1 << m_directionBit);
  }
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
    *m_stepPort = *m_stepPort | (1 << m_stepBit);
    if (m_mirrorPort)
    {
      *m_mirrorPort = *m_mirrorPort | (1 << m_mirrorBit);
    }
  }
  else
  {
    if (m_pulseTick >= m_dutyTicks)
    {
      *m_stepPort = *m_stepPort & ~(1 << m_stepBit);
      
      if (m_mirrorPort)
      {
        *m_mirrorPort = *m_mirrorPort & ~(1 << m_mirrorBit);
      }
    }
  }
  
  return S_OK;
}
