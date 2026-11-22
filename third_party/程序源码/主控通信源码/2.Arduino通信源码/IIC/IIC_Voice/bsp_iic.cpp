#include "bsp_iic.hpp"

uint8_t g_iicdata[1];

//设置水平翻转 Set horizontal flip
void set_voice(uint8_t data)
{
	ESPWIFIWire.beginTransmission(VoicceADDR);      // 开始与设备通信 Start communicating with the device
  ESPWIFIWire.write(Write_register);                // 写入要操作的寄存器地址 Write the register address to be operated
  ESPWIFIWire.write(data);                           // 写入数据 Writing Data
  ESPWIFIWire.endTransmission();         
}



void deal_camera_data(void)
{
	ESPWIFIWire.beginTransmission(VoicceADDR);
  ESPWIFIWire.write(Read_register);  // 写入寄存器地址 Write register address
  ESPWIFIWire.endTransmission(false);  // 不结束连接，准备读取 Do not end the connection, prepare to read Do not end the connection, prepare to read
  ESPWIFIWire.requestFrom(VoicceADDR, 1);  // 读取1字节 Read 1 byte
  if (ESPWIFIWire.available()) {
    g_iicdata[0] = ESPWIFIWire.read();
  }
  ESPWIFISerial.println(g_iicdata[0]);
  delay(50);
}





