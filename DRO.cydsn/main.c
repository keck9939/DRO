/* ========================================
*
* Copyright YOUR COMPANY, THE YEAR
* All Rights Reserved
* UNPUBLISHED, LICENSED SOFTWARE.
*
* CONFIDENTIAL AND PROPRIETARY INFORMATION
* WHICH IS THE PROPERTY OF your company.
*
* ========================================
*/
#include "device.h"

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

extern void mainTask(void*);
extern void monitorTask(void*);
void prvHardwareSetup( void );
TaskHandle_t mainTaskHandle;

int main(void)
{
	prvHardwareSetup();
	/* Create one of the two tasks. */
	xTaskCreate(mainTask,		/* Pointer to the function that implements the task. */
	"Main",	    /* Text name for the task.  This is to facilitate debugging only. */
	256,		/* Stack depth - most small microcontrollers will use much less stack than this. */
	NULL,		/* We are not using the task parameter. */
	1,			/* This task will run at priority 1. */
	&mainTaskHandle );
	
	xTaskCreate(monitorTask,		/* Pointer to the function that implements the task. */
	"Monitor",	    /* Text name for the task.  This is to facilitate debugging only. */
	256,		/* Stack depth - most small microcontrollers will use much less stack than this. */
	NULL,		/* We are not using the task parameter. */
	2,			/* This task will run at priority 2. */
	NULL );		/* We are not using the task handle. */
	
	/* Will only get here if there was insufficient memory to create the idle
	task.  The idle task is created within vTaskStartScheduler(). */
	vTaskStartScheduler();

	/* Should never reach here as the kernel will now be running.  If
	vTaskStartScheduler() does return then it is very likely that there was
	insufficient (FreeRTOS) heap space available to create all the tasks,
	including the idle task that is created within vTaskStartScheduler() itself. */
	for( ;; );
}

void prvHardwareSetup( void )
{
	/* Port layer functions that need to be copied into the vector table. */
	extern void xPortPendSVHandler( void );
	extern void xPortSysTickHandler( void );
	extern void vPortSVCHandler( void );
	extern cyisraddress CyRamVectors[];

	/* Install the OS Interrupt Handlers. */
	CyRamVectors[ 11 ] = ( cyisraddress ) vPortSVCHandler;
	CyRamVectors[ 14 ] = ( cyisraddress ) xPortPendSVHandler;
	CyRamVectors[ 15 ] = ( cyisraddress ) xPortSysTickHandler;

	UART_Start();
	QuadDec_1_Start();
	QuadDec_2_Start();
	QuadDec_3_Start();
	Tach_Start();
}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	/* The stack space has been execeeded for a task, considering allocating more. */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*---------------------------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* The heap space has been execeeded. */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}


/* [] END OF FILE */
