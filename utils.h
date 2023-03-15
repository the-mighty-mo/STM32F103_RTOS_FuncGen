#pragma once

#include <stdint.h>

#define SCALE_AMPLITUDE(perc) (((uint32_t)perc * 0xFFFF) / 100)
#define SCALE_DUTYCYCLE(dc) (((uint32_t)dc * 1024) / 100)
