#ifndef __IOI2C_H
#define __IOI2C_H

#include "board.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

//IO方向设置
// Set SDA output mode
#define SDA_OUT()   {                                                  \
                        DL_GPIO_initDigitalOutput(I2C_SDA_IOMUX);     \
                        DL_GPIO_setPins(I2C_PORT, I2C_SDA_PIN);      \
                        DL_GPIO_enableOutput(I2C_PORT, I2C_SDA_PIN); \
                    }
// Set SDA input mode
#define SDA_IN()    { DL_GPIO_initDigitalInput(I2C_SDA_IOMUX); }

//Get the level change of the SDA pin
#define SDA_GET()   ( ( ( DL_GPIO_readPins(I2C_PORT,I2C_SDA_PIN) & I2C_SDA_PIN ) > 0 ) ? 1 : 0 )
//SDA and SCL output
#define SDA(x)      ( (x) ? (DL_GPIO_setPins(I2C_PORT,I2C_SDA_PIN)) : (DL_GPIO_clearPins(I2C_PORT,I2C_SDA_PIN)) )
#define SCL(x)      ( (x) ? (DL_GPIO_setPins(I2C_PORT,I2C_SCL_PIN)) : (DL_GPIO_clearPins(I2C_PORT,I2C_SCL_PIN)) )



//IIC所有操作函数

int IIC_Start(void);
void IIC_Stop(void);
unsigned char I2C_WaitAck(void);
void IIC_Send_Ack(unsigned char ack);
void Send_Byte(uint8_t dat);
unsigned char Read_Byte(void);

int i2cWrite(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *data);
int i2cRead(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf);
#endif

//------------------End of File----------------------------
