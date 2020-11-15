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
#include "semphr.h"

#include <stdio.h>
#include <stdlib.h>

// This is used to allow the ISR to wake the input processing task when
// something is received from the display.
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
extern SemaphoreHandle_t TaskSync;
extern BaseType_t hUpdated;

void SaveData();

void monitorTask(void* p)
{
	xTaskToNotify = xTaskGetCurrentTaskHandle( );
    LED_Write(1);   // debug aid
	
	RXInt_StartEx(RXISR);
    vTaskDelay(pdMS_TO_TICKS( 2000 ));

	UART_ClearRxBuffer();
	buffercount = 0;
	ulTaskNotifyTake(pdTRUE, 0);
	
	for(;;)
	{
// wait for data from display
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		
// we need more than 3 characters since any message will contain 3 0xff's.
        if (buffercount > 3)
		{
			switch(buf[0])
			{
			case 0x65:  // touch event
                if (buffercount == 7)   // lenght should be 7
                {
    				if (buf[1] == 1)    // page 1
    				{
    					if (buf[2] == 6)    // OK button on page 1 (set hysteresis)
    					{
    						hind = 0;
    						iprintf("get %c.val\xFF\xFF\xFF", axis[hind]);  // request x axis value
    						fflush(stdout);
    					}
    				}
    				else    // page 0
                    {
    					if (buf[2] == 7)    // Long RPM press
    					{
    						xSemaphoreTake(TaskSync, portMAX_DELAY);
                            LED_Write(0);
                            iprintf("page 1\xFF\xFF\xFF");  // select page
    						fflush(stdout);
    						vTaskDelay(pdMS_TO_TICKS( 20 ));
    						iprintf("x.val=%i\xFF\xFF\xFF", h[0]);  // hysteresis for X
    						fflush(stdout);
#ifndef LATHE
    						iprintf("y.val=%i\xFF\xFF\xFF", h[1]);  // hysteresis for Y
    						fflush(stdout);
#endif
    						iprintf("z.val=%i\xFF\xFF\xFF", h[2]);  // hysteresis for Z
    						fflush(stdout);
    					}
                        else    // long press on axis
    				        zero[buf[2]/2-1] = pdTRUE;  // zero correspoinding axis
                    }
                }
				break;
			case 0x68:  // exit sleep
                if (buffercount == 9)
				    sleep  = pdFALSE;
				break;
			case 0x71:  // returned data
                if (buffercount == 8)   // must be 8 long
                {
    				h[hind] = buf[1];   // read hysteresis values as they are sent
    				if (++hind < 3) // get remaining values
    				{
#ifdef LATHE
                        if (axis[hind] == 'y')
                            hind++;
#endif
                            
                        iprintf("get %c.val\xFF\xFF\xFF", axis[hind]);  // request value
        			    fflush(stdout);
    				}
    				else    // go back to page 0 and resume main task
    				{
    					iprintf("page 0\xFF\xFF\xFF");
    					fflush(stdout);
                        SaveData();
    					vTaskDelay(pdMS_TO_TICKS( 20 ));
                        hUpdated = pdTRUE;
                        xSemaphoreGive(TaskSync);   // release the main task
    				}
                }
                break;
			case 0x86:  // display has gone to sleep
                if (buffercount == 4)
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

void SaveData()
{
    int i;
    
    for (i = 0; i < 3; i++)
    {
        if (EEPROM_ReadByte(i) != h[i])
            EEPROM_WriteByte(h[i], i);
    }
}


// UART receive ISR. The Nextion has a syncronization system of 3 0xff's in a row
// which actually proves to be surprisingly awkward.
CY_ISR(RXISR)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if ((UART_RXSTATUS_REG & UART_RX_STS_FIFO_NOTEMPTY) != 0u)
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
				if (FFCount == 3 || buffercount == 16)
				{
					vTaskNotifyGiveFromISR( xTaskToNotify, &xHigherPriorityTaskWoken );
					break;
				}
			}
			while((UART_RXSTATUS_REG & UART_RX_STS_FIFO_NOTEMPTY) != 0u);
		}
        else
        {
 			vTaskNotifyGiveFromISR( xTaskToNotify, &xHigherPriorityTaskWoken );
        }
	}
	RXInt_ClearPending();
}

/* [] END OF FILE */
