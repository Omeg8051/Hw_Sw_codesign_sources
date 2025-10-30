/******************************************************************************
* Copyright (C) 2023 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/
/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "arduino_style_gpio.h"
#include "sleep.h"

static Cust_gpio *gpio;

int main()
{
    int i = 0x5;
    gpio = XPAR_MY_AXI_GPIO_0_BASEADDR;
    init_platform();

    print("Hello World\n\r");
    print("Successfully ran Hello World application");
    CustGpio_set_data_dir(gpio, 0xF);

    while(1){
        CustGpio_set_data(gpio,i);
        i = ~i;
        sleep(1);
    }
    cleanup_platform();
    return 0;
}
