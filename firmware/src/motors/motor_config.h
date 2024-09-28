#pragma once

// Tuning parameters for the MAD2804 motor (orange stator).

#define FOC_PID_P 1
#define FOC_PID_I 0
#define FOC_PID_D 0.148
#define FOC_PID_OUTPUT_RAMP 1000
#define FOC_PID_LIMIT 6

#define FOC_VOLTAGE_LIMIT 6
#define FOC_LPF 0.0075

// #define FOC_PID_P 4
// #define FOC_PID_I 0
// #define FOC_PID_D 0.04
// #define FOC_PID_OUTPUT_RAMP 10000
// #define FOC_PID_LIMIT 3

// #define FOC_VOLTAGE_LIMIT 3
