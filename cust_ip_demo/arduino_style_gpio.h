#ifndef ARDUINO_STYLE_GPIO_H
#define ARDUINO_STYLE_GPIO_H

#include "xil_io.h"

#define GPIO_DATA_OUT_OFFSET 0
#define GPIO_DATA_DIR_OFFSET 4
#define GPIO_DATA_IN_OFFSET 8

typedef unsigned long Cust_gpio;

unsigned int CustGpio_read(Cust_gpio *gpio);
void CustGpio_set_data_dir(Cust_gpio *gpio, unsigned int data_dir);
void CustGpio_set_data(Cust_gpio *gpio, unsigned int data);

#endif /* ARDUINO_STYLE_GPIO_H */
