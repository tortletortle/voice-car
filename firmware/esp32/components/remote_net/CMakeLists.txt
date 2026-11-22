idf_component_register(
  SRCS "remote_net.c"
  INCLUDE_DIRS "include" "../../../common"
  REQUIRES cmd_router link_stm32 nvs_flash esp_wifi esp_netif esp_event esp_http_server
)
