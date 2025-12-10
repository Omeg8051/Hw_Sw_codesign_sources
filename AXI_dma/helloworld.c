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
#include "xil_printf.h"
#include "xparameters.h"
#include "xaxidma.h"

XAxiDma dma;
XAxiDma_Config *dma_cfg;

const char dma_out[64] = "Lorem ipsum dolor sit amet consectetur adipiscing elit quisque faucibus ex sapien vitae pellentesque sem placerat in id cursus mi pretium tellus duis convallis tempus leo eu aenean sed diam urna tempor pulvinar vivamus fringilla lacus nec metus bibendum egestas iaculis massa nisl malesuada lacinia integer nunc posuere ut hendrerit semper vel class aptent taciti sociosqu ad litora torquent per conubia nostra inceptos himenaeos orci varius natoque penatibus et magnis dis parturient montes nascetur ridiculus.";

char dma_in[64];

int main()
{   
    dma_cfg = XAxiDma_LookupConfig(XPAR_AXI_DMA_0_BASEADDR);
    XAxiDma_CfgInitialize(&dma,dma_cfg);

    Xil_DCacheFlushRange(dma_out, 64);
	Xil_DCacheFlushRange(dma_in, 64);

    XAxiDma_SimpleTransfer(&dma,dma_out,64,XAXIDMA_DMA_TO_DEVICE);
    print("Writing to DMA buffer\n\r");
    print("Successfully ran Hello World application\r\n");

    while(XAxiDma_Busy(&dma,XAXIDMA_DMA_TO_DEVICE)){
        print(".");
        usleep(1000000);
    }
    print("\r\nReading from DMA buffer\r\n");
    XAxiDma_SimpleTransfer(&dma,dma_in,64,XAXIDMA_DEVICE_TO_DMA);

    while(XAxiDma_Busy(&dma,XAXIDMA_DEVICE_TO_DMA)){
        print(".");
        usleep(1000000);
    }

    print(".\r\n");
    int same = 1;
    for (size_t i = 0; i < 64; i++)
    {
        if(dma_out[i] != dma_in[i]){
            same = 0;
            break;
        }
    }
    if(same){
        print("DMA input match output\r\n");
    } else {

        print("DMA input DOES NOT match output\r\n");
    }
    
    return 0;
}
