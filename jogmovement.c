#include "jogmovement.h"
#include "errors.h"

CJogMovement::CJogMovement(CMotorConfig& motorConfiguration) :
  CMovement(motorConfiguration)
{
  
}

int CJogMovement::Tick(uint* pPeriodMultipler)
{
  if (!m_stopped)
  {
  }
  
  return S_FALSE;
}

void CJogMovement::SetAxes(JogMoveFlags)
{
  // If we're changing direction or stopping a movement, then we'll need to decelerate first.
  
}