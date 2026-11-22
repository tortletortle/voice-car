#include "link_stm32.h"

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define LINK_UART_NUM UART_NUM_2
#define LINK_TXD_GPIO 5
#define LINK_BAUD 115200
#define LINK_TX_TIMEOUT_MS 30u

static const char *TAG = "link_stm32";
static SemaphoreHandle_t s_tx_mutex;
static uint8_t s_sequence;

esp_err_t link_stm32_init(void)
{
    const uart_config_t config = {
        .baud_rate = LINK_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    esp_err_t err;

    if (s_tx_mutex != NULL) return ESP_OK;
    s_tx_mutex = xSemaphoreCreateMutex();
    if (s_tx_mutex == NULL) return ESP_ERR_NO_MEM;

    /* TX-only in this first remote milestone: STM32 ACK reception is not wired. */
    err = uart_driver_install(LINK_UART_NUM, 256, 0, 0, NULL, 0);
    if (err != ESP_OK) goto fail_mutex;
    err = uart_param_config(LINK_UART_NUM, &config);
    if (err != ESP_OK) goto fail_uart;
    err = uart_set_pin(LINK_UART_NUM, LINK_TXD_GPIO, UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) goto fail_uart;

    ESP_LOGI(TAG, "UART2 TX ready: GPIO%d, %d baud, 8N1",
             LINK_TXD_GPIO, LINK_BAUD);
    return ESP_OK;

fail_uart:
    (void)uart_driver_delete(LINK_UART_NUM);
fail_mutex:
    vSemaphoreDelete(s_tx_mutex);
    s_tx_mutex = NULL;
    return err;
}

bool link_stm32_send(vc_cmd_t cmd)
{
    uint8_t frame[VC_FRAME_LEN];
    uint8_t sequence;
    int written;
    esp_err_t tx_result;

    if (!vc_cmd_is_known((uint8_t)cmd) || cmd == CMD_NOP ||
        s_tx_mutex == NULL) {
        return false;
    }
    if (xSemaphoreTake(s_tx_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return false;

    sequence = s_sequence++;
    vc_pack((uint8_t)cmd, sequence, frame);
    written = uart_write_bytes(LINK_UART_NUM, (const char *)frame,
                               sizeof(frame));
    tx_result = (written == (int)sizeof(frame))
        ? uart_wait_tx_done(LINK_UART_NUM, pdMS_TO_TICKS(LINK_TX_TIMEOUT_MS))
        : ESP_FAIL;

    (void)xSemaphoreGive(s_tx_mutex);
    return written == (int)sizeof(frame) && tx_result == ESP_OK;
}
