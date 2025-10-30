#include "arduino_style_gpio.h"

unsigned int CustGpio_read(Cust_gpio *gpio){
    return Xil_In32(gpio + GPIO_DATA_IN_OFFSET);
}
void CustGpio_set_data_dir(Cust_gpio *gpio, unsigned int data_dir){
    Xil_Out32(gpio + GPIO_DATA_DIR_OFFSET,data_dir);
}
void CustGpio_set_data(Cust_gpio *gpio, unsigned int data){
    //DATA_OUT_OFFSET = 0; done't waist instructions
    Xil_Out32(gpio,data);
}