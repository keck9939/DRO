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
#include <math.h>
#include <stdlib.h>

void displayVal(int ax, int disp);
void ZeroAxis(int ax);

volatile static int disp[3] = {0, 0, 0};
int h[3] = {4, 3, 2};
volatile BaseType_t zero[3] = {pdFALSE, pdFALSE, pdFALSE};
volatile BaseType_t sleep = pdFALSE;
extern SemaphoreHandle_t TaskSync;
volatile BaseType_t hUpdated = pdFALSE;
static int tachVal;
static int lastTachVal;

void ResetValues()
{
	displayVal(0, disp[0]);
	displayVal(1, disp[1]);
	displayVal(2, disp[2]);
    iprintf("r.val=%i\xFF\xFF\xFF", 0);
	fflush(stdout);
}

void mainTask(void* nu)
{
	int cc;
    
	EEPROM_Start();
	
	for (cc = 0; cc < 3; cc++)
	{
		h[cc] = EEPROM_ReadByte(cc);
		if (h[cc] > 20 || h[cc] < 0)
		    h[cc] = cc+2;
	}
	
	vTaskDelay(pdMS_TO_TICKS( 2000 ));
	iprintf("ussp=600\xFF\xFF\xFF");   // sleep after 10 minutes of inactivity
	iprintf("thup=1\xFF\xFF\xFF");     // wake on touch
	fflush(stdout);
    ResetValues();
		
	for(;;)
	{
		vTaskDelay(pdMS_TO_TICKS( 20 ));
        xSemaphoreTake(TaskSync, portMAX_DELAY);
        if (hUpdated)
        {
            ResetValues();
            hUpdated = pdFALSE;
        }
        
		for (cc = 0; cc < 3; cc++)
		{
			if (zero[cc])
			{
				ZeroAxis(cc);
				zero[cc] = pdFALSE;
			}
		}
        
		cc = QuadDec_1_GetCounter();
		if (disp[0] < cc)
		{
			disp[0] = cc;
			displayVal(0, disp[0]);
		}
		else if (disp[0] > cc+h[0])
		{
			disp[0] = cc+h[0];
			displayVal(0, disp[0]);
		}
		
		cc = QuadDec_2_GetCounter();
		if (disp[1] < cc)
		{
			disp[1] = cc;
			displayVal(1, disp[1]);
		}
		else if (disp[1] > cc+h[1])
		{
			disp[1] = cc+h[1];
			displayVal(1, disp[1]);
		}

		cc = QuadDec_3_GetCounter();
		if (disp[2] < cc)
		{
			disp[2] = cc;
			displayVal(2, disp[2]);
		}
		else if (disp[2] > cc+h[2])
		{
			disp[2] = cc+h[2];
			displayVal(2, disp[2]);
		}
        
        while ((Tach_ReadStatusRegister() & Tach_STATUS_FIFONEMP) != 0)
        {
            tachVal = Tach_ReadCapture();
        }
        
        if (!(tachVal == 0 && lastTachVal == 0))
        {
            iprintf("r.val=%i\xFF\xFF\xFF", tachVal*10);
		    fflush(stdout);
            lastTachVal = tachVal;
        }
        
        xSemaphoreGive(TaskSync);
	}
}

char axis[] = {'x', 'y', 'z'};
void displayVal(int ax, int disp)
{
	div_t result;
	int ld;
	if (sleep)
	{
		iprintf("sleep=0\xFF\xFF\xFF");
		sleep = pdFALSE;
		fflush(stdout);
	}
	char neg = disp<0;
	ld = ((disp&1) == 1 ? 5 : 0);
	disp = disp/2;
	result = div(abs(disp), 1000);
	if (neg)
	    iprintf("%c.txt=\"-%i.%03i%i\"\xFF\xFF\xFF", axis[ax], result.quot, result.rem, ld);
	else
	    iprintf("%c.txt=\"%i.%03i%i\"\xFF\xFF\xFF", axis[ax], result.quot, result.rem, ld);
	fflush(stdout);
}

void ZeroAxis(int ax)
{
	int delta;
	switch(ax)
	{
	case 0:
		delta = QuadDec_1_GetCounter() - disp[0];;
		disp[0] = 0;
		QuadDec_1_SetCounter(delta);
		displayVal(0, disp[0]);
		break;
	case 1:
		delta = QuadDec_2_GetCounter() - disp[1];;
		disp[1] = 0;
		QuadDec_2_SetCounter(delta);
		displayVal(1, disp[1]);
		break;
	case 2:
		delta = QuadDec_3_GetCounter() - disp[2];;
		disp[2] = 0;
		QuadDec_3_SetCounter(delta);
		displayVal(2, disp[2]);
		break;
	}
}



/* [] END OF FILE */
