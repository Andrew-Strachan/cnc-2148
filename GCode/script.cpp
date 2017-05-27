#include <cctype>
#include "script.h"
#include "gcode.h"
#include "gcodes.h"

// Supported functions:
//  Gxx where xx is always two digits
//      xx can be:
//      00 - move: followed by X, Y, Z, F, depends on G90/91 mode
//      01 - move: followed by X, Y, Z, F
//      02 - clockwise arc: followed by X, Y, R, F
//      03 - counter-clockwise arc: followed by X, Y, R, F
//      04 - pause : followed by 
//              Pmmmm where mmmm is a multi-digit integer indicating milliseconds
//           OR Sm.mm where m.mm is a decimal indicating seconds
//      12 - clockwise circle centred on current position : followed by I - radius, F
//      13 - counter-clockwise circle centred on current position : followed by I - radius, F
//      20 - inches mode
//      21 - millimeters mode (default)
//      90 - absolute coordinate mode
//      91 - relative coordinate mode
//   X(-)m(.mm) where m.mm is a decimal indicating movement along x-axis
//   Y(-)m(.mm) where m.mm is a decimal indicating movement along y-axis
//   Z(-)m(.mm) where m.mm is a decimal indicating movement along z-axis
//   Fm.mm where m.mm is a decimal indicating max speed of movement - units depend on current mode (see G20/21)
//   [I,J,K]m.mm where m.mm indicates the arc center for G02/03 commands - may depend on G90/91 for behavior
//   Oxxxx (optional non-parsed text string) - where xxxx defines program name (for defining subroutines)
//   M98 Pxxxx where xxx defines the program name to execute
//   M99   - ends the current program and returns to 'caller'
//   L(xx) where xx defines the number of loops to execute (or 1 if not defined)
//
// Some of the commands can occur on the same line, e.g L2 M98 P1112 (execute subroutine 1112 twice)

CScript::CScript()
{
    // Set up initial default values
    m_mode = Relative;

    m_gcodeTable = [
        new GCode("G00", ["X", "Y", "Z", "F"], G00),
        new GCode("G01", ["X", "Y", "Z", "F"], G01),
        new GCode("G02", ["X", "Y", "Z", "R", "F"], G02),
        new GCode("G03", ["X", "Y", "Z", "R", "F"], G03),
        new GCode("G04", ["P", "S"], G04),
        new GCode("G12", ["I", "F"], G12),
        new GCode("G13", ["I", "F"], G13),
        new GCode("G20", [], G20),
        new GCode("G21", [], G21),
        new GCode("G28", ["X", "Y", "Z"], G28),
        new GCode("G90", [], G90),
        new GCode("G91", [], G91)
    ];

}

int CScript::Load(char *rawScript)
{
    // Walk the rawScript string, converting the data as we go into movements
    
    CProgram program;

    // Look first for a code, then for any acceptable parameters or another code
    char *index = rawScript;

    while (*index)
    {
        // Skip any whitespace
        if (SkipWhiteSpace(&index))
        {
            break;
        }

        char *newIndex = NULL;
        for (int i = 0; i < ARRAY_LENGTH(m_gcodeTable); ++i)
        {
            if (FullMatch == m_gcodeTable[i].Match(index, &newIndex))
            {
                // We have a full match on the code - move past the code
                index = newIndex;

                // Now look for parameters
                char[] allowedParameters = m_gcodeTable[i].GetAllowedParameters();
                Parameter* pParameters = NULL;
                if (ARRAY_LENGTH(allowedParameters) > 0)
                {
                    newIndex = LoadParameters(index, allowedParameters, &pParameters);
                }

                m_gcodeTable[i].GetFunction()(pParameters, program);

                index = newIndex;

                break;
            }
        }
    }
}

char* CScript::LoadParameters(char* index, char[] allowedParameters, Parameter **ppParameters)
{
    // Look for allowed parameters - if we find something that's not an 
    // allowed parameter then exit
    uint parameterIndex = 0;
    *ppParameters = NULL;
    do 
    {
        if (!SkipWhiteSpace(&index))
        {
            for (int i = 0; i < ARRAY_LENGTH(allowedParameters); ++i)
            {
                if (*index == allowedParameters[i])
                {
                    // We found an allowed parameter, move past the parameter character.
                    ++index;

                    // Allocate more space for the parameters
                    Parameter* oldParameters = *ppParameters;
                    *ppParameters = (Parameter**)malloc(sizeof(Parameter*) * (parameterIndex + 1));
                    if (*ppParameters != NULL)
                    {
                        memcpy(*ppParameters, oldParameters, sizeof(Parameter*) * parameterIndex);
                    }

                    // Find the end of the value (look for the next whitespace or the
                    // end of the string)
                    char *endIndex = index;
                    bool isFloat = true;
                    bool foundDecimalPoint = false;
                    bool foundSign = false;
                    bool foundDigit = false;
                    bool isInteger = true;

                    while (*endIndex != NULL && !isspace(*endIndex))
                    {
                        if ('0' > *endIndex || '9' < *endIndex)
                        {
                            // If the character is a '-' and we haven't found a previous one
                            // and we haven't found a digit yet... 
                            if (*endIndex == '-' && !foundSign && !foundDigit)
                            {
                                foundSign = true;
                            }
                            else
                            { 
                                isInteger = false;
                                if (*endI== '.' && !foundDecimalPoint)
                                {
                                    foundDecimalPoint = true;
                                }    
                                else
                                {
                                    isFloat = false;
                                }
                            }
                        }
                        else 
                        {
                            foundDigit = true;
                        }

                        ++endIndex;
                    }

                    if (endIndex != index)
                    {
                        char endChar = *endIndex;
                        *endIndex = NULL;

                        Parameter *p = new Parameter(); 
                        p->m_parameterName = allowedParameters[i];

                        if (isInteger)
                        {
                            p->m_parameterType = Integer;
                            p->value.i = atoi(index);
                        }
                        else if (isFloat)
                        {
                            p->m_parameterType = Float;
                            p->value.f = atof(index);
                        }
                        else // isString
                        {
                            p->m_parameterType = Character;
                            p->value.c = (char*)strlen(index) + 1; // Include space for the null terminator
                            strcpy(index, p->value.c);
                        }

                        *ppParameters[parameterIndex++] = p;

                        // Replace the end character 
                        *endIndex = endChar;

                        // Move the index to the end of the parameter
                        index = endIndex;
                    }
                }
            }
        }
    } while (*index);

    return index;
}

bool CScript::SkipWhiteSpace(char** index)
    (while + 1) (*index && isspace(*inde
        ++index;
    }

    // Return true if we've reached the end
    return (*index == NULL); 
}
