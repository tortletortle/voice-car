#include "remote_net.h"

#include "remote_actions.h"
#include "remote_net_config.h"
#include "cmd_router.h"

#include "cJSON.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define REMOTE_TASK_STACK 4096u
#define REMOTE_TASK_PRIORITY 5u

static const char *TAG = "remote_net";
static bool s_started;

static bool send_all(int fd, const char *data, size_t length)
{
    size_t sent = 0u;
    while (sent < length) {
        const int count = send(fd, data + sent, length - sent, 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        sent += (size_t)count;
    }
    return true;
}

static bool send_error(int fd, const char *error)
{
    char response[160];
    const int length = snprintf(response, sizeof(response),
        "{\"ok\":false,\"error\":\"%s\"}\n", error);
    return length > 0 && (size_t)length < sizeof(response) &&
           send_all(fd, response, (size_t)length);
}

static bool send_action_result(int fd, bool ok, const char *action,
                               const char *error)
{
    char response[224];
    int length;
    if (ok) {
        length = snprintf(response, sizeof(response),
            "{\"ok\":true,\"action\":\"%s\",\"stage\":\"esp32_queued\"}\n",
            action);
    } else {
        length = snprintf(response, sizeof(response),
            "{\"ok\":false,\"action\":\"%s\",\"stage\":\"esp32\",\"error\":\"%s\"}\n",
            action, error);
    }
    return length > 0 && (size_t)length < sizeof(response) &&
           send_all(fd, response, (size_t)length);
}

static bool request_fields_valid(const cJSON *root)
{
    const cJSON *item;
    unsigned count = 0u;
    bool action_seen = false;
    bool source_seen = false;

    if (!cJSON_IsObject(root)) return false;
    for (item = root->child; item != NULL; item = item->next) {
        ++count;
        if (item->string == NULL) return false;
        if (strcmp(item->string, "action") == 0 && !action_seen) {
            action_seen = true;
        } else if (strcmp(item->string, "source") == 0 && !source_seen) {
            source_seen = true;
        } else {
            return false;
        }
    }
    return count == 2u && action_seen && source_seen;
}

static bool handle_request(int fd, const char *line)
{
    cJSON *root = cJSON_Parse(line);
    const cJSON *source_item;
    const cJSON *action_item;
    vc_remote_action_t action;
    const char *source;
    const char *name;
    bool sent;

    if (root == NULL) return send_error(fd, "invalid_json");
    if (!request_fields_valid(root)) {
        cJSON_Delete(root);
        return send_error(fd, "invalid_fields");
    }

    source_item = cJSON_GetObjectItemCaseSensitive(root, "source");
    action_item = cJSON_GetObjectItemCaseSensitive(root, "action");
    if (!cJSON_IsString(source_item) || !cJSON_IsString(action_item) ||
        source_item->valuestring == NULL || action_item->valuestring == NULL) {
        cJSON_Delete(root);
        return send_error(fd, "invalid_field_type");
    }

    source = source_item->valuestring;
    name = action_item->valuestring;
    if (strcmp(source, "manual") != 0 && strcmp(source, "ai") != 0) {
        cJSON_Delete(root);
        return send_error(fd, "invalid_source");
    }
    if (strlen(name) >= 40u || !vc_remote_action_lookup(name, &action)) {
        cJSON_Delete(root);
        return send_error(fd, "unknown_action");
    }

    sent = cmd_router_submit(action.command);
    if (!sent) {
        const bool replied = send_action_result(fd, false, name, "router_rejected");
        cJSON_Delete(root);
        return replied;
    } else {
        const bool replied = send_action_result(fd, true, name, NULL);
        cJSON_Delete(root);
        return replied;
    }
}

static void remote_client_loop(int client_fd)
{
    char incoming[64];
    char line[REMOTE_LINE_MAX + 1u];
    size_t used = 0u;
    bool dropping_line = false;

    for (;;) {
        const int received = recv(client_fd, incoming, sizeof(incoming), 0);
        int i;
        if (received == 0) break;
        if (received < 0) {
            if (errno == EINTR) continue;
            ESP_LOGW(TAG, "client recv failed: errno=%d", errno);
            break;
        }

        for (i = 0; i < received; ++i) {
            const char ch = incoming[i];
            if (ch == '\r') continue;
            if (ch == '\n') {
                if (dropping_line) {
                    if (!send_error(client_fd, "line_too_long")) return;
                } else if (used > 0u) {
                    line[used] = '\0';
                    if (!handle_request(client_fd, line)) return;
                }
                used = 0u;
                dropping_line = false;
                continue;
            }
            if (dropping_line) continue;
            if (used >= REMOTE_LINE_MAX) {
                dropping_line = true;
                used = 0u;
                continue;
            }
            line[used++] = ch;
        }
    }
}

static void remote_server_task(void *arg)
{
    int server_fd;
    struct sockaddr_in address;
    int reuse = 1;
    (void)arg;

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server_fd < 0) {
        ESP_LOGE(TAG, "socket creation failed: errno=%d", errno);
        vTaskDelete(NULL);
        return;
    }
    (void)setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons((uint16_t)REMOTE_TCP_PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) != 0) {
        ESP_LOGE(TAG, "bind failed: errno=%d", errno);
        close(server_fd);
        vTaskDelete(NULL);
        return;
    }
    if (listen(server_fd, 1) != 0) {
        ESP_LOGE(TAG, "listen failed: errno=%d", errno);
        close(server_fd);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "TCP control server listening on port %u",
             (unsigned)REMOTE_TCP_PORT);
    for (;;) {
        struct sockaddr_in peer;
        socklen_t peer_length = sizeof(peer);
        const int client_fd = accept(server_fd, (struct sockaddr *)&peer,
                                     &peer_length);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            ESP_LOGW(TAG, "accept failed: errno=%d", errno);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        ESP_LOGI(TAG, "phone client connected");
        remote_client_loop(client_fd);
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        ESP_LOGI(TAG, "phone client disconnected");
    }
}

esp_err_t remote_net_init(void)
{
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t ap_config = {0};
    esp_err_t err;

    if (s_started) return ESP_OK;
    if (strlen(REMOTE_AP_PASSWORD) < 8u ||
        strlen(REMOTE_AP_SSID) == 0u ||
        strlen(REMOTE_AP_SSID) > 32u) {
        return ESP_ERR_INVALID_ARG;
    }

    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    if (esp_netif_create_default_wifi_ap() == NULL) return ESP_FAIL;

    err = esp_wifi_init(&wifi_config);
    if (err != ESP_OK) return err;
    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) return err;

    (void)snprintf((char *)ap_config.ap.ssid, sizeof(ap_config.ap.ssid),
                   "%s", REMOTE_AP_SSID);
    (void)snprintf((char *)ap_config.ap.password,
                   sizeof(ap_config.ap.password), "%s", REMOTE_AP_PASSWORD);
    ap_config.ap.ssid_len = (uint8_t)strlen(REMOTE_AP_SSID);
    ap_config.ap.channel = REMOTE_AP_CHANNEL;
    ap_config.ap.max_connection = REMOTE_AP_MAX_CONN;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.pmf_cfg.required = false;

    err = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    if (err != ESP_OK) return err;
    err = esp_wifi_start();
    if (err != ESP_OK) return err;

    if (xTaskCreate(remote_server_task, "remote_tcp", REMOTE_TASK_STACK,
                    NULL, REMOTE_TASK_PRIORITY, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    s_started = true;
    ESP_LOGI(TAG, "SoftAP ready: SSID=%s, TCP=192.168.4.1:%u",
             REMOTE_AP_SSID, (unsigned)REMOTE_TCP_PORT);
    return ESP_OK;
}
