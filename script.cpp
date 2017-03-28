#include "script.h"

// Supported functions:
//  Gxx where xx is always two digits
//      xx can be:
//      00
//      01
//      02
//      03
//      04 - followed by
//              Pmmmm where mmmm is a multi-digit integer indicating milliseconds
//           OR Sm.mm where m.mm is a decimal indicating seconds
//      12
//      13
//      20 - inches mode
//      21 - millimeters mode (default)
//      28 - followed by X, Y, Z
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
}

int CScript::Load(uchar *rawScript)
{
    // Walk the rawScript string, converting the data as we go into movements

}
