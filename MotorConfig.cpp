#include "errors.h"
#include "Motor.h"
#include "MotorConfig.h"

CMotorConfig::CMotorConfig() :
  m_allocatedMotors(3),
  m_usedMotors(0)
{
  m_motors = (MotorEntry*)malloc(m_allocatedMotors * sizeof(MotorEntry));
}

int CMotorConfig::AddMotor(MotorId motorId, CMotor* pMotor)
{
  for (uint index = 0; index < m_usedMotors; ++index)
  {
    if (m_motors[index].Id == motorId)
    {
      return E_INVALID_MOTOR_ID;
    }
  }

  if (m_usedMotors >= m_allocatedMotors)
  {
    uint allocatedMotors = m_allocatedMotors + 3;
    MotorEntry *pMotors = (MotorEntry*)realloc(m_motors, allocatedMotors);
    if (pMotors != NULL)
    {
      m_allocatedMotors = allocatedMotors;
      m_motors = pMotors;
    }
    else
    {
      return E_OUTOFMEMORY;
    }
  }

  m_motors[m_usedMotors].Id = motorId;
  m_motors[m_usedMotors].pMotor = pMotor;

  ++m_usedMotors;
  
  return S_OK;
}

int CMotorConfig::GetMotor(MotorId motorId, CMotor **ppMotor)
{
  for (uint index = 0; index < m_usedMotors; ++index)
  {
    if (m_motors[index].Id == motorId)
    {
      *ppMotor = m_motors[index].pMotor;
      return S_OK;
    }
  }
  
  return E_INVALID_MOTOR_ID;
}