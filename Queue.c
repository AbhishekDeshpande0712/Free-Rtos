/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

//Queue project

#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Queue.h"
#include "timers.h"
#include "string.h"
#include<stdio.h>

TaskHandle_t xTaskHandle1 = NULL;
TaskHandle_t xTaskHandle2 = NULL;
TaskHandle_t xTaskHandle3 = NULL;
TaskHandle_t xTaskHandle4 = NULL;

QueueHandle_t Q1 = NULL;
QueueHandle_t  Q2 = NULL;

uint8_t input = 0;

uint8_t count = 0x02;
typedef struct app
{
	uint8_t cmd;
	uint8_t args[10];
}app_t;

char menu[] = {
"1. LED_ON  2. LED_OFF 3. TOGGLE_ON 4. TOGGLE_OFF"
};
void rtos_delay(uint32_t delay_ms);
void vTask1_menu_display(void *params);
void vTask2_cmd_handling(uint8_t *params);
void vTask3_cmd_processing(void *params);
void vTask4_uart_write(void *params);

static void prvSetupHardware(void);

#ifdef  USE_SEMIHOSTING
// semi hosting
extern void initialise_monitor_handles();
#endif

int main(void)
{
#ifdef  USE_SEMIHOSTING
	// semihosting
	  initialise_monitor_handles();
#endif

	// Reset the RCC cnfiguration to default reset state
		RCC_DeInit();

		// update the systemcoreclock variable
		SystemCoreClockUpdate();

		// setting up GPIO's
		prvSetupHardware();

		printf("Welcome to queue controlled led applications\n");

	//	SEGGER_SYSVIEW_Conf();
	//	SEGGER_SYSVIEW_Start();
		Q1 = xQueueCreate(10,sizeof(app_t *));

		Q2 = xQueueCreate(10, sizeof(char *));

		if((Q1!=NULL) && (Q2!=NULL))
		{
		// creating two tasks
		xTaskCreate( vTask1_menu_display,"Task-1", 500 , NULL, 1, &xTaskHandle1);

		xTaskCreate( vTask2_cmd_handling,"Task-2", 500, &count, 1, &xTaskHandle2);

		xTaskCreate(vTask3_cmd_processing, "Task-3",500, NULL, 1, &xTaskHandle3);

		xTaskCreate(vTask4_uart_write, "Task-4",500, NULL, 1, &xTaskHandle4);

		vTaskStartScheduler();
		}
	for(;;);
}

void vTask1_menu_display(void *params)
{
	char *pdata = menu;
	while(1)
	{
		xQueueSend(Q2,&pdata,portMAX_DELAY);
		printf("queue filled\n");
		while(xTaskNotifyWait(0,0,NULL,0xffffffff) == pdFALSE);
	}
}

void vTask2_cmd_handling(void *params)
{
	uint8_t i= params;
	printf("datacmd is %d\n",i);
	app_t *data;
	printf("task 2 runs\n");
	while(1)
	{
		xTaskNotifyWait(0,0,NULL,0xffffffff);
	//	data->cmd = params;
		xQueueSend(Q1,&data,portMAX_DELAY);
	}
}

void vTask3_cmd_processing(void *params)
{
	app_t *data = NULL;
	printf("task 3 runs\n");
	while(1)
	{
		xQueueReceive(Q1,&data,portMAX_DELAY);
	//	printf("command entered in task 3 is %d\n",data->cmd);
	}
}

void vTask4_uart_write(void *params)
{
	char *ptr = NULL;
	while(1)
	{
		printf("task 4 entered\n");
		xQueueReceive(Q2,&ptr,portMAX_DELAY);
	//	char *msg = ptr;
		for(int i=0;i<strlen(ptr);i++)
		{
			printf("%c",ptr[i]);
		}
		printf("\r\n");
	}
}

void rtos_delay(uint32_t delay_ms)
{
	uint32_t current_tick = xTaskGetTickCount();
	uint32_t delay_ticks = (configTICK_RATE_HZ * delay_ms)/ 1000;
	while( xTaskGetTickCount() < (current_tick + delay_ticks));

}

void EXTI15_10_IRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = 0;
	// clear the flag first
	EXTI_ClearITPendingBit(EXTI_Line13);
	xTaskNotifyFromISR(xTaskHandle1,0,eNoAction,&xHigherPriorityTaskWoken);
	xTaskNotifyFromISR(xTaskHandle2,0,eNoAction,&xHigherPriorityTaskWoken);
	if(xHigherPriorityTaskWoken)
	{
		taskYIELD();
	}
}

static void prvSetupHardware(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	GPIO_InitTypeDef led_init, button_init;

	//clearing the pins before initialising
		memset(&led_init, 0, sizeof(led_init));

	led_init.GPIO_Mode  = GPIO_Mode_OUT;
	led_init.GPIO_Pin = GPIO_Pin_5;
	led_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	led_init.GPIO_Speed = GPIO_Medium_Speed;
	led_init.GPIO_OType = GPIO_OType_PP;

	GPIO_Init(GPIOA,&led_init);

	//clearing the pins before initialising
			memset(&button_init, 0, sizeof(button_init));
	button_init.GPIO_Mode = GPIO_Mode_IN;
	button_init.GPIO_OType  = GPIO_OType_PP;
	button_init.GPIO_Pin = GPIO_Pin_13;
	button_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	button_init.GPIO_Speed = GPIO_Medium_Speed;;

	GPIO_Init(GPIOC,&button_init);

	// EXTI 13 setup
		SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource13);

		EXTI_InitTypeDef exti_init;

		exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
		exti_init.EXTI_Line = EXTI_Line13;
		exti_init.EXTI_Trigger = EXTI_Trigger_Falling;
		exti_init.EXTI_LineCmd = ENABLE;

		EXTI_Init(&exti_init);

		NVIC_SetPriority(EXTI15_10_IRQn, 5);
		NVIC_EnableIRQ(EXTI15_10_IRQn);

}
