idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES common cmd_router link_stm32 remote_net nvs_flash
)
