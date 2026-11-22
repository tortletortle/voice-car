idf_component_register(
  SRCS "app_main.c"
  INCLUDE_DIRS "."
  REQUIRES voice_uart remote_net cmd_router link_stm32
)
