#include <stdio.h>
#include "xil_types.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xparameters.h"
#include "xgpiops.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xscugic.h"

static XGpioPs psGpioInstancePtr;
extern XGpioPs_Config XGpioPs_ConfigTable[XPAR_XGPIOPS_NUM_INSTANCES];
static int iPinNumber = 7;
XScuGic InterruptController;
static int InterruptFlag;
extern char inbyte(void);

#define TIMER_CNTR_0 0
#define TIMER_IRQID (XPAR_FABRIC_AXI_TIMER_0_INTR + 32)

void Timer_InterruptHandler(void *data, u8 TmrCtrNumber)
{
    print("\r\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
    print(" Inside Timer ISR \n\r");
    XTmrCtr_Stop(data, TmrCtrNumber);
    print("LED 'LD4' Turned ON \r\n");
    XGpioPs_WritePin(&psGpioInstancePtr, iPinNumber, 1);
    XTmrCtr_Reset(data, TmrCtrNumber);
    print(" Timer ISR Exit\n\r");
    print("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
    InterruptFlag = 1;
}

int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr)
{
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
        (Xil_ExceptionHandler) XScuGic_InterruptHandler,
        XScuGicInstancePtr);
    Xil_ExceptionEnable();
    return XST_SUCCESS;
}

/*int ScuGicInterrupt_Init(u32 DeviceId, XTmrCtr *TimerInstancePtr)
{
    int Status;
    XScuGic_Config *GicConfig;

    GicConfig = XScuGic_LookupConfig(DeviceId);
    if (NULL == GicConfig) return XST_FAILURE;

    Status = XScuGic_CfgInitialize(&InterruptController, GicConfig,
        GicConfig->CpuBaseAddress);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    Status = SetUpInterruptSystem(&InterruptController);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    Status = XScuGic_Connect(&InterruptController,
        XPAR_FABRIC_AXI_TIMER_0_INTR,
        (Xil_ExceptionHandler) XTmrCtr_InterruptHandler,
        (void *)TimerInstancePtr);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    XScuGic_Enable(&InterruptController, XPAR_FABRIC_AXI_TIMER_0_INTR);
    return XST_SUCCESS;
}*/

int ScuGicInterrupt_Init(u32 DeviceId, XTmrCtr *TimerInstancePtr)
{
    int Status;
    XScuGic_Config *GicConfig;

    GicConfig = XScuGic_LookupConfig(DeviceId);
    if (NULL == GicConfig) {
        print("GIC LookupConfig FAILED\r\n");
        return XST_FAILURE;
    }
    print("GIC LookupConfig OK\r\n");

    Status = XScuGic_CfgInitialize(&InterruptController, GicConfig,
        GicConfig->CpuBaseAddress);
    if (Status != XST_SUCCESS) {
        print("GIC CfgInitialize FAILED\r\n");
        return XST_FAILURE;
    }
    print("GIC CfgInitialize OK\r\n");

    Status = SetUpInterruptSystem(&InterruptController);
    if (Status != XST_SUCCESS) {
        print("SetUpInterruptSystem FAILED\r\n");
        return XST_FAILURE;
    }
    print("SetUpInterruptSystem OK\r\n");

    Status = XScuGic_Connect(&InterruptController,
        TIMER_IRQID,
        (Xil_ExceptionHandler) XTmrCtr_InterruptHandler,
        (void *)TimerInstancePtr);
    if (Status != XST_SUCCESS) {
        print("GIC Connect FAILED\r\n");
        return XST_FAILURE;
    }
    print("GIC Connect OK\r\n");

    XScuGic_Enable(&InterruptController, TIMER_IRQID);
    print("GIC Enable OK\r\n");

    return XST_SUCCESS;
}

int main()
{
    u8 TmrCtrNumber = TIMER_CNTR_0;
    static XGpio GPIOInstance_Ptr;
    XGpioPs_Config *GpioConfigPtr;
    XTmrCtr TimerInstancePtr;
    int xStatus;
    u32 Readstatus = 0, OldReadStatus = 0;
    int iPinNumberEMIO = 54;
    u32 uPinDirectionEMIO = 0x0;
    u32 uPinDirection = 0x1;
    int exit_flag, choice, internal_choice;

    print("##### Application Starts #####\n\r\n");

    xStatus = XGpio_Initialize(&GPIOInstance_Ptr, XPAR_AXI_GPIO_0_BASEADDR);
    if (XST_SUCCESS != xStatus) print("GPIO INIT FAILED\n\r");

    XGpio_SetDataDirection(&GPIOInstance_Ptr, 1, 1);

    xStatus = XTmrCtr_Initialize(&TimerInstancePtr, XPAR_AXI_TIMER_0_BASEADDR);
    if (XST_SUCCESS != xStatus) print("TIMER INIT FAILED\n\r");

    XTmrCtr_SetHandler(&TimerInstancePtr, Timer_InterruptHandler, &TimerInstancePtr);
    XTmrCtr_SetResetValue(&TimerInstancePtr, 0, 0xf0000000);
    XTmrCtr_SetOptions(&TimerInstancePtr, TmrCtrNumber, XTC_INT_MODE_OPTION);
   

    GpioConfigPtr = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_BASEADDR);
    if (GpioConfigPtr == NULL) return XST_FAILURE;

    xStatus = XGpioPs_CfgInitialize(&psGpioInstancePtr, GpioConfigPtr,
        GpioConfigPtr->BaseAddr);
    if (XST_SUCCESS != xStatus) print("PS GPIO INIT FAILED\n\r");

    XGpioPs_SetDirectionPin(&psGpioInstancePtr, iPinNumber, uPinDirection);
    XGpioPs_SetOutputEnablePin(&psGpioInstancePtr, iPinNumber, 1);
    XGpioPs_WritePin(&psGpioInstancePtr, iPinNumber, 1);

    XGpioPs_SetDirectionPin(&psGpioInstancePtr, iPinNumberEMIO, uPinDirectionEMIO);
    XGpioPs_SetOutputEnablePin(&psGpioInstancePtr, iPinNumberEMIO, 0);

    xStatus = ScuGicInterrupt_Init(XPAR_XSCUGIC_0_BASEADDR, &TimerInstancePtr);


    if (XST_SUCCESS != xStatus) print(":( SCUGIC INIT FAILED\n\r");

    exit_flag = 0;
    while (exit_flag != 1)
    {
        print(" SELECT the Operation from the Below Menu \r\n");
        print("###################### Menu Starts ########################\r\n");
        print("Press '1' to use NORMAL GPIO as an input (btn0)\r\n");
        print("Press '2' to use EMIO as an input (btn1)\r\n");
        print("Press any other key to Exit\r\n");
        print(" ##################### Menu Ends #########################\r\n");

        choice = inbyte();
        printf("Selection : %c \r\n", choice);
        internal_choice = 1;

        switch(choice)
        {
            case '1':
                exit_flag = 0;
                print("Press btn0 push button on board \r\n");
                while(XGpio_DiscreteRead(&GPIOInstance_Ptr, 1) == 1);
                while (internal_choice != '0')
                {
                    Readstatus = XGpio_DiscreteRead(&GPIOInstance_Ptr, 1);
                    if (1 == Readstatus && 0 == OldReadStatus)
                    {
                        print("btn0 pressed \n\r");
                        print("LED 'LD4' Turned OFF \r\n");
                        XGpioPs_WritePin(&psGpioInstancePtr, iPinNumber, 0);
                        XTmrCtr_Start(&TimerInstancePtr, 0);
                        print("timer start \n\r");

                        u32 ctrlReg = XTmrCtr_GetOptions(&TimerInstancePtr, 0);
                        char buf[64];
                        sprintf(buf, "Timer options: 0x%08X\r\n", (unsigned int)ctrlReg);
                        print(buf);
                        u32 val = XTmrCtr_GetValue(&TimerInstancePtr, 0);
                        sprintf(buf, "Timer value: 0x%08X\r\n", (unsigned int)val);
                        print(buf);

                        print("Wait for the Timer interrupt to trigger \r\n");
                        while (InterruptFlag != 1);
                        InterruptFlag = 0;
                        print("Press '0' to go to Main Menu \n\r");
                        internal_choice = inbyte();
                        if (internal_choice != '0')
                            print("Press btn0 push button on board \r\n");
                    }
                    OldReadStatus = Readstatus;
                }
                break;

            case '2':
                exit_flag = 0;
                print("Press btn1 push button on board \r\n");
                while(XGpioPs_ReadPin(&psGpioInstancePtr, iPinNumberEMIO) == 1);
                while (internal_choice != '0')
                {
                    Readstatus = XGpioPs_ReadPin(&psGpioInstancePtr, iPinNumberEMIO);
                    if (1 == Readstatus && 0 == OldReadStatus)
                    {
                        print("btn1 pressed \n\r");
                        print("LED 'LD4' Turned OFF \r\n");
                        XGpioPs_WritePin(&psGpioInstancePtr, iPinNumber, 0);
                        XTmrCtr_Start(&TimerInstancePtr, 0);
                        print("timer start \n\r");



                        u32 ctrlReg = XTmrCtr_GetOptions(&TimerInstancePtr, 0);
                        char buf[64];
                        sprintf(buf, "Timer options: 0x%08X\r\n", (unsigned int)ctrlReg);
                        print(buf);
                        u32 val = XTmrCtr_GetValue(&TimerInstancePtr, 0);
                        sprintf(buf, "Timer value: 0x%08X\r\n", (unsigned int)val);
                        print(buf);


                        
                        print("Wait for the Timer interrupt to trigger \r\n");
                        while (InterruptFlag != 1);
                        InterruptFlag = 0;
                        print("Press '0' to go to Main Menu \n\r");
                        internal_choice = inbyte();
                        if (internal_choice != '0')
                            print("Press btn1 push button on board \r\n");
                    }
                    OldReadStatus = Readstatus;
                }
                break;

            default:
                exit_flag = 1;
                break;
        }
    }

    print("\r\n***********\r\nBYE \r\n***********\r\n");
    return 0;
}