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
#include "project.h"
uint16 vaegtData[3];
uint16 lysData[11];
uint8 flowCount;
uint8 msg[30];
uint32 check;
//funktion til at læse vaegtsensorer
void vaegtADC(){
    for(int i=0; i<3; i++){
        uint32 snit = 0;
        VaegtMux_FastSelect(i);
        for(int b=0; b<10; b++){
        snit+=ADCVaegt_Read16();
        }
        vaegtData[i]=snit/10;
    }
}


//funktion til aflæsning af lyssensor
void lysADC(){
        for(int i = 0; i < 11; i++){
            LysMux_FastSelect(i);
            ADCLys_StartConvert();
            ADCLys_IsEndConversion(ADCLys_WAIT_FOR_RESULT);
            lysData[i] = ADCLys_GetResult16();
            ADCLys_StopConvert();
        }
}

// funktion til flowmåler
void flow(){
    flowCount = FlowCounter_ReadCounter();
}

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    VaegtMux_Start();
    ADCVaegt_Start();
    LysMux_Start();
    ADCLys_Start();
    UART_Start();
    CyDelay(500);
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */

    for(;;)
    {
        /* Place your application code here. */
        //læs sensor og counter
        flow();
        lysADC();
        vaegtADC();
        //gem data i array
        //gem flowcount
        
        msg[0]=flowCount; 
        //gem lysdat
        int a = 0;
        for(int i = 1; i < 23; i+=2){ 
        msg[i] = lysData[a] & 0x00FF;
        msg[i+1] = lysData[a]>>8 & 0x00FF;
        a++;
        } 
        //gem vægtdata
        a=0;
        for(int i=23; i<29; i+=2){ 
        msg[i] = vaegtData[a] & 0x00FF;
        msg[i+1] = vaegtData[a]>>8 & 0x00FF;
        a++;
        }
        //Lav checksum
        check=0;
        for(int i = 0; i < 29; i++){
            check += msg[i];
        }
        check = check & 0x000000FF;
        msg[29]=check;
        //send besked
        UART_PutArray(msg,30); 
        CyDelay(20); //24 = 42 Hz
    }
}

/* [] END OF FILE */
