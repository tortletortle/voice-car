#include "bsp_iic.h"
 
#define delaytime 2 //根据自己的需求调整时间

uint8_t buf[2];


void IIC_Init(void)
{			
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); //使能PB端口时钟
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11;	//端口配置
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;      //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;     //50M
  GPIO_Init(GPIOB, &GPIO_InitStructure);					      //根据设定参数初始化GPIOB 
}


//设置水平翻转 Set horizontal flip
void Broadcast(uint8_t data)
{
	i2cWrite(VoicceADDR,Write_register,1,&data);
}



void read_voice_data(void)
{
	//每个数据读两次，确保准确性
	i2cRead(VoicceADDR, Read_register,1,&buf[0]);
	delay_time(delaytime);
	printf("Read_id: %d\r\n",buf[0]);
	
}


