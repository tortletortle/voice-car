#include "stm32f10x.h"
#include "delay.h"
#include "bsp_usart.h"

extern uint8_t data_buff[5]; //备份buf Backup buf

int main()
{
	
	SystemInit();
	
	delay_init();
	
	USART1_init(115200);//接PC的串口PA9(TX)  PA10(RX) Connect to PC's serial port PA9 (TX) PA10 (RX)
	
	
	printf("waiting  wifi start!\r\n");
	

  delay_ms(100);
	
	Write_Data(init);
	
	while(1)
	{

	}
	
}

