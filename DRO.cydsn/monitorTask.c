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

#include <stdio.h>
#include <stdlib.h>

static TaskHandle_t xTaskToNotify = NULL;
volatile static uint8 buf[16];
volatile static int buffercount;
volatile static int FFCount = 0;

extern volatile BaseType_t zero[3];
extern volatile BaseType_t sleep;
CY_ISR_PROTO(RXISR);

extern int h[3];
static int hind;
extern char axis[3];
extern TaskHandle_t mainTaskHandle;

void monitorTask(void* p)
{
	xTaskToNotify = xTaskGetCurrentTaskHandle( );
	
	RXInt_StartEx(RXISR);
	
	UART_ClearRxBuffer();
	buffercount = 0;
	ulTaskNotifyTake(pdTRUE, 0);
	
	for(;;)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		
		if (buffercount > 3)
		{
			switch(buf[0])
			{
			case 0x65:
				if (buf[1] == 1)    // page 1
				{
					if (buf[2] == 6)    // OK button
					{
						hind = 0;
						iprintf("get %c.val\xFF\xFF\xFF", axis[hind]);
						fflush(stdout);
					}
				}
				else    // page 0
                {
					if (buf[2] == 7)    // Long RPM press
					{
						vTaskSuspend(mainTaskHandle);
                        iprintf("page 1\xFF\xFF\xFF");
						fflush(stdout);
						vTaskDelay(pdMS_TO_TICKS( 20 ));
						iprintf("x.val=%i\xFF\xFF\xFF", h[0]);
						fflush(stdout);
						iprintf("y.val=%i\xFF\xFF\xFF", h[1]);
						fflush(stdout);
						iprintf("z.val=%i\xFF\xFF\xFF", h[2]);
						fflush(stdout);
					}
                    else
				        zero[buf[2]/2-1] = pdTRUE;
                }
				break;
			case 0x68:
				sleep  = pdFALSE;
				break;
			case 71:
				h[hind] = buf[1];
				if (hind++ < 3) // get remaining values
				{
					iprintf("get %c.val\xFF\xFF\xFF", axis[hind]);
					fflush(stdout);
				}
				else    // go back to page 0 and resume main task
				{
					iprintf("page 0\xFF\xFF\xFF");
					fflush(stdout);
					vTaskDelay(pdMS_TO_TICKS( 20 ));
                    vTaskResume(mainTaskHandle);
				}
			case 0x86:
				sleep = pdTRUE;
				break;
			default:
				break;
			}
		}
		buffercount = 0;
		FFCount = 0;
	}
}


CY_ISR(RXISR)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	uint8_t readStatus = UART_RXSTATUS_REG;
	
	if ((readStatus & UART_RX_STS_FIFO_NOTEMPTY) != 0u)
	{
		if (FFCount != 3)
		{
			do
			{
				buf[buffercount] = UART_RXDATA_REG;
				if (buf[buffercount] == 0xff)
				    FFCount++;
				else
				    FFCount = 0;
				buffercount++;
				if (FFCount == 3)
				{
					vTaskNotifyGiveFromISR( xTaskToNotify, &xHigherPriorityTaskWoken );
					break;
				}
			}
			while((readStatus & UART_RX_STS_FIFO_NOTEMPTY) != 0u);
		}
		RXInt_ClearPending();
	}
}

/* [] END OF FILE */
