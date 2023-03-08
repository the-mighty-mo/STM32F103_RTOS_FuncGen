#include <stdint.h>

#define GPIO_MAX_V (100)
#define SCALE_AMPLITUDE(mV) (((uint32_t)mV * 0xFFFF) / GPIO_MAX_V)
#define SCALE_DUTYCYCLE(dc) (((uint16_t)dc * 1024) / 100)
