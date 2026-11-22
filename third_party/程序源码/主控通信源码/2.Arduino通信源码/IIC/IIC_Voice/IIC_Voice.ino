#include <Wire.h>
#include <stdio.h>
#include "bsp_iic.hpp"

void setup() 
{
  Wire.begin();        // join i2c bus (address optional for master)
  ESPWIFISerial.begin(115200);
  set_voice(init);
  delay(50);
}


void loop() 
{
  deal_camera_data();
}


