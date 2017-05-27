#ifndef __GCODE_H__
#define __GCODE_H__

#include "program.h"

typedef enum _ParameterType
{
    Integer,
    Float,
    Character
} ParameterType;

typedef struct _Parameter 
{
    char m_parameterName;
    ParameterType m_parameterType;
    union {
        float f;
        int i;
        char c;
    } m_value;
} Parameter;

void (CodeFunction*)(Parameter[] parameters, CProgram &program);

typedef enum _MatchType
{
    None,
    PartialMatch,
    FullMatch
} MatchType;

class CGCode 
{
private:
    char *m_code;
    char[] m_allowedParameters;
    CodeFunction* m_func;

public:
    CGCode(char* code, char [] allowedParameters, CodeFunction* func);

    MatchType Matches(char* code, char** newIndex);

    inline char[] GetAllowedParameters() { return m_allowedParameters; }
    inline CodeFunction* GetFunction() { return m_func; }
};

#endif // __GCODE_H__