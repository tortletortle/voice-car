#include "bsp_uart.h"

extern void uart0_send_char(char str);


uint8_t recv_buf[5]={0}; //接收buf Receive buf
char cmd[5];
uint8_t g_index = 0;

void Processing_Data(uint8_t RXdata)
{
	recv_buf[g_index++] =RXdata;
	
	if (g_index >=5)
		{
			g_index = 0;
			uart0_send_char(recv_buf[3]);
		}

}


void Write_Data(uint8_t dat)
{

	cmd[0] = 0xAA;
	cmd[1] = 0x55;
	cmd[2] = 0xFF;
	cmd[3] = dat;  // 直接使用传入的数据
	cmd[4] = 0xFB;

	for(int j = 0; j<5 ;j++)
	{
		uart0_send_char(cmd[j]);
	}
   

}


