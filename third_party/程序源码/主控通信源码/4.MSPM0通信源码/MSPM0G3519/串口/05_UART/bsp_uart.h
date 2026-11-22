#ifndef __BSP_UART_H
#define __BSP_UART_H


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"

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


void Processing_Data(uint8_t RXdata);
void Write_Data(uint8_t dat);

#endif

