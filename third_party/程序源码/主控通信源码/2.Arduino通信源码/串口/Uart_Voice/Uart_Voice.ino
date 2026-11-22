#include <stdio.h>
#include "bsp_uart.hpp"


extern uint8_t new11;
void setup()
{
  serial_init();//串口初始化 Serial port initialization
  Write_Data(init);
}
void loop() 
{
}

