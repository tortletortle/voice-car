#include "bsp_iic.h"
 
uint8_t buf[2];

void set_voice(uint8_t data)
{
	i2cWrite(VoicceADDR,Write_register,1,&data);
}



void read_camera_data(void)
{
	i2cRead(VoicceADDR, Read_register,1,&buf[0]);
//	delay_ms(10);
	printf("Read_ID: %d\r\n",buf[0]);
	
}

