idf_component_register(
  SRCS "link_stm32.c"
  INCLUDE_DIRS "include" "../../../common"
  REQUIRES esp_driver_uart freertos
)
