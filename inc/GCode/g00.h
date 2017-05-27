#ifndef __G00_H__
#define __G00_H__

#include "gcode.h"
#include "program.h"

static void G00(Parameter[] parameters, CProgram &program)
{
    // Validate the parameters

    // Create a new movement
    CMovement movement = new CMovement(motorConfig);
    for (int i = 0; i < ARRAY_LENGTH(parameters); ++i)
    {
        MotorId moveAxis = 0;
        switch(parameters[i].m_parameterName) 
        {
            case "X":
                moveAxis = X_Axis;
                break;

            case "Y":      
                moveAxis = Y_Axis;
                break;

            case "Z":      
                moveAxis = Z_Axis;
                break;
        }

        if (moveAxis != 0)
        {
            float value = parameters[i].m_value.f;
            if (parameters[i].m_parameterType == Integer)
            {
                value = (float)parameters[i].m_value.i;
            }

            movement->AddLinearMove(moveAxis, value);
        }
    }

    program.AddMovement(&movement);
}

#endif // __G00_H__
