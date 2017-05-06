#include "jogmovement.h"
#include "flags.h"
#include "errors.h"

CJogMovement::CJogMovement(CMotorConfig& motorConfiguration) :
  CMovement(motorConfiguration),
  m_oldMoveFlags(JogMoveNone),
  m_currentMoveFlags(JogMoveNone)
{
  // Add a psuedo linear move for each motor
  for (uint index = 0; index < m_motorConfig.Length(); ++index)
  {
    AddLinearMove(m_motorConfig[index]->Id, 0);
  }
}

int CJogMovement::Tick(uint* pPeriodMultipler)
{
  // Start out assuming the movement is not complete.
  int result = S_OK;
  
  if (!m_stopped)
  {
    uint maxSpeedMultiplier = 0;
    for (uint index = 0; index < m_usedMotorSlots; ++index)
    {
      MotorStepData *pStepData = m_stepData[index];
      
      JogMoveFlags axisFlags = JogMoveNone;
      JogMoveFlags changeFlags = JogMoveNone;
      bool positive = false;
      
      JogMoveFlags mask = JogMoveNone;
      JogMoveFlags positiveValue = JogMoveNone;
      switch (pStepData->Id)
      {
      case X_Axis:
        mask = XMask;
        positiveValue = PX;
        break;
        
      case Y_Axis:
        mask = YMask;
        positiveValue = PY;
        break;

      case Z_Axis:
        mask = ZMask;
        positiveValue = PZ;
        break;
      }

      changeFlags = (m_oldMoveFlags & mask) ^ (m_currentMoveFlags & mask);
      axisFlags = (m_oldMoveFlags & mask) | (m_currentMoveFlags & mask);
      positive = (axisFlags == positiveValue);

      int tickCount = ++pStepData->TickCount;
      bool updateOldMoveFlags = false;
      
      if (axisFlags == 0)
      {
        if (pStepData->Motor->InitialSpeedMultiplier() > pStepData->Motor->CurrentSpeedMultiplier())
        {
          // This motor is complete
          uint currentSpeedMultiplier = Decelerate(pStepData, 0);
          
          pStepData->TicksPerStep = (currentSpeedMultiplier * pStepData->Motor->MinTicksPerStep()) / pStepData->Motor->MinSpeedMultiplier();
        } 
        else 
        {
          updateOldMoveFlags = true;
        }
      }
      else
      {
        if (tickCount >= pStepData->TicksPerStep)
        {
          ++pStepData->StepCount;
          pStepData->TickCount -= pStepData->TicksPerStep;
          
          pStepData->Motor->Tick(true);

          // Should we start slowing down?
          if (changeFlags)  
          {
            uint currentSpeedMultiplier = Decelerate(pStepData, 0);
            
            pStepData->TicksPerStep = (currentSpeedMultiplier * pStepData->Motor->MinTicksPerStep()) / pStepData->Motor->MinSpeedMultiplier();
            
            if (currentSpeedMultiplier == pStepData->Motor->InitialSpeedMultiplier())
            {
              updateOldMoveFlags = true;
              
              // We've reached our slowest speed, so either stop or reverse direction
              if (axisFlags)
              {
                // We've got to reverse direction
                pStepData->Motor->SetDirection(positive);
              }
            }
          }
          else 
          {
            uint currentSpeedMultiplier = Accelerate(pStepData, 0);
            
            pStepData->TicksPerStep = (currentSpeedMultiplier * pStepData->Motor->MinTicksPerStep()) / pStepData->Motor->MinSpeedMultiplier();
          }
        }
        else
        {
          pStepData->Motor->Tick(false);
          pStepData->TickCount = tickCount;
        }
        
        maxSpeedMultiplier = max(pStepData->Motor->MinSpeedMultiplier(), maxSpeedMultiplier);
      }
            
      if (updateOldMoveFlags)
      {
         m_oldMoveFlags = (m_oldMoveFlags & ~mask) | (m_currentMoveFlags & mask);
      }
    }
    
    *pPeriodMultipler = maxSpeedMultiplier;
      
    if (maxSpeedMultiplier == 0)
    {
      // There is no movement asked for, or made.
      result = S_FALSE;
    }
  }
  
  return result;
}

void CJogMovement::SetAxes(JogMoveFlags moveFlags)
{
  // If we're changing direction or stopping a movement, then we'll need to decelerate first.
  m_currentMoveFlags = moveFlags;
}