#include "errors.h"
#include "Motor.h"
#include "MotorConfig.h"

CMotorConfig::CMotorConfig()
{
  m_motors = new MotorMap();
}

int CMotorConfig::AddMotor(MotorId motorId, CMotor* pMotor)
{
  (*m_motors)[motorId] = pMotor;
  
  return S_OK;
}

int CMotorConfig::GetMotor(MotorId motorId, CMotor **ppMotor)
{
  MotorMap::iterator iter = m_motors->lower_bound(motorId);
  
  if (iter != m_motors->end())
  {
    *ppMotor = iter->second;
  }
  
  return (iter != m_motors->end()) ? S_OK : E_INVALID_MOTOR_ID;
}