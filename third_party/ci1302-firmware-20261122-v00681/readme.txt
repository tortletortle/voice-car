1. For detailed information about the SDK's components, features, and development, please refer to the SDK documentation for the corresponding chip series in the Software Development section of the Docume
   Taking CI13060 as an example, the reference link is as follows: https://document.chipintelli.com/%E8%BD%AF%E4%BB%B6%E5%BC%80%E5%8F%91/SDK/CI130X%E8%8A%AF%E7%89%87SDK/CI-SDK-Offline/

2. If you need to modify and adjust the relevant algorithm parameters in the SDK, please refer to the SDK documentation for the corresponding chip series in the Software Development section of the Documentation Center.
   Taking AEC as an example, the reference link is as follows:：https://document.chipintelli.com/%E8%BD%AF%E4%BB%B6%E5%BC%80%E5%8F%91/SDK/CI130X%E8%8A%AF%E7%89%87SDK/components/%E5%9B%9E%E5%A3%B0%E6%B6%88%E9%99%A4%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E/

3. If the firmware size exceeds the flash capacity of the selected chip, please try again after switching to a smaller acoustic model, reducing the number of command words or voice prompts, or using a chip with larger flash capacity.

4. If algorithms are enabled, the system provides relatively less memory space for language model. To ensure normal recognition operation, please reduce the number of command words.


The firmware communication protocol is as follows:
<welcome>:AA 55 FF 01 FB:AA 55 FF 01 FB
<inactivate>:AA 55 FF 02 FB:AA 55 FF 02 FB
你好小车:AA 55 03 00 FB:AA 55 03 00 FB
增大音量:AA 55 00 F1 FB:AA 55 00 F1 FB
减小音量:AA 55 00 F2 FB:AA 55 00 F2 FB
最大音量:AA 55 00 F3 FB:AA 55 00 F3 FB
中等音量:AA 55 00 F4 FB:AA 55 00 F4 FB
最小音量:AA 55 00 F5 FB:AA 55 00 F5 FB
打开播报:AA 55 00 F6 FB:AA 55 00 F6 FB
关闭播报:AA 55 00 F7 FB:AA 55 00 F7 FB
立刻停车:AA 55 00 01 FB:AA 55 00 01 FB
小车前进:AA 55 00 02 FB:AA 55 00 02 FB
向前开动:AA 55 00 02 FB:AA 55 00 02 FB
开始前进:AA 55 00 02 FB:AA 55 00 02 FB
小车后退:AA 55 00 03 FB:AA 55 00 03 FB
向后倒车:AA 55 00 03 FB:AA 55 00 03 FB
开始后退:AA 55 00 03 FB:AA 55 00 03 FB
小车左转:AA 55 00 04 FB:AA 55 00 04 FB
小车右转:AA 55 00 05 FB:AA 55 00 05 FB
云台抬头:AA 55 00 10 FB:AA 55 00 10 FB
云台低头:AA 55 00 11 FB:AA 55 00 11 FB
云台向左:AA 55 00 12 FB:AA 55 00 12 FB
云台向右:AA 55 00 13 FB:AA 55 00 13 FB
俯仰回中:AA 55 00 14 FB:AA 55 00 14 FB
偏航回中:AA 55 00 15 FB:AA 55 00 15 FB
云台摇头:AA 55 00 16 FB:AA 55 00 16 FB
云台点头:AA 55 00 17 FB:AA 55 00 17 FB
打个招呼:AA 55 00 20 FB:AA 55 00 20 FB
你真好看:AA 55 00 21 FB:AA 55 00 21 FB
讲个笑话:AA 55 00 21 FB:AA 55 00 21 FB
你真可爱:AA 55 00 22 FB:AA 55 00 22 FB
你是谁呀:AA 55 00 22 FB:AA 55 00 22 FB
我生气了:AA 55 00 23 FB:AA 55 00 23 FB
我想睡觉:AA 55 00 24 FB:AA 55 00 24 FB
我在听呢:AA 55 00 25 FB:AA 55 00 25 FB
我没听懂:AA 55 00 26 FB:AA 55 00 26 FB
吓我一跳:AA 55 00 27 FB:AA 55 00 27 FB
我很难过:AA 55 00 28 FB:AA 55 00 28 FB
眨一下眼:AA 55 00 29 FB:AA 55 00 29 FB
我喜欢你:AA 55 00 29 FB:AA 55 00 29 FB
这个不行:AA 55 00 2A FB:AA 55 00 2A FB
