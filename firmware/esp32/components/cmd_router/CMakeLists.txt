idf_component_register(
    SRCS "cmd_router.c"
    INCLUDE_DIRS "." "../../../common"
    REQUIRES common link_stm32 freertos log
)
