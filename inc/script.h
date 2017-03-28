// Script.h

#ifndef __SCRIPT_H__
#define __SCRIPT_H__

class CScript
{
private:
  typedef enum
  {
    Absolute = 0x1,  // vs Relative
    Inches = 0x2, // vs mm
  } Modes ;

  typedef enum

  MovementMode m_mode;

public:
  CScript();

  int Load(uchar* rawScript);
}

#endif // __SCRIPT_H__