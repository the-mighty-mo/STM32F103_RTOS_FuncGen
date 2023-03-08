/*----------------------------------------------------------------------------
	
	Designers Guide to the Cortex-M Family
	CMSIS RTOS Message Queue Example

*----------------------------------------------------------------------------*/
#include "STM32F10x.h"
#include "cmsis_os.h"
#include "pwm_wave.h"
#include "uart_handler.h"

typedef enum _waveform_cfg_param_t {
	PARAM_PERIOD_MS,
	PARAM_AMPLITUDE,
	PARAM_DUTYCYCLE,
} waveform_cfg_param_t;

typedef struct _waveform_cfg_t {
	waveform_cfg_param_t type;
	uint32_t value;
} waveform_cfg_t;

osMailQDef(pwm_cfg_q, 0x8, waveform_cfg_t);
osMailQId Q_pwm_cfg_id;

osThreadDef(uart_thread, osPriorityAboveNormal, 1, 0);
osThreadDef(pwm_thread, osPriorityNormal, 1, 0);

osThreadId T_uart_thread;
osThreadId T_pwm_thread;

static GPIO_InitTypeDef _PORTA_Conf = {
	.GPIO_Pin = GPIO_Pin_All,
	.GPIO_Speed = GPIO_Speed_2MHz,
	.GPIO_Mode = GPIO_Mode_Out_PP
};

/*----------------------------------------------------------------------------
  Main: Initialize and start RTX Kernel
 *---------------------------------------------------------------------------*/

int main (void) 
{
	osKernelInitialize();                    						// initialize CMSIS-RTOS

	GPIO_Init(GPIOA, &_PORTA_Conf);
	uart_handler_init();

	Q_pwm_cfg_id = osMailCreate(osMailQ(pwm_cfg_q), NULL);

	T_uart_thread = osThreadCreate(osThread(uart_thread), NULL);
	T_pwm_thread = osThreadCreate(osThread(pwm_thread), NULL);

	osKernelStart();                         						// start thread execution
}
