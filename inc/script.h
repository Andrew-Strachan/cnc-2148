#ifndef __SCRIPT_H__
#define __SCRIPT_H__

#include "program.h"

class CScript
{
private:
  Modes m_mode;

  GCode[] m_gcodeTable;

  bool SkipWhiteSpace(char** index);;

  char* LoadParameters(char[] allowedParameters, char* index);

public:
  CScript();

  int Load(uchar* rawScript, Program *program);

};

#endif // __SCRIPT_H__
