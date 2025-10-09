#include <stdio.h>
#include "xparameters.h"
#include "xil_exception.h"
#include "xscugic_hw.h"
#include "xil_printf.h"
#include "xstatus.h"
#include <xgpio_l.h>
#include "xgpio.h"
#include "xscugic.h"
#include "xil_util.h"
#include "xtmrctr.h"
#if defined (VERSAL_NET)
#include "xplatform_info.h"
#endif

/************************** Constant Definitions *****************************/

/*
* The following constants map to the XPAR parameters created in the
* xparameters.h file. They are defined here such that a user can easily
* change all the needed parameters in one place.
*/

#define TMR_DEVICE_ID		XPAR_XTMRCTR_0_BASEADDR
#define TMR_INT_TICK_HZ			100
#define MAIN_LOOP_HOLD_TICK_NUM	100

#ifndef SDT
#define XSCUGIC_CPU_BASEADDR		XPAR_SCUGIC_0_CPU_BASEADDR
#define XSCUGIC_DIST_BASEADDR		XPAR_SCUGIC_0_DIST_BASEADDR
#else
#define XSCUGIC_DIST_BASEADDR		XPAR_XSCUGIC_0_BASEADDR
#endif

#define GIC_DEVICE_INT_MASK        0x02010003 /* Bit [25:24] Target list filter
												Bit [23:16] 16 = Target CPU iface 0
												Bit [3:0] identifies the SFI */
#define XSCUGIC_SW_TIMEOUT_VAL	10000000U /* Wait for 10 sec */
static void (*my_example_interrupt_handler_table[92])(void* arg_ptr);
static void *my_example_interrupt_handler_arges[92];

void my_example_register_interrupt_handler(int int_id, void (*handler)(void *),void* handler_arg){
	my_example_interrupt_handler_table[int_id] = handler;
	my_example_interrupt_handler_arges[int_id] = handler_arg;

}

int Gic_init(UINTPTR base_addr);
int Gic_example();
void my_primary_int_handle(u32 CallbackRef);
void Gpio_int_hndl(void* arg);
void TMR_Intr_Handler(void *data);

int InterruptProcessed = 0;
static XScuGic_Config *CfgPtr;
static XGpio io_0, io_1;
static XTmrCtr TMRInst;
static unsigned long sys_tick = 0;

int main(){

	/**
	 * init GPIO
	 */
	XGpio_Initialize(&io_0, XPAR_AXI_GPIO_0_BASEADDR);	//initialize input XGpio variable
    XGpio_Initialize(&io_1, XPAR_AXI_GPIO_1_BASEADDR);	//initialize output XGpio variable

    XGpio_SetDataDirection(&io_0, 1, 0xF);			//set first channel tristate buffer to input
    XGpio_SetDataDirection(&io_0, 2, 0xF);			//set second channel tristate buffer to input

    XGpio_SetDataDirection(&io_1, 1, 0x0);		//set first channel tristate buffer to output

	/**
	 * enable GPIO_0 channel 1 interrupt
	 */
	XGpio_InterruptEnable(&io_0,XGPIO_IR_CH1_MASK);
    XGpio_InterruptGlobalEnable(&io_0);
    

	/**
	 * timer start
	 */
	int stat = XTmrCtr_Initialize(&TMRInst, TMR_DEVICE_ID);
	if(stat != XST_SUCCESS) return XST_FAILURE;

	XTmrCtr_SetHandler(&TMRInst, TMR_Intr_Handler, &TMRInst);
	//calculate 1k zh systick
	XTmrCtr_SetResetValue(&TMRInst, 0, 0xFFFFFFFFUL - (TMRInst.Config.SysClockFreqHz / TMR_INT_TICK_HZ));
	XTmrCtr_SetOptions(&TMRInst, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);

	/**
	 * enable timer interrupt
	 */
	XTmrCtr_EnableIntr(&TMRInst,0);
	
	print("my GCI test\r\n");
	Gic_init(XSCUGIC_DIST_BASEADDR);

	my_example_register_interrupt_handler(61,Gpio_int_hndl,&io_0);

	my_example_register_interrupt_handler(62,TMR_Intr_Handler,&TMRInst);

	XScuGic_EnableIntr(XSCUGIC_DIST_BASEADDR,61);
	XScuGic_EnableIntr(XSCUGIC_DIST_BASEADDR,62);
	XTmrCtr_Start(&TMRInst,0);

	unsigned long l_tick = 0;
    while(1){
		/**
		 * do idle stuff here
		 */
		if(sys_tick - l_tick >= MAIN_LOOP_HOLD_TICK_NUM){
			l_tick = sys_tick;
			xil_printf("it had been %d systick since start up\r\nBased on %d Hz sys tick\r\n",sys_tick,TMR_INT_TICK_HZ);
		}
	}

	return 0;
}

#define DEFAULT_PRIORITY    0xa0a0a0a0UL
#define DEFAULT_TARGET    0x01010101UL
int Gic_init(UINTPTR base_addr){
	uint32_t reg_idx;

	/**
	 * Gic Dist init
	 */
	XScuGic_WriteReg(base_addr,XSCUGIC_DIST_EN_OFFSET,0);
	for (reg_idx = 32; reg_idx < XSCUGIC_MAX_NUM_INTR_INPUTS; reg_idx += 16) {
		/*
		* Each INT_ID uses two bits, or 16 INT_ID per register
		* Set them all to be level sensitive, active HIGH.
		*/
		XScuGic_WriteReg(base_addr,
				XSCUGIC_INT_CFG_OFFSET + (reg_idx * 4) / 16, 0UL);
	}
	for (reg_idx = 0; reg_idx < XSCUGIC_MAX_NUM_INTR_INPUTS; reg_idx += 4) {
		/*
		 * 2. The priority using int the priority_level register
		 * The priority_level and spi_target registers use one byte
		 * per INT_ID.
		 * Write a default value that can be changed elsewhere.
		 */
		XScuGic_WriteReg(base_addr,
				XSCUGIC_PRIORITY_OFFSET + (reg_idx * 4) / 4, DEFAULT_PRIORITY);
	}
	for (reg_idx = 32; reg_idx < XSCUGIC_MAX_NUM_INTR_INPUTS; reg_idx += 4) {
		/*
		 * 3. The CPU interface in the spi_target register
		 */
		XScuGic_WriteReg(base_addr,
				XSCUGIC_SPI_TARGET_OFFSET + (reg_idx * 4) / 4, DEFAULT_TARGET);
	}
	for (reg_idx = 0; reg_idx < XSCUGIC_MAX_NUM_INTR_INPUTS; reg_idx += 32) {
		/*
		 * 4. Enable the SPI using the enable_set register.
		 * Leave all disabled for now.
		 */
		XScuGic_WriteReg(base_addr,
				XSCUGIC_DISABLE_OFFSET + (reg_idx * 4) / 4, 0xFFFFFFFFUL);
	}
	XScuGic_WriteReg(base_addr, XSCUGIC_DIST_EN_OFFSET, 0x01UL);

	CfgPtr = XScuGic_LookupConfig(XSCUGIC_DIST_BASEADDR);

	
	/**
	 * GIC CPU INIT
	 */
	XScuGic_WriteReg(CfgPtr->CpuBaseAddress, XSCUGIC_CPU_PRIOR_OFFSET, 0xF0);
	XScuGic_WriteReg(CfgPtr->CpuBaseAddress, XSCUGIC_CONTROL_OFFSET, 0x01);

	/**
	 * register primary interrupt
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT, (Xil_ExceptionHandler) my_primary_int_handle, (void *)CfgPtr->CpuBaseAddress);
	/*
	* Enable interrupts in the ARM
	*/
	Xil_ExceptionEnable();
	return 0;
}
void Gpio_int_hndl(void* arg)
{
XGpio *base_addr;
	base_addr = (XGpio *)arg;

	xil_printf("GPIO_Int\r\n");
/*
	* Write to the EOI register, we are all done here.
	* Let this function return, the boot code will restore the stack.
	*/
	XGpio_InterruptClear(base_addr,(uint32_t)-1);
}
void my_primary_int_handle(u32 CallbackRef)
{
u32 BaseAddress;
	BaseAddress = CallbackRef;
	u32 IntID;

	/*
	* Read the int_ack register to identify the interrupt and
	* make sure it is valid.
	*/
	IntID = XScuGic_ReadReg(BaseAddress, XSCUGIC_INT_ACK_OFFSET) & XSCUGIC_ACK_INTID_MASK;

	if (XSCUGIC_MAX_NUM_INTR_INPUTS < IntID) {
		return;
	}

	/**
	 * call handler by table
	 */
	my_example_interrupt_handler_table[IntID](my_example_interrupt_handler_arges[IntID]);
	/*
	* Write to the EOI register, we are all done here.
	* Let this function return, the boot code will restore the stack.
	*/
	
	XScuGic_WriteReg(BaseAddress, XSCUGIC_EOI_OFFSET, IntID);
}

void TMR_Intr_Handler(void *arg)
{
	XTmrCtr *base_addr = (XTmrCtr *)arg;
	if (XTmrCtr_IsExpired(base_addr,0)){
		// Once timer has expired 3 times, stop, increment counter
		// reset timer and start running again
		sys_tick ++;
		XTmrCtr_Reset(base_addr,0);
		XTmrCtr_Start(base_addr,0);
		
	}
}