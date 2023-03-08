/*----------------------------------------------------------------------------
	
	Designers Guide to the Cortex-M Family
	CMSIS RTOS Message Queue Example

*----------------------------------------------------------------------------*/
#include "STM32F10x.h"
#include "cmsis_os.h"
#include "uart.h"

#define GPIO_MAX_V (100)
#define SCALE_AMPLITUDE(mV) (((uint32_t)mV * 0xFFFF) / GPIO_MAX_V)
#define SCALE_DUTYCYCLE(dc) (((uint16_t)dc * 1024) / 100)

typedef enum _waveform_cfg_param_t {
	PARAM_PERIOD_MS,
	PARAM_AMPLITUDE,
	PARAM_DUTYCYCLE,
} waveform_cfg_param_t;

typedef struct _waveform_cfg_t {
	waveform_cfg_param_t type;
	uint32_t value;
} waveform_cfg_t;

osMessageQDef(uart_q, 0x10, uint8_t);
osMessageQId Q_uart_id;

osMailQDef(pwm_cfg_q, 0x8, waveform_cfg_t);
osMailQId Q_pwm_cfg_id;

void pwm_on(void const *arg);
void pwm_off(void const *arg);
void pwm_delay(void const *arg);

osTimerDef(pwm_on_timer, pwm_on);
osTimerDef(pwm_off_timer, pwm_off);
osTimerDef(pwm_delay_timer, pwm_delay);
osTimerId TMR_pwm_on_timer;
osTimerId TMR_pwm_off_timer;
osTimerId TMR_pwm_delay_timer;

void uart_thread(void const *arg);
void pwm_thread(void const *arg);

osThreadDef(uart_thread, osPriorityAboveNormal, 1, 0);
osThreadDef(pwm_thread, osPriorityNormal, 1, 0);

osThreadId T_uart_thread;
osThreadId T_pwm_thread;

void uart_thread(void const *arg)
{
	osEvent result;
	uint8_t input;

	while (1) {
		result = osMessageGet(Q_uart_id, osWaitForever);
		input = result.value.v;

		SendChar(input);
	}
}

void pwm_on(void const *arg)
{
	uint32_t amplitude = (uint32_t)arg;
	GPIO_Write(GPIOA, amplitude);
}

void pwm_off(void const *arg)
{
	GPIO_Write(GPIOA, 0x0000);
}

void pwm_delay(void const *arg)
{
	uint32_t periodMs = (uint32_t)arg;
	osTimerStart(TMR_pwm_off_timer, periodMs);
}

void pwm_thread(void const *arg)
{
	uint32_t amplitude;
	uint32_t onTimeMs;
	uint32_t offTimeMs;
	uint32_t periodMs = 100;

	{
		uint16_t mV = 75;
		uint8_t dutyCycle = 75;

		if (mV > GPIO_MAX_V) mV = GPIO_MAX_V;
		if (dutyCycle > 100) dutyCycle = 100;

		uint16_t dutyCycle_q0d10 = SCALE_DUTYCYCLE(dutyCycle);

		amplitude = SCALE_AMPLITUDE(mV);
		onTimeMs = ((uint64_t)periodMs * dutyCycle_q0d10) >> 10;
		offTimeMs = periodMs - onTimeMs;
	}

	TMR_pwm_on_timer = osTimerCreate(osTimer(pwm_on_timer), osTimerPeriodic, (void *)amplitude);
	TMR_pwm_off_timer = osTimerCreate(osTimer(pwm_off_timer), osTimerPeriodic, NULL);
	TMR_pwm_delay_timer = osTimerCreate(osTimer(pwm_delay_timer), osTimerOnce, (void *)periodMs);

	osTimerStart(TMR_pwm_on_timer, periodMs);
	osTimerStart(TMR_pwm_delay_timer, onTimeMs);
}

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

	USART1_Init();
	GPIO_Init(GPIOA, &_PORTA_Conf);
	//Configure USART interrupt so we can read user inputs using interrupt
	//Configure and enable USART1 interrupt
	NVIC->ICPR[USART1_IRQn/32] = 1UL << (USART1_IRQn%32);	//clear any previous pending interrupt flag
	NVIC->IP[USART1_IRQn] = 0x80;		//set priority to 0x80
	NVIC->ISER[USART1_IRQn/32] = 1UL << (USART1_IRQn%32);	//set interrupt enable bit
	USART1->CR1 |= USART_CR1_RXNEIE; //enable USART receiver not empty interrupt

	Q_uart_id = osMessageCreate(osMessageQ(uart_q), NULL);
	Q_pwm_cfg_id = osMailCreate(osMailQ(pwm_cfg_q), NULL);

	T_uart_thread = osThreadCreate(osThread(uart_thread), NULL);
	T_pwm_thread = osThreadCreate(osThread(pwm_thread), NULL);

	osKernelStart();                         						// start thread execution
}

/*-----------------------------------------------------------------------------
	USART1 IRQ Handler
		The hardware automatically clears the interrupt flag, once the ISR is entered
 *---------------------------------------------------------------------------*/
void USART1_IRQHandler(void)
{
	uint8_t intKey = (int8_t)(USART1->DR & 0x1FF);
	osMessagePut(Q_uart_id, intKey, 0);
}
