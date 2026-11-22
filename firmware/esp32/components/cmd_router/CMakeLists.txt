idf_component_register(
  SRCS "cmd_router.c"
       "../../../common/cmd_router_core.c"
  INCLUDE_DIRS "include" "../../../common"
  REQUIRES link_stm32
)
