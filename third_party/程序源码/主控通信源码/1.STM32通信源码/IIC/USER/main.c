#include "AllHeader.h"


int main(void)
{	
	bsp_init();
	
	TIM3_Init();
	
	IIC_Init();
	
	Broadcast(init); //ÉèÖÃ²¥±¨ÄÚÈİ Set the content of the announcement

	printf("pelase wait !!!\r\n");
	
	while(1)
	{

		read_voice_data();

	}
	
}



