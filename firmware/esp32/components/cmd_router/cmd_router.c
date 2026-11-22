#include "cmd_router.h"

#include "cmd_router_core.h"
#include "link_stm32.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "cmd_router";
static vc_router_state_t s_state;
static SemaphoreHandle_t s_mutex;
static TaskHandle_t s_task;

static void router_task(void *arg)
{
    TickType_t last_frame_tick = xTaskGetTickCount();
    (void)arg;
    for (;;) {
        vc_cmd_t command = CMD_NOP;
        bool have_command = false;
        bool heartbeat = false;
        bool sent = true;
        const TickType_t now = xTaskGetTickCount();

        /*
         * Serialize state selection with one bounded UART frame write.
         * A newly submitted STOP therefore cannot be overtaken by a command
         * that was removed from the pending state but not yet transmitted.
         */
        if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
            have_command = vc_router_state_take(&s_state, &command) != 0u;
            if (have_command) {
                sent = link_stm32_send(command);
                last_frame_tick = xTaskGetTickCount();
                if (command == CMD_STOP) {
                    vc_router_state_complete_stop(&s_state, sent ? 1u : 0u);
                } else if (!sent) {
                    /* Preserve a failed item only when no newer item replaced it. */
                    if (vc_cmd_is_motion((uint8_t)command) &&
                        !s_state.normal_pending) {
                        (void)vc_router_state_submit(&s_state, command);
                    } else if (vc_cmd_is_gimbal((uint8_t)command) &&
                               !s_state.gimbal_pending) {
                        (void)vc_router_state_submit(&s_state, command);
                    }
                }
            } else if ((TickType_t)(now - last_frame_tick) >= pdMS_TO_TICKS(250)) {
                /* STM32 link watchdog expects a valid frame at least every 1s. */
                sent = link_stm32_send(CMD_NOP);
                heartbeat = true;
                last_frame_tick = xTaskGetTickCount();
            }
            (void)xSemaphoreGive(s_mutex);
        }

        if (!have_command && !heartbeat) {
            vTaskDelay(pdMS_TO_TICKS(5));
        } else if (!sent) {
            ESP_LOGW(TAG, "UART transmit failed; pending command retained when safe");
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

esp_err_t cmd_router_init(void)
{
    if (s_mutex != NULL) return ESP_OK;
    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) return ESP_ERR_NO_MEM;

    vc_router_state_init(&s_state);
    if (xTaskCreate(router_task, "cmd_router", 3072, NULL, 8, &s_task) != pdPASS) {
        vSemaphoreDelete(s_mutex);
        s_mutex = NULL;
        s_task = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

bool cmd_router_submit(vc_cmd_t cmd)
{
    uint8_t accepted;
    if (s_mutex == NULL) return false;
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    accepted = vc_router_state_submit(&s_state, cmd);
    (void)xSemaphoreGive(s_mutex);
    return accepted != 0u;
}
