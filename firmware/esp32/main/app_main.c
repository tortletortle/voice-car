#include "cmd_router.h"
#include "link_stm32.h"
#include "remote_net.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "voice_car_main";

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* UART2 carries the existing six-byte AA 55 command frame to STM32. */
    ESP_ERROR_CHECK(link_stm32_init());
    ESP_ERROR_CHECK(cmd_router_init());

    err = remote_net_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "phone remote control unavailable: %s",
                 esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "phone remote control ready");
}
