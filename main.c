/*----------------------------------------------------------------------------
	
	Designers Guide to the Cortex-M Family
	CMSIS RTOS Message Queue Example

*----------------------------------------------------------------------------*/
#include "global.h"
#include "pwm_wave.h"
#include "sawtooth_wave.h"
#include "triangle_wave.h"
#include "uart_handler.h"

osThreadDef(uart_handler_thread, osPriorityAboveNormal, 1, 0);
osThreadDef(pwm_wave_thread, osPriorityNormal, 1, 0);
osThreadDef(sawtooth_wave_thread, osPriorityNormal, 1, 0);
osThreadDef(triangle_wave_thread, osPriorityNormal, 1, 0);

osThreadId T_uart_thread;
osThreadId T_pwm_thread;
osThreadId T_sawtooth_thread;
osThreadId T_triangle_thread;

static GPIO_InitTypeDef _WAVEFORM_PORT_Conf = {
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

	GPIO_Init(WAVEFORM_PORT, &_WAVEFORM_PORT_Conf);
	uart_handler_init();
	pwm_wave_init();
	sawtooth_wave_init();
	triangle_wave_init();

	T_uart_thread = osThreadCreate(osThread(uart_handler_thread), NULL);
	//T_pwm_thread = osThreadCreate(osThread(pwm_wave_thread), NULL);
	T_sawtooth_thread = osThreadCreate(osThread(sawtooth_wave_thread), NULL);
	//T_triangle_thread = osThreadCreate(osThread(triangle_wave_thread), NULL);

	osKernelStart();                         						// start thread execution
}
