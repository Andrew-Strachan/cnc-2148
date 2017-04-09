#ifndef __MOTORCONFIG_H__
#define __MOTORCONFIG_H__

#include <map>
#include "motor.h"
#include "limits.h"

typedef enum tagMotorId {
  X_Axis = 1,
  Y_Axis = 2,
  Z_Axis = 3,
  Rotate_Axis = 4,
  FilamentFeed_1 = 5,
  FilamentFeed_2 = 6,
  FilamentFeed_3 = 7
} MotorId;


typedef std::map<MotorId, CMotor*> MotorMap;

class CMotorConfig
{
private:
  typedef struct _MotorEntry {
    MotorId Id;
    CMotor *pMotor;
  } MotorEntry;

  uint m_allocatedMotors;
  uint m_usedMotors;
  MotorEntry *m_motors;
  
public:
  CMotorConfig();
  
  int AddMotor(MotorId motorId, CMotor *pMotor);
  
  int GetMotor(MotorId motorId, CMotor **pMotor);
};

#endif // __MOTORCONFIG_H__
