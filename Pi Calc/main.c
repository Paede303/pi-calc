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
#include "ButtonHandler.h"

#include <math.h>

#define interuppt10ms 1<<0
#define ALGORITHMUS 1<<1
#define STARTEN_GAUSS 1<<2
#define LEIBNIZ_PI_RDY 1<<3
#define GAUSS_PI_RDY 1<<4
#define RESET 1<<5
#define STARTEN_LEIBNIZ 1<<6
#define READY 1<<7
#define LEIBNIZ_CALC_RDY 1<<8
#define STARTEN 1<<9
#define READY_GAUSS 1<<10
#define STOP_LEIBNIZ 1<<11
#define STOP_GAUSS 1<<12
#define GAUSS_CALC_RDY 1<<13



typedef enum{
Anzeige,
Leibniz,
Gauss,
HALT_STOP
}Displaymodi_t;

typedef enum{
Start,
Stop,
Fertig
}Picalc_t;

uint8_t display_aktualisieren;
uint32_t TIME_Leibniz;
uint32_t Zeit_Leibniz;
uint32_t Zeit_Gauss;
uint32_t TIME_Gauss;
float pi_leibniz;
float pi_Gauss;


extern void vApplicationIdleHook( void );
//void vLedBlink(void *pvParameters);
void TimerTask2(void *pvParameters);
void DisplayTask(void *pvParameters);
void ButtonTask(void *pvParameters);
void LeibnizTask(void *pvParameters);
void GaussTask(void *pvParameters);

EventGroupHandle_t TimerEventGroup = NULL;


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
	initButtons();
	
	TimerEventGroup = xEventGroupCreate();
	xTaskCreate( TimerTask2, (const char *) "ScheissTask", configMINIMAL_STACK_SIZE+150, NULL, 1, NULL);
	xTaskCreate( DisplayTask, (const char *) "Display", configMINIMAL_STACK_SIZE+150, NULL, 2, NULL);
	xTaskCreate( LeibnizTask, (const char *) "Leibniz", configMINIMAL_STACK_SIZE+150, NULL, 1, NULL);
	xTaskCreate( GaussTask, (const char *) "Gauss", configMINIMAL_STACK_SIZE+150, NULL, 1, NULL);

	vTaskStartScheduler();
	return 0;
}


ISR(TCD0_OVF_vect)
{
	//static uint8_t count = 1;
	BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	xEventGroupSetBitsFromISR(TimerEventGroup,interuppt10ms,&xHigherPriorityTaskWoken );
}

void TimerTask2(void *pvParameters)
{	
	(void) pvParameters;
	uint8_t zell_verdammt;
	uint8_t Zeit_Messung;

	initTimer();
	while(1) 
	{
		xEventGroupWaitBits(
		TimerEventGroup,		/* The event group being tested. */
		interuppt10ms,					/* The bits within the event group to wait for. */
		pdTRUE,					/* BIT_0 should be cleared before returning. */
		pdTRUE,					/* Don't wait for both bits, either bit will do. */
		portMAX_DELAY);			/* Wait a maximum of 100ms for either bit to be set. */
		zell_verdammt++;
		Zeit_Messung = 0;
		if(zell_verdammt == 50)			//10ms *50 -> 500ms
		{
			display_aktualisieren = 1;
			zell_verdammt =0;
		}
		else
		{
			display_aktualisieren = 0;
		}
		vTaskDelay(1);
	}
}

void DisplayTask(void *pvParameters)
{
	static Displaymodi_t Displaymode = Anzeige;
	uint16_t buttonstate;
	uint8_t modecount = 0;
	while(TimerEventGroup == NULL) 
	{
		vTaskDelay(1);
	}

	while(1)
	{
		buttonstate = xEventGroupGetBits(TimerEventGroup);
		updateButtons();
		if(getButtonPress(BUTTON1) == SHORT_PRESSED)
		{
			xEventGroupSetBits(TimerEventGroup,STARTEN);
		}
		if(getButtonPress(BUTTON2) == SHORT_PRESSED)
		{
			xEventGroupSetBits(TimerEventGroup,Stop);
			xEventGroupClearBits(TimerEventGroup,STARTEN);
		}
		if(getButtonPress(BUTTON3) == SHORT_PRESSED)
		{
			xEventGroupSetBits(TimerEventGroup,RESET);
		}
		if(getButtonPress(BUTTON4) == SHORT_PRESSED)
		{
			xEventGroupSetBits(TimerEventGroup,ALGORITHMUS);
		}
		switch(Displaymode)
		{
			case Anzeige:
					vDisplayClear();
		 			vDisplayWriteStringAtPos(0,0,"Leibniz Pi: %f");
		 			vDisplayWriteStringAtPos(1,0,"Gauss Pi: %f");
					 

				if(buttonstate & ALGORITHMUS)
				{
					Displaymode = Leibniz;
					xEventGroupClearBits(TimerEventGroup,ALGORITHMUS);				
				}
			break;
			
			case Leibniz:
		
				if(buttonstate & STARTEN)
				{
					xEventGroupSetBits(TimerEventGroup,STARTEN_LEIBNIZ);
					xEventGroupSetBits(TimerEventGroup,READY);
					xEventGroupWaitBits(TimerEventGroup,LEIBNIZ_CALC_RDY,pdFALSE,pdTRUE,portMAX_DELAY);
					if(xEventGroupGetBits(TimerEventGroup)&LEIBNIZ_CALC_RDY!=0)
					{
						vDisplayClear();
						vDisplayWriteStringAtPos(0,0,"Leibniz Pi:%f", pi_leibniz);
						vDisplayWriteStringAtPos(1,0,"Gauss Pi:%f");
						vDisplayWriteStringAtPos(2,0,"Zeit Leibniz:%dms",Zeit_Leibniz);
						xEventGroupClearBits(TimerEventGroup,LEIBNIZ_CALC_RDY);
						xEventGroupClearBits(TimerEventGroup,READY);
					}
					if(xEventGroupGetBits(TimerEventGroup)&LEIBNIZ_PI_RDY)
					{
						vDisplayClear();
						vDisplayWriteStringAtPos(0,0,"Leibniz Pi:%f",pi_leibniz);
						vDisplayWriteStringAtPos(1,0,"Gauss Pi:%f");
						vDisplayWriteStringAtPos(2,0,"Zeit Leibniz:%dms",TIME_Leibniz);
					}
				}
				if(buttonstate & ALGORITHMUS)
				{
					Displaymode = Gauss;
					xEventGroupClearBits(TimerEventGroup,ALGORITHMUS);
				}
				
			break;
			
			case Gauss:
				if(buttonstate & STARTEN)
				{
					xEventGroupSetBits(TimerEventGroup,STARTEN_GAUSS);
					xEventGroupSetBits(TimerEventGroup,READY_GAUSS);
					xEventGroupWaitBits(TimerEventGroup,GAUSS_CALC_RDY,pdTRUE,pdTRUE,portMAX_DELAY);
					if(xEventGroupGetBits(TimerEventGroup)&GAUSS_CALC_RDY!=0)
					{
						vDisplayClear();
						vDisplayWriteStringAtPos(0,0,"Leibniz Pi:%f");
						vDisplayWriteStringAtPos(1,0,"Gauss Pi:%f",pi_Gauss);
						vDisplayWriteStringAtPos(2,0,"Zeit Leibniz:%dms",Zeit_Gauss);
						xEventGroupClearBits(TimerEventGroup,GAUSS_CALC_RDY);						
					}
					if(xEventGroupGetBits(TimerEventGroup)&GAUSS_PI_RDY)
					{
						vDisplayClear();
						vDisplayWriteStringAtPos(0,0,"Leibniz Pi:%f");
						vDisplayWriteStringAtPos(1,0,"Gauss Pi:%f",pi_Gauss);
						vDisplayWriteStringAtPos(2,0,"Zeit Leibniz:%dms",TIME_Gauss);
					}
				}
				if(buttonstate&ALGORITHMUS)
				{
					Displaymode=Leibniz;
					xEventGroupClearBits(TimerEventGroup,ALGORITHMUS);
				}
			break;
				


		
		}
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}

void LeibnizTask(void *pvparameters)
{
	double zahl=0;
	uint32_t genauigkeit = 10000000;
	uint32_t i;
	while(TimerEventGroup == NULL) 
	{
		vTaskDelay(1);
	}
	
	while(1)
	{

		if(xEventGroupGetBits(TimerEventGroup)&STARTEN_LEIBNIZ)
		{
			

				for(i = 1; i <= genauigkeit; i++)
				{
					Zeit_Leibniz++;
					zahl += pow(-1.0,i+1)/(2*i-1);
					pi_leibniz = zahl*4;
						
					if(xEventGroupGetBits(TimerEventGroup)&READY)
					{
						//xEventGroupClearBits(TimerEventGroup,STARTEN_LEIBNIZ);
						xEventGroupSetBits(TimerEventGroup,LEIBNIZ_CALC_RDY);
						//xEventGroupWaitBits(TimerEventGroup,STARTEN_LEIBNIZ,pdFALSE,pdTRUE,portMAX_DELAY);
						//xEventGroupClearBits(TimerEventGroup,READY);
					}
					else
					{
						xEventGroupClearBits(TimerEventGroup,LEIBNIZ_CALC_RDY);
					}
					if(xEventGroupGetBits(TimerEventGroup)&RESET)
					{
						zahl = 0;
						i = genauigkeit;
						pi_leibniz = 0;
						xEventGroupClearBits(TimerEventGroup,RESET|STARTEN_LEIBNIZ);
					}				
					if(pi_leibniz == 3.14159)
					{
						TIME_Leibniz = Zeit_Leibniz;
						xEventGroupSetBits(TimerEventGroup,LEIBNIZ_PI_RDY);
					}
					else
					{
						xEventGroupClearBits(TimerEventGroup,LEIBNIZ_PI_RDY);
					}
				}
			
		}
	}
}

void GaussTask(void *pvParameters)
{
	float a = 1;
	float b = 1/sqrt(2);
	float s = 0.5;
	float hilf;
	float c;
	uint8_t n;
	while(TimerEventGroup == NULL) 
	{
		vTaskDelay(1);
	}
	
	while(1)
	{

			if(xEventGroupGetBits(TimerEventGroup)&STARTEN_GAUSS)
			{
				for(n = 1; n <= 3; n++)
				{
					Zeit_Gauss++;
					hilf = a;
					a = (a+b)/2;
					b = sqrt(hilf*b);
					c = pow(a,2)-pow(b,2);
					s = s-pow(2,n)*c;
					pi_Gauss = (2*pow(a,2))/s;
					if(xEventGroupGetBits(TimerEventGroup)&READY_GAUSS)
					{
						//xEventGroupClearBits(TimerEventGroup,STARTEN_LEIBNIZ);
						xEventGroupSetBits(TimerEventGroup,GAUSS_CALC_RDY);
						//xEventGroupWaitBits(TimerEventGroup,STARTEN_LEIBNIZ,pdFALSE,pdTRUE,portMAX_DELAY);
						//xEventGroupClearBits(TimerEventGroup,READY);
					}
					else
					{
						xEventGroupClearBits(TimerEventGroup,GAUSS_CALC_RDY);
					}
					if(xEventGroupGetBits(TimerEventGroup)&RESET)
					{
						n = 4;
						a = 1;
						b = 1/sqrt(2);
						s = 0.5;
						c = 0;
						hilf = 0;
						pi_Gauss = 0;
						xEventGroupClearBits(TimerEventGroup,RESET|STARTEN_GAUSS);
					}
					if(n == 3)
					{
						TIME_Gauss = Zeit_Gauss;
						xEventGroupSetBits(TimerEventGroup,GAUSS_PI_RDY);

					}
					else
					{
						xEventGroupClearBits(TimerEventGroup,GAUSS_PI_RDY);
					}
				}
			}

	}
}