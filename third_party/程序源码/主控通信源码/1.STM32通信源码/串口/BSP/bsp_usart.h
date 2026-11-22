#ifndef __BSP_USART_H
#define __BSP_USART_H


#include <stdio.h>
#include <string.h>
#include "stm32f10x.h"

#define This_red 0x5F    
#define This_blue 0x60
#define This_green 0x61
#define This_yellow 0x62
#define Recognize_yellow 0x63
#define Recognize_green 0x64
#define Recognize_blue 0x65
#define Recognize_red 0x66
#define init 0x67    


void USART1_init(u32 baudrate);
void USART1_Send_ArrayU8(uint8_t *BufferPtr);
void USART1_Send_U8(uint8_t ch);
void Processing_Data(uint8_t RXdata);
void Write_Data(uint8_t dat);

#endif
