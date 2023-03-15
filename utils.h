#pragma once

#include <stdint.h>

#define SCALE_AMPLITUDE(perc)	(((uint32_t)perc * 0xFFFF) / 100)
#define SCALE_DUTYCYCLE(dc)		(((uint32_t)dc * 1024) / 100)

#define AMPLITUDE_TO_USER(amp)	((((uint32_t)amp * 100) + 0x7FFF) / 0xFFFF)
#define DUTYCYCLE_TO_USER(dc)	((((uint32_t)dc * 100) + 512) / 1024)
