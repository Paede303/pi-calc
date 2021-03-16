/*
 * main.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : chaos
 */ 

//#include <avr/io.h>
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"

#include "mem_check.h"
#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"

#include <math.h>

uint8_t sec=0;

#define BIT_0 (1<<0)
#define BIT_1 (1<<1)

extern void vApplicationIdleHook( void );
//void vLedBlink(void *pvParameters);
void TimerTask(void *pvParameters);
void DisplayTask(void *pvParameters);
void ButtonTask(void *pvParameters);
void LeibnizTask(void *pvParameters);

//TaskHandle_t ledTask;
TaskHandle_t buttonTask1;
TaskHandle_t buttonTask2;
TaskHandle_t buttonTask3;
TaskHandle_t buttonTask4;

EventGroupHandle_t TimerEventGroup;


void vApplicationIdleHook( void )
{	
	
}

void initTimer(void) {	
	
	//TCC0.CTRLA = TC_TC0_CLKSEL_DIV64_gc;	
	TCD0.CTRLA = TC_CLKSEL_DIV64_gc;
	TCD0.CTRLB = 0x00;
	TCD0.INTCTRLA = 0x03;
	TCD0.PER = 5000;	

	//Config OC1A & OC1B as PWM outputs an  PD4 und PD5
/*	PORTD.DIR |= 0x30;
	TCD1.CTRLA = TC_CLKSEL_DIV8_gc;	
	TCD1.CTRLB = TC_WGMODE_SINGLESLOPE_gc | TC0_CCAEN_bm | TC0_CCBEN_bm;	
	TCD1.CTRLD = 0x00;
	TCD1.PER = 40000;	
	TCD1.CCA = 35500; //Umschaltwert für Servo1
	TCD1.CCB = 35500; //Umschaltwert für Servo2	*/
}
int main(void)
{
    resetReason_t reason = getResetReason();

	vInitClock();
	vInitDisplay();


	xTaskCreate( TimerTask, (const char *) "TimeTask", configMINIMAL_STACK_SIZE+100, NULL, 2, NULL);
	xTaskCreate( DisplayTask, (const char *) "Display", configMINIMAL_STACK_SIZE+100, NULL, 2, NULL);
	xTaskCreate( LeibnizTask, (const char *) "Leibniz", configMINIMAL_STACK_SIZE+100, NULL, 2, NULL);


	vDisplayClear();
	vDisplayWriteStringAtPos(0,0,"FreeRTOS 10.0.1");
	vDisplayWriteStringAtPos(1,0,"EDUBoard 1.0");
	vDisplayWriteStringAtPos(2,0,"Template");
	vDisplayWriteStringAtPos(3,0,"ResetReason: %d", reason);
	vTaskStartScheduler();
	return 0;
}


ISR(TCD0_OVF_vect)
{
	//static uint8_t count = 1;
	BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	xEventGroupSetBitsFromISR(TimerEventGroup,BIT_0,&xHigherPriorityTaskWoken );

}
void TimerTask(void *pvParameters)
{
	uint8_t zell_verdammt;
	(void) pvParameters;
	PORTF.DIRSET = PIN0_bm; /*LED1*/
	PORTF.OUT = 0x01;
	TimerEventGroup = xEventGroupCreate();
	initTimer();
	for(;;) 
	{
		//xEventGroupWaitBits(
		//TimerEventGroup,		/* The event group being tested. */
		//BIT_0,					/* The bits within the event group to wait for. */
		//pdTRUE,					/* BIT_0 should be cleared before returning. */
		//pdTRUE,					/* Don't wait for both bits, either bit will do. */
		//portMAX_DELAY);			/* Wait a maximum of 100ms for either bit to be set. */
		zell_verdammt++;
		if(zell_verdammt == 50)			//10ms *100 -> 1s
		{
			//display_aktualisieren = 1;
		}
		else
		{
			//display_aktualisieren = 0;
		}
	}
}
void DisplayTask(void *pvParameters)
{
//	xEventGroupWaitBits(
//	TimerEventGroup,		/* The event group being tested. */
//	BIT_1,					/* The bits within the event group to wait for. */
//	pdTRUE,					/* BIT_0 should be cleared before returning. */
//	pdTRUE,					/* Don't wait for both bits, either bit will do. */
//	portMAX_DELAY);			/* Wait a maximum of 100ms for either bit to be set. */
	while(1)
	{
		// 		uint32_t stack = get_mem_unused();
		// 		uint32_t heap = xPortGetFreeHeapSize();
		// 		uint32_t taskStack = uxTaskGetStackHighWaterMark(ledTask);
		// 		vDisplayClear();
		 		//vDisplayWriteStringAtPos(0,0,"Time: %d:%d:%d", hr,min,sec);
		 		//vDisplayWriteStringAtPos(1,0,"Alarm: %d:%d:%d", hr_set,min_set,sec_set);
		 		//vDisplayWriteStringAtPos(2,0,"TaskStack: %d", taskStack);
		 		//vDisplayWriteStringAtPos(3,0,"FreeSpace: %d", stack+heap);
		PORTF.OUTTGL = 0x01;
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}
void LeibnizTask(void *pvparameters)
{
	double zahl=0;
	uint32_t genauigkeit = 10000000;
	uint32_t i;
	double pi;
	
	while(1)
	{
		for(i = 1; i <= genauigkeit; i++)
		{	
			zahl += pow(-1.0,i+1)/(2*i-1);			
		}
		pi = zahl*4;
		vDisplayClear();
		vDisplayWriteStringAtPos(0,0,"Leibniz %f", pi);
		vTaskDelay(1000);
	}
}