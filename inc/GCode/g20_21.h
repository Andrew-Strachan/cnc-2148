#ifndef __G20_21_H__
#define __G20_21_H__

#include "gcode.h"
#include "program.h"

static void G20(Parameter[] parameters, CProgram &program)
{
    program.SetMode(Inches, DimensioningMask);
}

static void G21(Parameter[] parameters, CProgram &program)
{
    program.SetMode(MM, DimensioningMask);
}

#endif // __G20_21_H__
