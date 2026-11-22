idf_component_register(
    SRCS "remote_net.c" "remote_actions.c"
    INCLUDE_DIRS "." "../../../common"
    REQUIRES cmd_router esp_wifi esp_event esp_netif lwip json freertos log
)
