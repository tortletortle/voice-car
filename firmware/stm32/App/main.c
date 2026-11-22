/**
 * STM32F103 voice-car executor — skeleton.
 * Open/attach this tree in Keil MDK (SPL or HAL). Wire SysTick = 1 ms.
 */
#include <stdint.h>
#include "app.h"

volatile uint32_t g_ms;

void SysTick_Handler(void)
{
  g_ms++;
}

int main(void)
{
  /* TODO: SystemInit, clocks, SysTick_Config(SystemCoreClock/1000) */
  app_init();

  uint32_t last = 0;
  for (;;) {
    uint32_t now = g_ms;
    uint32_t dt = now - last;
    if (dt == 0) continue;
    last = now;
    app_loop(dt);
  }
}
