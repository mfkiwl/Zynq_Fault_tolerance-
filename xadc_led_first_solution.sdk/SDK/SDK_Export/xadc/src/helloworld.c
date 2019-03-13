#include <stdio.h>
#include "platform.h"
#include "xadcps.h"
#include "xgpiops.h"
#include "xil_types.h"
#include "Xscugic.h"
#include "Xil_exception.h"
#include "xscutimer.h"
#include "xscuwdt.h"



#include "sleep.h"

#include "sys/types.h"


/*LED */ #include "xgpio.h"

//XADC info
#define XPAR_AXI_XADC_0_DEVICE_ID 0


//GPIO info
#define GPIO_DEVICE_ID		XPAR_XGPIOPS_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define GPIO_INTERRUPT_ID	XPS_GPIO_INT_ID

//timer info
#define TIMER_DEVICE_ID		XPAR_XSCUTIMER_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TIMER_IRPT_INTR		XPAR_SCUTIMER_INTR


//define private watchdog
#define WDT_DEVICE_ID		XPAR_SCUWDT_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define WDT_IRPT_INTR		XPAR_SCUWDT_INTR

#define WDT_LOAD_VALUE		0xFFFFFFFF

#define ledpin 47
#define pbsw 51
#define LED_DELAY     100000000
#define TIMER_LOAD_VALUE	0xFF
//void print(char *str);

/*led*/ #define LED_CHANNEL 1
#define GPIO_EXAMPLE_DEVICE_ID  XPAR_AXI_GPIO_0_DEVICE_ID

static XAdcPs  XADCMonInst; //XADC
static XScuGic Intc; //GIC
static XGpioPs Gpio; //GPIO
static XScuTimer Timer;//timer
static XScuWdt WdtInstance; //watchdog

static int toggle = 0;//used to toggle the LED

static void SetupInterruptSystem(XScuGic *GicInstancePtr, XGpioPs *Gpio, u16 GpioIntrId,
		XScuTimer *TimerInstancePtr, u16 TimerIntrId,XScuWdt *WdtInstancePtr, u16 WdtIntrId);

static void GPIOIntrHandler(void *CallBackRef, int Bank, u32 Status);
static void TimerIntrHandler(void *CallBackRef);
static void WdtIntrHandler(void *CallBackRef);


/*led*/  XGpio GpioLEd;

int main()
{

	/* Led */
	 int Status;
	    u32 value           = 0;
	    u32 period          = 0;
	    u32 brightness      = 0;
/*led*/

	    Status = XGpio_Initialize(&GpioLEd, GPIO_EXAMPLE_DEVICE_ID);
	     if (Status != XST_SUCCESS) {
	         return XST_FAILURE;
	     }

	     // Clear the LEDs
	         XGpio_DiscreteWrite(&GpioLEd, LED_CHANNEL, 0);

	XAdcPs_Config *ConfigPtr;           //xadc config

	 XScuTimer_Config *TMRConfigPtr;     //timer config

	 XGpioPs_Config *GPIOConfigPtr;      //gpio config

	 XScuWdt_Config *WCHConfigPtr;		 //watch dog

	 XAdcPs *XADCInstPtr = &XADCMonInst; //xadc pointer



	 //	 u32 Timebase = 0;
	 //	 u32 ExpiredTimeDelta = 0;


	 int reg=0;

	 // XADC varialble declaration


	u32 TempIntRawData;		float TempIntData;	//TempInt : The internal
	u32 VccIntRawData;		float VccIntData;	//VCCInt: The internal PL core voltage
	u32 VccAuxRawData;		float VccAuxData;	//VCCAux: The auxiliary PL voltage
	u32 VccBramRawData;		float VccBramData;	//VCCBram: The PL BRAM voltage
	u32 VccPIntRawData;		float VccPIntData;	//VCCPInt: The PS internal core voltage
	u32 VccPAuxRawData;		float VccPAuxData;	//VCCPAux: The PS auxiliary voltage
	u32 VccDdrRawData;		float VccDdrData;	//VCCDdr: The operating voltage of the DDR RAM connected to the PS



	 init_platform();


     //GPIO Initilization
     GPIOConfigPtr = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_DEVICE_ID);
     XGpioPs_CfgInitialize(&Gpio, GPIOConfigPtr,GPIOConfigPtr->BaseAddr);
     //set direction and enable output
     XGpioPs_SetDirectionPin(&Gpio, ledpin, 1);
     XGpioPs_SetOutputEnablePin(&Gpio, ledpin, 1);
    //set direction input pin
     XGpioPs_SetDirectionPin(&Gpio, pbsw, 0x0);

     //timer initialisation
     TMRConfigPtr = XScuTimer_LookupConfig(TIMER_DEVICE_ID);
     XScuTimer_CfgInitialize(&Timer, TMRConfigPtr,TMRConfigPtr->BaseAddr);

     XScuTimer_SelfTest(&Timer);
	 //load the timer
	 XScuTimer_LoadTimer(&Timer, TIMER_LOAD_VALUE);
	 XScuTimer_EnableAutoReload(&Timer);
	 //start timer
	 XScuTimer_Start(&Timer);

		XScuWdt_LoadWdt(&WdtInstance, WDT_LOAD_VALUE);


	    //set up the interrupts
//    SetupInterruptSystem(&Intc, &Gpio, GPIO_INTERRUPT_ID,&Timer,TIMER_IRPT_INTR,&WdtInstance,WDT_IRPT_INTR);


	 // watchdog
	WCHConfigPtr = XScuWdt_LookupConfig(WDT_DEVICE_ID);
	int wd =  XScuWdt_CfgInitialize(&WdtInstance, WCHConfigPtr, WCHConfigPtr->BaseAddr);

	// XADC
	ConfigPtr = XAdcPs_LookupConfig(XPAR_AXI_XADC_0_DEVICE_ID);
	int xadc = XAdcPs_CfgInitialize(&XADCMonInst,ConfigPtr,ConfigPtr->BaseAddress);


	int xadc_test = XAdcPs_SelfTest(&XADCMonInst); // ensure that no problem with device

	u8 get_seq= XAdcPs_GetSequencerMode(&XADCMonInst); // get current Sequence mode

	XAdcPs_SetSequencerMode(&XADCMonInst, XADCPS_SEQ_MODE_SINGCHAN); // stop sequencer mode and set it for single channel

	u8 get_seq_after= XAdcPs_GetSequencerMode(&XADCMonInst); // sequencer mode after setting to 1 channel

	u32 get_alarm = XAdcPs_GetAlarmEnables(&XADCMonInst); //get Alarm enable status before disable

	XAdcPs_SetAlarmEnables(&XADCMonInst,0x0);  //disable all alarms

	u32 get_alam_after = XAdcPs_GetAlarmEnables(&XADCMonInst); // get Alarm enable after disable


	 XAdcPs_SetSequencerMode(&XADCMonInst, XADCPS_SEQ_MODE_SAFE); // enable mode safe of sequencer


	 u32 getseq = XAdcPs_GetSeqInputMode(&XADCMonInst); // get sequencer mode status


	 int setseqmode = XAdcPs_SetSeqInputMode(&XADCMonInst, 0x0800 ); // to restart the sequencer


	 u32 getseq_after = XAdcPs_GetSeqInputMode(&XADCMonInst);

	int set_seq_ch_reslt= XAdcPs_SetSeqChEnables(&XADCMonInst,
			XADCPS_CH_TEMP|
			XADCPS_CH_VCCINT|
			XADCPS_CH_VCCAUX|
			XADCPS_CH_VBRAM|
			XADCPS_CH_VCCPINT|
     		XADCPS_CH_VCCPAUX|
     		XADCPS_CH_VCCPDRO );

	//XAdcPs_SetAlarmEnables(&XADCMonInst, XADCPS_CFR1_ALM_TEMP_MASK);

	//XAdcPs_IntrEnable(&XADCMonInst,XADCPS_INTX_ALM0_MASK);

	//XAdcPs_SetAlarmThreshold(&XADCMonInst, XADCPS_ATR_TEMP_UPPER_OFFSET, (60000) );

	// u16 AlarmThershold = XAdcPs_GetAlarmThreshold(&XADCMonInst, XADCPS_ATR_TEMP_UPPER_OFFSET);

	//u16 MaxRawTemp = XAdcPs_GetMinMaxMeasurement(&XADCMonInst, XADCPS_MAX_TEMP_OFFSET);
	//float MaxTemp = XAdcPs_RawToTemperature(MaxRawTemp);

//	u16 MinTemp = XAdcPs_GetMinMaxMeasurement(&XADCMonInst, XADCPS_MIN_TEMP_OFFSET);

	//-------------------------------  Affichage ------------------------------------------//

	printf("\n\r Mohamed Amine Edition Zedboard Using Vivado and SDK ( 2013.4) \n\r");
		reg = XScuWdt_IsWdtExpired(&WdtInstance);


	 printf("\n\n 1- Previous reset state = %d\n\r", reg);

	 printf (" \n\n\n\n 1- get current sequence mode  %d\n", (int) get_seq);
	 printf (" \n 1- get sequence mode after   %d\n\n\n", (int) get_seq_after);

	 printf (" \n get alarm status current %d \n\ ",(int)get_alarm);
	 printf (" \n get alarm status after %d\n ",(int)get_alam_after);


	printf ( "\n 2- watch dog initiliaed status %d \n ", wd );

	printf (" \n 3- the number of XADC bloc in our system is  %d \n", xadc+1);

	printf ("\n 5- the result of XADC device test is %d \n ", xadc_test);


	printf ( "\n 4- get current sequencer mode status %d \n", (int)getseq);

	printf ( "\n 6- result of set sequencer input mode is %d\n ", setseqmode);

	printf (" \n 7- get sequencer mode new status %d \n\n ", (int) getseq_after);

	printf( " \n 8- result sequence ch enable %d \n", set_seq_ch_reslt);

	//printf ( "\n Alarm Thershold status %d \n ",(int) AlarmThershold);


	//printf ("\n\n \n\n\n ");
/*
int a ;
printf ( " enter 1 \n");

 scanf(" %d\n", &a);
 printf ( " vous avez entrer  \n %d", a);

if ( a == 1) {
*/
	/*
    while (1) {
      // print(" Select a Brightness between 0 and 9 : ");
       value = 1;
       period = value - 0x30;
       xil_printf("Brightness Level %d selected ", period);

       // Since the LED width is 1e6 clk cycles, we need to normalize
       // the period to that clk.  Since we accept values 0-9, that will
       // scale period from 0-999,000.  0 turns off LEDs, 999,000 is full
       // brightness

       brightness = period * 110000;
       // Write the duty_cycle width (Period) out to the PL GPIO peripheral

       XGpio_DiscreteWrite(&GpioLEd, LED_CHANNEL, brightness);

    }*/

   // return XST_SUCCESS;
//}

//else if ( a ==2)


//{
    XGpio_DiscreteWrite(&GpioLEd, LED_CHANNEL, 0);
	while(1)

	{
		 printf("\n********* Begin detection session ********* \n");
		 //XGpio_DiscreteWrite(&GpioLEd, LED_CHANNEL, 0);
		 TempIntRawData = XAdcPs_GetAdcData(XADCInstPtr, XADCPS_CH_TEMP);
		 TempIntData = XAdcPs_RawToTemperature(TempIntRawData);
		 printf ("\nRaw temp %lu Real Tempin %.2f �C\n", TempIntRawData,TempIntData);
/*
    	VccIntRawData = XAdcPs_GetAdcData(XADCInstPtr, XADCPS_CH_VCCINT);
    	VccIntData = XAdcPs_RawToVoltage(VccIntRawData);
    	 printf ("\nRaw Vcc %lu Real Vccin %.2f V\n", VccIntRawData, VccIntData);


         VccAuxRawData = XAdcPs_GetAdcData(XADCInstPtr, XADCPS_CH_VCCAUX);
          VccAuxData = XAdcPs_RawToVoltage(VccAuxRawData);
         printf("Raw VccAux %lu Real VccAux %.2f V \n\r", VccAuxRawData, VccAuxData);


         VccBramRawData = XAdcPs_GetAdcData(XADCInstPtr, XADCPS_CH_VBRAM);
          VccBramData = XAdcPs_RawToVoltage(VccBramRawData);
         printf("Raw VccBram %lu Real VccBram %.2f V\n\r", VccBramRawData, VccBramData);

         VccPIntRawData = XAdcPs_GetAdcData(XADCInstPtr, XADCPS_CH_VCCPINT);
          VccPIntData = XAdcPs_RawToVoltage(VccPIntRawData);
         printf("Raw VccPInt %lu Real VccPInt %.2f V\n\r", VccPIntRawData, VccPIntData);

         VccPAuxRawData = XAdcPs_GetAdcData(XADCInstPtr, XADCPS_CH_VCCPAUX);
           VccPAuxData = XAdcPs_RawToVoltage(VccPAuxRawData);
         printf("Raw VccPAux %lu Real VccPAux %.2f V\n\r", VccPAuxRawData, VccPAuxData);

         VccDdrRawData = XAdcPs_GetAdcData(XADCInstPtr, XADCPS_CH_VCCPDRO);
         VccDdrData = XAdcPs_RawToVoltage(VccDdrRawData);
         printf("Raw VccDDR %lu Real VccDDR %.2f V\n\r", VccDdrRawData, VccDdrData);

*/


         printf("\n********* End detection session ********* \n");

         if  (TempIntData > 51) {
            // print(" Select a Brightness between 0 and 9 : ");
             value = 1;
             period = value - 0x30;
           //  xil_printf("Brightness Level %d selected ", period);

             // Since the LED width is 1e6 clk cycles, we need to normalize
             // the period to that clk.  Since we accept values 0-9, that will
             // scale period from 0-999,000.  0 turns off LEDs, 999,000 is full
             // brightness

             brightness = period * 110000;
             // Write the duty_cycle width (Period) out to the PL GPIO peripheral

             XGpio_DiscreteWrite(&GpioLEd, LED_CHANNEL, brightness);

          }
    	 sleep (1);
    	XGpio_DiscreteWrite(&GpioLEd, LED_CHANNEL, 0);
    }


}

//}
//}

/*
static void SetupInterruptSystem(XScuGic *GicInstancePtr, XGpioPs *Gpio, u16 GpioIntrId,
		XScuTimer *TimerInstancePtr, u16 TimerIntrId, XScuWdt *WdtInstancePtr, u16 WdtIntrId)
{


		XScuGic_Config *IntcConfig; //GIC config
		Xil_ExceptionInit();
		//initialise the GIC
		IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
		XScuGic_CfgInitialize(GicInstancePtr, IntcConfig,
						IntcConfig->CpuBaseAddress);
	    //connect to the hardware
		Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
					(Xil_ExceptionHandler)XScuGic_InterruptHandler,
					GicInstancePtr);
	    //set up the GPIO interrupt
		XScuGic_Connect(GicInstancePtr, GpioIntrId,
					(Xil_ExceptionHandler)XGpioPs_IntrHandler,
					(void *)Gpio);
		//set up the timer interrupt
		XScuGic_Connect(GicInstancePtr, TimerIntrId,
						(Xil_ExceptionHandler)TimerIntrHandler,
						(void *)TimerInstancePtr);
		//set up the watchdog
		XScuGic_Connect(GicInstancePtr, WdtIntrId,
						(Xil_ExceptionHandler)WdtIntrHandler,
						(void *)WdtInstancePtr);

		//setup the watchdog
		XScuWdt_SetWdMode(WdtInstancePtr);

		//Enable  interrupts for all the pins in bank 0.
		XGpioPs_SetIntrTypePin(Gpio, pbsw, XGPIOPS_IRQ_TYPE_EDGE_RISING);
		//Set the handler for gpio interrupts.
		XGpioPs_SetCallbackHandler(Gpio, (void *)Gpio, GPIOIntrHandler);
		//Enable the GPIO interrupts of Bank 0.
		XGpioPs_IntrEnablePin(Gpio, pbsw);
		//Enable the interrupt for the GPIO device.
		XScuGic_Enable(GicInstancePtr, GpioIntrId);
		//enable the interrupt for the Timer at GIC
		XScuGic_Enable(GicInstancePtr, TimerIntrId);
		//enable interrupt on the timer
		XScuTimer_EnableInterrupt(TimerInstancePtr);
		//enable interrupt on the watchdog
		XScuGic_Enable(GicInstancePtr, WdtIntrId);
		// Enable interrupts in the Processor.
		Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
	}

static void TimerIntrHandler(void *CallBackRef)
{

	XScuTimer *TimerInstancePtr = (XScuTimer *) CallBackRef;
	XScuTimer_ClearInterruptStatus(TimerInstancePtr);
	//printf("****Timer Event!!!!!!!!!!!!!****\n\r");
	XScuWdt_Start(&WdtInstance);

}


static void GPIOIntrHandler(void *CallBackRef, int Bank, u32 Status)
{
	int delay;
	XGpioPs *Gpioint = (XGpioPs *)CallBackRef;

	//printf("****button pressed****\n\r");

	if (toggle == 0 )
		toggle = 1;
	else
		toggle = 0;

	//load timer
	XScuTimer_LoadTimer(&Timer, TIMER_LOAD_VALUE);
	//start timer
	XScuTimer_Start(&Timer);
	XGpioPs_WritePin(Gpioint, ledpin, toggle);

	for( delay = 0; delay < LED_DELAY; delay++)//wait
	{}
	XGpioPs_IntrClearPin(Gpioint, pbsw);
}
static void WdtIntrHandler(void *CallBackRef)
{}
*/
