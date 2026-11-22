#include "bsp_uart.hpp"
#include <stdio.h>
#include <SoftwareSerial.h>

uint8_t new11 = 0;
byte  cmd[5];



void serial_init(void)
{
  ESPWIFISerial.begin(115200);

}


// 串口中断函数
void serialEvent() {
  // 当串口接收到数据时，自动调用此函数
  uint8_t Rx1_Temp = 0;
  while (Serial.available()) {
    // 读取接收到的字节
    Rx1_Temp = Serial.read();

    Processing_Data(Rx1_Temp);
  }
}

uint8_t recv_buf[5]={0}; //接收buf Receive buf
uint8_t g_index = 0;


void Processing_Data(uint8_t RXdata)
{
	recv_buf[g_index++] =RXdata;
	
	if (g_index >=5)
		{
			g_index = 0;
			Serial.println(recv_buf[3]);
		}

}


void Write_Data(byte dat)
{

	cmd[0] = 0xAA;
	cmd[1] = 0x55;
	cmd[2] = 0xFF;
	cmd[3] = dat;  // 直接使用传入的数据
	cmd[4] = 0xFB;

	for(int j = 0; j<5 ;j++)
	{
	  ESPWIFISerial.write(cmd[j]);
	}
   
}




