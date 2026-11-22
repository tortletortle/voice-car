#ifndef __BSP_CAMERA_H_
#define __BSP_CAMERA_H_



#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "IOI2C.h"


#define VoicceADDR    0x2b
#define Write_register 0x03
#define Read_register 0x64
 
//²¥±¨´Ê Active broadcast content
#define This_red 0x5F    
#define This_blue 0x60
#define This_green 0x61
#define This_yellow 0x62
#define Recognize_yellow 0x63
#define Recognize_green 0x64
#define Recognize_blue 0x65
#define Recognize_red 0x66
#define init 0x67     


void set_voice(uint8_t data);
void read_camera_data(void);

#endif
