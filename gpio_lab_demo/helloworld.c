/******************************************************************************
* Copyright (C) 2023 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/
/**
 * Those who successfully finishing the lab experiment gets to play the game.
 * 
 * 
 * 
 * This is a mini game implementation
 * 
 * The round count can be configured by "switch[1:0] * 4" before the game start
 * 
 * The game start by holding one of the button until
 * the  green LED begin to count down to 0
 * 
 * Press the button prompted by the LEDs each round.
 * 1 button on the first round.
 * 2 buttons on the second round.
 * etc...
 * 
 * Button pressed will increment by one every round.
 * 
 * Finishing all the round will be indicated by a uniform flash of all LEDs.
 * 
 * Losing the game will be indicated by the erratic flashing of the LEDs.
 * 
 */

#include <stdio.h>
#include <sys/_intsup.h>
#include <xgpio_l.h>
#include "xgpio.h"
#include "platform.h"
#include "xparameters.h"
#include "xil_printf.h"
//change the following path to yout platform package
#include "/home/omeg/Desktop/proj/Vitis/pynq/zynq_fsbl/zynq_fsbl_bsp/libsrc/standalone/src/arm/cortexr5/xtime_l.h"
#include "sleep.h"

unsigned int debounced_read(XGpio *in,int channel){
    unsigned int i = XGpio_DiscreteRead(in, channel);
    usleep(10000);
    return i;
}

unsigned int leftRotate(unsigned int n, unsigned int d)
{
    /* In n<<d, last d bits are 0. To put first 3 bits of n
       at last, do bitwise or of n<<d with n >>(INT_BITS -
       d) */
    return (n << d) | (n >> (32 - d));
}

unsigned int pseudo_random_from_sys_time(unsigned int top){
    static XTime l_t, c_t;

    XTime_GetTime(&c_t);
    l_t += c_t;
    return leftRotate(l_t, c_t + top) % top;

}

void display_seq_fast(XGpio *out,unsigned int seq){
    XGpio_DiscreteWrite(out,1,seq);
    usleep(70000);
    XGpio_DiscreteWrite(out,1,0);
    usleep(100000);
}

void display_button_seq(XGpio *out,unsigned int seq,char tail_delay){
    XGpio_DiscreteWrite(out,1,seq);
    usleep(500000);
    XGpio_DiscreteWrite(out,1,0);
    if(tail_delay)usleep(1000000);
}

int querry_user_input(XGpio *out,XGpio *in,const unsigned char correct){
    unsigned char user;
    do{
        user = debounced_read(in,1);
    }while(!user);
    display_button_seq(out,user,0);
    while(debounced_read(in,1))usleep(10000);
    return user != correct;
}

int question_cycle(XGpio *out,XGpio *in,const unsigned char *seq,const unsigned char round){
    int user_bad = 0;
    //display question
    for(unsigned char cr = 0;cr < round + 1;cr++){
        display_button_seq(out,seq[cr],cr != round);
    }
    //query input;
    for(unsigned char cr = 0;(cr < round + 1) && !user_bad;cr++){
       user_bad |= querry_user_input(out,in,seq[cr]);
    }
    display_button_seq(out,0,0);
    return user_bad;
}

void punsih_player(XGpio *out){
    for(char i = 10;i;i--){
        display_seq_fast(out,pseudo_random_from_sys_time(15) + 1);
    }
}
void reward_player(XGpio *out){
    for(char i = 5;i;i--){
        display_seq_fast(out,0xF);
    }
}

int main()
{
    XGpio io_0, io_1;
    unsigned char game_round;
    unsigned char user_bad;
    unsigned char top_round;
    unsigned char press_seq[120];
    
    //init GPIO IP core
    XGpio_Initialize(&io_0, XPAR_AXI_GPIO_0_BASEADDR);	//initialize input XGpio variable
    XGpio_Initialize(&io_1, XPAR_AXI_GPIO_1_BASEADDR);	//initialize output XGpio variable

    XGpio_SetDataDirection(&io_0, 1, 0xF);			//set first channel tristate buffer to input
    XGpio_SetDataDirection(&io_0, 2, 0xF);			//set second channel tristate buffer to input

    XGpio_SetDataDirection(&io_1, 1, 0x0);		//set first channel tristate buffer to output
    
    init_platform();
    
    while(1){
        //main logic
        //init game
        user_bad = 0;
        //query start game?
        while (!debounced_read(&io_0,1))
        {
            display_button_seq(&io_1,0xF,1);
        }
        //calculate game round to be sw[1:0] * 4
        top_round = (debounced_read(&io_0,2) + 1) << 2;
        //seed pseudo random.
        pseudo_random_from_sys_time(4);
        //game start count down
        for(game_round = 0xF;game_round;game_round>>= 1 ){
            display_button_seq(&io_1,game_round,1);
        }
        //main game loop;
        for(game_round = 0;(game_round < top_round) && !user_bad;game_round++){
            //gen a seq
            press_seq[game_round] = 1 << pseudo_random_from_sys_time(4);
            
            user_bad |= question_cycle(&io_1,&io_0,press_seq,game_round);

            

        }

        if(user_bad){
            punsih_player(&io_1);
        } else {
            reward_player(&io_1);
        }

    }
    
    cleanup_platform();
    return 0;
}
