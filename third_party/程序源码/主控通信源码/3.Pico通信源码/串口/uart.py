from machine import Pin, UART
import time

# 初始化串口 UART1，使用默认的 GP4 (TX) 和 GP5 (RX) Initialize serial port UART1, use default GP4 (TX) and GP5 (RX)
uart = UART(1, baudrate=115200, tx=Pin(4), rx=Pin(5))

This_red=0x5F
This_blue=0x60
This_green=0x61
This_yellow=0x62
Recognize_yellow=0x63
Recognize_green=0x64
Recognize_blue=0x65
Recognize_red=0x66
init=0x67


#wifi模式选择 Wi-Fi mode selection
def set_voice(data):
    global uart
    cmd = [0xAA, 0x55, 0xFF, data,0xFB]
    byte_data = bytearray(cmd)
    uart.write(byte_data)
    time.sleep(2) #因为模块会自动重启 Because the module will automatically restart

try:
    set_voice(This_blue)
    while 1:
        if uart.any():
            data = uart.read()
            print("Received:", data.hex()[6:8]) 
        time.sleep(0.1)
except KeyboardInterrupt:
    if uart != None:
        uart.close()
