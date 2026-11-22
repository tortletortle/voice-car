#include "remote_net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "cmd_protocol.h"
#include "cmd_router.h"
#include "link_stm32.h"

/* 用户 2026-11-22 拍板：SoftAP + 网页按钮；SSID/密码按口述 */
#define REMOTE_AP_SSID     "realme"
#define REMOTE_AP_PASS     "99999999"
#define REMOTE_AP_CHANNEL  1
#define REMOTE_AP_MAX_CONN 4

static const char *TAG = "remote_net";

static const char INDEX_HTML[] =
    "<!DOCTYPE html><html lang=\"zh-CN\"><head>"
    "<meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no\">"
    "<title>VoiceCar</title>"
    "<style>"
    "*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}"
    "body{margin:0;font-family:system-ui,sans-serif;background:#111;color:#eee;padding:12px}"
    "h1{font-size:1.1rem;margin:0 0 8px;text-align:center}"
    ".hint{font-size:.75rem;color:#888;text-align:center;margin-bottom:12px}"
    ".grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;max-width:360px;margin:0 auto}"
    "button{appearance:none;border:0;border-radius:12px;padding:16px 8px;font-size:1rem;"
    "font-weight:600;color:#fff;background:#333;min-height:56px}"
    "button:active{transform:scale(.97);opacity:.85}"
    ".stop{grid-column:1/-1;background:#c0392b;font-size:1.2rem;min-height:64px}"
    ".drive{background:#2980b9}"
    ".gimbal{background:#8e44ad}"
    ".yaw{background:#16a085}"
    ".face{background:#d35400;min-height:48px;font-size:.9rem}"
    ".sec{max-width:360px;margin:16px auto 0;font-size:.8rem;color:#aaa}"
    "#st{min-height:1.2em;text-align:center;margin-top:10px;font-size:.85rem;color:#7dcea0}"
    "</style></head><body>"
    "<h1>VoiceCar 遥控</h1>"
    "<div class=\"hint\">连热点 realme → 打开本页 → 点按钮</div>"
    "<div class=\"grid\">"
    "<button class=\"stop\" onclick=\"go('STOP')\">STOP 停</button>"
    "<button class=\"drive\" onclick=\"go('LEFT')\">← 左转</button>"
    "<button class=\"drive\" onclick=\"go('FWD')\">↑ 前进</button>"
    "<button class=\"drive\" onclick=\"go('RIGHT')\">右转 →</button>"
    "<div></div>"
    "<button class=\"drive\" onclick=\"go('BACK')\">↓ 后退</button>"
    "<div></div>"
    "</div>"
    "<div class=\"sec\">云台俯仰</div>"
    "<div class=\"grid\">"
    "<button class=\"gimbal\" onclick=\"go('UP')\">抬头</button>"
    "<button class=\"gimbal\" onclick=\"go('DOWN')\">低头</button>"
    "<button class=\"gimbal\" onclick=\"go('NOD')\">点头</button>"
    "<button class=\"gimbal\" onclick=\"go('PCEN')\">俯仰回中</button>"
    "</div>"
    "<div class=\"sec\">云台偏航</div>"
    "<div class=\"grid\">"
    "<button class=\"yaw\" onclick=\"go('YLEFT')\">头左转</button>"
    "<button class=\"yaw\" onclick=\"go('YRIGHT')\">头右转</button>"
    "<button class=\"yaw\" onclick=\"go('SHAKE')\">摇头</button>"
    "<button class=\"yaw\" onclick=\"go('YCTR')\">偏航回中</button>"
    "</div>"
    "<div class=\"sec\">表情</div>"
    "<div class=\"grid\">"
    "<button class=\"face\" onclick=\"go('HI')\">打招呼</button>"
    "<button class=\"face\" onclick=\"go('HAPPY')\">开心</button>"
    "<button class=\"face\" onclick=\"go('CUTE')\">可爱</button>"
    "<button class=\"face\" onclick=\"go('ANGRY')\">生气</button>"
    "<button class=\"face\" onclick=\"go('SLEEP')\">睡觉</button>"
    "<button class=\"face\" onclick=\"go('LISTEN')\">在听</button>"
    "<button class=\"face\" onclick=\"go('CONF')\">疑惑</button>"
    "<button class=\"face\" onclick=\"go('SURP')\">惊讶</button>"
    "<button class=\"face\" onclick=\"go('SAD')\">难过</button>"
    "<button class=\"face\" onclick=\"go('WINK')\">眨眼</button>"
    "<button class=\"face\" onclick=\"go('NO')\">不行</button>"
    "</div>"
    "<div id=\"st\"></div>"
    "<script>"
    "async function go(c){"
    "  const st=document.getElementById('st');"
    "  st.textContent='… '+c;"
    "  try{"
    "    const r=await fetch('/api/cmd?c='+encodeURIComponent(c),{method:'GET',cache:'no-store'});"
    "    const t=await r.text();"
    "    st.textContent=r.ok?('OK '+t):('ERR '+t);"
    "  }catch(e){st.textContent='网络失败';}"
    "}"
    "</script></body></html>";

typedef struct {
  const char *name;
  vc_cmd_t cmd;
} remote_cmd_map_t;

static const remote_cmd_map_t s_map[] = {
  {"STOP", CMD_STOP},
  {"FWD", CMD_FWD},
  {"BACK", CMD_BACK},
  {"LEFT", CMD_LEFT},
  {"RIGHT", CMD_RIGHT},
  {"UP", CMD_GIMBAL_UP},
  {"DOWN", CMD_GIMBAL_DOWN},
  {"YLEFT", CMD_GIMBAL_LEFT},
  {"YRIGHT", CMD_GIMBAL_RIGHT},
  {"PCEN", CMD_GIMBAL_PITCH_CENTER},
  {"CENTER", CMD_GIMBAL_PITCH_CENTER},
  {"YCTR", CMD_GIMBAL_YAW_STOP},
  {"YAWSTOP", CMD_GIMBAL_YAW_STOP},
  {"SHAKE", CMD_GIMBAL_YAW_SHAKE},
  {"NOD", CMD_GIMBAL_PITCH_NOD},
  {"HI", CMD_FACE_HELLO},
  {"HELLO", CMD_FACE_HELLO},
  {"HAPPY", CMD_FACE_HAPPY},
  {"CUTE", CMD_FACE_CUTE},
  {"ANGRY", CMD_FACE_ANGRY},
  {"SLEEP", CMD_FACE_SLEEP},
  {"LISTEN", CMD_FACE_LISTEN},
  {"CONF", CMD_FACE_CONFUSED},
  {"SURP", CMD_FACE_SURPRISE},
  {"SAD", CMD_FACE_SAD},
  {"WINK", CMD_FACE_WINK},
  {"NO", CMD_FACE_REJECT},
  {"REJECT", CMD_FACE_REJECT},
};

static int map_name_to_cmd(const char *name, vc_cmd_t *out)
{
  size_t i;
  if (!name || !out) {
    return 0;
  }
  for (i = 0; i < sizeof(s_map) / sizeof(s_map[0]); ++i) {
    if (strcasecmp(name, s_map[i].name) == 0) {
      *out = s_map[i].cmd;
      return 1;
    }
  }
  return 0;
}

static esp_err_t root_get(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t api_cmd_get(httpd_req_t *req)
{
  char buf[64];
  char name[24];
  vc_cmd_t cmd;
  char line[48];

  if (httpd_req_get_url_query_len(req) == 0 ||
      httpd_req_get_url_query_str(req, buf, sizeof(buf)) != ESP_OK) {
    httpd_resp_set_status(req, "400 Bad Request");
    return httpd_resp_send(req, "missing query", HTTPD_RESP_USE_STRLEN);
  }
  if (httpd_query_key_value(buf, "c", name, sizeof(name)) != ESP_OK) {
    httpd_resp_set_status(req, "400 Bad Request");
    return httpd_resp_send(req, "need c=", HTTPD_RESP_USE_STRLEN);
  }
  if (!map_name_to_cmd(name, &cmd)) {
    httpd_resp_set_status(req, "400 Bad Request");
    return httpd_resp_send(req, "unknown cmd", HTTPD_RESP_USE_STRLEN);
  }

  ESP_LOGI(TAG, "web cmd %s -> 0x%02X", name, (unsigned)cmd);
  cmd_router_submit(cmd);

  snprintf(line, sizeof(line), "web:%s\r\n", name);
  link_stm32_pc_write(line);

  httpd_resp_set_type(req, "text/plain");
  snprintf(line, sizeof(line), "%s 0x%02X", name, (unsigned)cmd);
  return httpd_resp_send(req, line, HTTPD_RESP_USE_STRLEN);
}

static httpd_handle_t start_http(void)
{
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t server = NULL;
  cfg.lru_purge_enable = true;

  if (httpd_start(&server, &cfg) != ESP_OK) {
    ESP_LOGE(TAG, "httpd_start failed");
    return NULL;
  }

  const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get,
    .user_ctx = NULL,
  };
  const httpd_uri_t api = {
    .uri = "/api/cmd",
    .method = HTTP_GET,
    .handler = api_cmd_get,
    .user_ctx = NULL,
  };
  httpd_register_uri_handler(server, &root);
  httpd_register_uri_handler(server, &api);
  ESP_LOGI(TAG, "HTTP ready http://192.168.4.1/");
  return server;
}

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
  (void)arg;
  if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t *e = (wifi_event_ap_staconnected_t *)data;
    ESP_LOGI(TAG, "STA join AID=%d", e->aid);
  } else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *e = (wifi_event_ap_stadisconnected_t *)data;
    ESP_LOGI(TAG, "STA leave AID=%d", e->aid);
  }
}

static void start_softap(void)
{
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event, NULL, NULL));

  wifi_config_t wifi_config = {0};
  strncpy((char *)wifi_config.ap.ssid, REMOTE_AP_SSID, sizeof(wifi_config.ap.ssid));
  strncpy((char *)wifi_config.ap.password, REMOTE_AP_PASS, sizeof(wifi_config.ap.password));
  wifi_config.ap.ssid_len = (uint8_t)strlen(REMOTE_AP_SSID);
  wifi_config.ap.channel = REMOTE_AP_CHANNEL;
  wifi_config.ap.max_connection = REMOTE_AP_MAX_CONN;
  wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
  wifi_config.ap.pmf_cfg.required = false;

  if (strlen(REMOTE_AP_PASS) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "SoftAP SSID=%s pass=%s IP=192.168.4.1", REMOTE_AP_SSID, REMOTE_AP_PASS);
  link_stm32_pc_write("esp32: SoftAP realme http://192.168.4.1/\r\n");
}

void remote_net_init(void)
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  start_softap();
  if (!start_http()) {
    ESP_LOGE(TAG, "HTTP server not started");
  }
}
