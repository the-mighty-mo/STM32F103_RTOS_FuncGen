/*
 * CE 426 - Real-Time Embedded Systems
 * Instructor: Dr. Tewolde
 * Author: Benjamin Hall
 * Final Project: Function Generator
 *
 * Description: This program runs a function generator.
 * The supported waveforms are:
 * - PWM
 * - Triangle
 * - Sawtooth
 * - Sine
 * Waveforms can be selected and configured using USART1.
 * The CPU is put into a low power mode while idle.
 */

#include "global.h"
#include "pwm_wave.h"
#include "sawtooth_wave.h"
#include "sine_wave.h"
#include "triangle_wave.h"
#include "uart_handler.h"

// user IO thread gets higher priority, the other
// threads just manage configuration params
osThreadDef(uart_handler_thread, osPriorityAboveNormal, 1, 0);
osThreadDef(pwm_wave_thread, osPriorityNormal, 1, 0);
osThreadDef(sawtooth_wave_thread, osPriorityNormal, 1, 0);
osThreadDef(sine_wave_thread, osPriorityNormal, 1, 0);
osThreadDef(triangle_wave_thread, osPriorityNormal, 1, 0);

osThreadId T_uart_thread;
osThreadId T_pwm_thread;
osThreadId T_sawtooth_thread;
osThreadId T_sine_thread;
osThreadId T_triangle_thread;

// config for the waveform GPIO port
static GPIO_InitTypeDef _WAVEFORM_PORT_Conf = {
	// use all pins
	.GPIO_Pin = GPIO_Pin_All,
	// 2 MHz is sufficient, our waves can only go up to 1 kHz
	.GPIO_Speed = GPIO_Speed_2MHz,
	// we're outputing to the pins, pull-up/pull-down
	.GPIO_Mode = GPIO_Mode_Out_PP
};

/*----------------------------------------------------------------------------
  Main: Initialize and start RTX Kernel
 *---------------------------------------------------------------------------*/

int main (void) 
{
	osKernelInitialize();                    						// initialize CMSIS-RTOS

	// initialize the waveform port
	GPIO_Init(WAVEFORM_PORT, &_WAVEFORM_PORT_Conf);
	// initialize UART for user IO
	uart_handler_init();
	// initialize all the waveforms
	pwm_wave_init();
	sawtooth_wave_init();
	sine_wave_init();
	triangle_wave_init();

	// create threads for user IO and the waveforms
	T_uart_thread = osThreadCreate(osThread(uart_handler_thread), NULL);
	T_pwm_thread = osThreadCreate(osThread(pwm_wave_thread), NULL);
	T_sawtooth_thread = osThreadCreate(osThread(sawtooth_wave_thread), NULL);
	T_sine_thread = osThreadCreate(osThread(sine_wave_thread), NULL);
	T_triangle_thread = osThreadCreate(osThread(triangle_wave_thread), NULL);

	osKernelStart();                         						// start thread execution
}
