idf_component_register(
    SRCS "link_stm32.c"
    INCLUDE_DIRS "." "../../../common"
    REQUIRES common esp_driver_uart freertos log
)
