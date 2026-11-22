idf_component_register(
    SRCS "link_stm32.c"
    INCLUDE_DIRS "." "../../../common"
    REQUIRES common driver freertos log
)
