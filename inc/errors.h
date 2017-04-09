#ifndef __ERRORS_H__
#define __ERRORS_H__

#define S_OK ((long)0)
#define S_FALSE ((long)1)

#define E_BASE ((long)0x80000000)
#define E_OUTOFMEMORY (E_BASE + 1)

#define E_MOTOR_BASE  (E_BASE + 0x0100)
#define E_OVERLAPPING_PULSES (E_MOTOR_BASE + 1)
#define E_INVALID_MOTOR_ID (E_MOTOR_BASE + 2)

#define E_MOVEMENT_BASE (E_BASE + 0x0200)
#define E_DUPLICATE_RESOURCE_MOVE (E_MOVEMENT_BASE + 1)

#define E_MOTOR_CONFIG_BASE ((long)0x80000300)


#endif // __ERRORS_H__