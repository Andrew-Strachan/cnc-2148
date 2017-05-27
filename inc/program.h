#ifndef __PROGRAM_HPP__
#define __PROGRAM_HPP__

typedef enum
{
  Absolute = 0x1,  
  Relative = 0x2,
  PositioningMask = 0x3,
  Inches = 0x4, 
  MM = 0x8,
  DimensioningMask = 0xB
} Modes ;

public class CProgram
{
public:
  CProgram();

  void AddMovement(CMovement *pMovement);

  void SetMode(Modes mode, Modes mask);
};

#endif // __PROGRAM_HPP__
