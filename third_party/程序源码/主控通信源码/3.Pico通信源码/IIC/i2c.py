from machine import Pin, I2C
import time

# 初始化 I2C Initializing I2C
i2c = I2C(1, scl=Pin(15), sda=Pin(14), freq=100000)
# 定义I2C设备的地址 Define the address of the I2C device
VoicceADDR=0x2b # 示例地址 Example Address
 
# 定义你想要读取的寄存器地址 Define the register address you want to read
Write_register=0x03
Read_register=0x64

This_red=[0x5F]  
This_blue=[0x60]
This_green=[0x61]
This_yellow=[0x62]
Recognize_yellow=[0x63]
Recognize_green=[0x64]
Recognize_blue=[0x65]
Recognize_red=[0x66]
init=[0x67]       

def set_voice(data):
    i2c.writeto_mem(VoicceADDR, Write_register, bytes(data))


set_voice(init)
set_voice(init)
time.sleep(0.5)

while 1:
        who = i2c.readfrom_mem(VoicceADDR, Read_register, 1)   
        time.sleep(.1)
        print(f"Read data:{who[0]}  ")

        
        
        
        
        
        