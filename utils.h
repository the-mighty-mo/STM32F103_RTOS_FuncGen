#pragma once

#include <stdint.h>

// Max amplitude is uint16_t MAX = 0xFFFF, input is %
#define SCALE_AMPLITUDE(perc)	(((uint32_t)perc * 0xFFFF) / 100)
// Max duty cycle is 1024, input is %
#define SCALE_DUTYCYCLE(dc)		(((uint32_t)dc * 1024) / 100)

// User amplitude is %, max is uint16_t MAX = 0xFFFF, round by adding half before divide = 0x7FFF
#define AMPLITUDE_TO_USER(amp)	((((uint32_t)amp * 100) + 0x7FFF) / 0xFFFF)
// User duty cycle is %, max is 1024, round by adding half before divide = 512
#define DUTYCYCLE_TO_USER(dc)	((((uint32_t)dc * 100) + 512) / 1024)
