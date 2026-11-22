#include "board.h"
#include "bsp_iic.h"


int main(void)
{
    //开发板初始化 Development board initialization
    board_init();
	
	  SYSCFG_DL_init(); 
	
	  set_voice(init); 
		delay_ms(500); 
    printf("Initialization Data Succeed \r\n");
	
    while(1) 
    {
			read_camera_data();
			delay_ms(500);
    }

}