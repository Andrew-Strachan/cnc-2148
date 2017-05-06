#ifndef __JOGMOVEMENT_H__
#define __JOGMOVEMENT_H__

#include "movement.h"

typedef enum 
{
    JogMoveNone = 0,
    PX = 0x01,
    NX = 0x02, 
    PY = 0x04,
    NY = 0x08,
    PZ = 0x10,
    NZ = 0x20,
    
    XMask = 0x03,
    YMask = 0x0c,
    ZMask = 0x30
} JogMoveFlags;

class CJogMovement : public CMovement
{
private:
  JogMoveFlags m_oldMoveFlags;
  JogMoveFlags m_currentMoveFlags;
  
public:
  CJogMovement(CMotorConfig& motorConfiguration);
  
  int Tick(uint* pPeriodMultipler);
  
  void SetAxes(JogMoveFlags);
};

#endif // __JOGMOVEMENT_H__