idf_component_register(
  SRCS "voice_uart.c"
  INCLUDE_DIRS "include" "../../../common"
  REQUIRES esp_driver_uart cmd_router link_stm32
)
