#ifndef __G00_H__
#define __G00_H__

#include "gcode.h"
#include "program.h"

static void G90(Parameter[] parameters, CProgram &program)
{
    program.SetMode(Absolute, PositioningMask);
}

static void G91(Parameter[] parameters, CProgram &program)
{
    program.SetMode(Relative, PositioningMask);
}

#endif // __G00_H__
