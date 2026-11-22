#include "app.h"
#include "../Bsp/board.h"
#include "../Bsp/uart_link.h"
#include "../Bsp/motor.h"
#include "../Bsp/servo.h"
#include "../Mid/protocol.h"
#include "../Mid/motion.h"
#include "../Mid/gimbal.h"
#include "../Mid/oled_ui.h"
#include "../../common/cmd_protocol.h"

static uint8_t s_last_seq;
static uint8_t s_seq_valid;
static uint8_t s_link_ok;
static vc_cmd_t s_last_cmd;
static uint8_t s_face_id;
static char s_oled_lines[4][17];
static char s_oled_drawn[4][17];
static uint8_t s_oled_page;

void app_init(void)
{
  uint8_t i;
  uart_link_init();
  motor_init();
  servo_init();
  protocol_init();
  motion_init();
  gimbal_init();
  s_last_seq = 0;
  s_seq_valid = 0;
  s_link_ok = 0;
  s_last_cmd = CMD_NOP;
  s_face_id = 0u;
  s_oled_page = 0;
  for (i = 0; i < 4u; ++i) {
    s_oled_lines[i][0] = '\0';
    s_oled_drawn[i][0] = '\0';
  }
}

static void dispatch(vc_cmd_t cmd)
{
  switch (cmd) {
  case CMD_STOP:
  case CMD_FWD:
  case CMD_BACK:
  case CMD_LEFT:
  case CMD_RIGHT:
    motion_on_cmd(cmd);
    break;
  case CMD_GIMBAL_UP:
  case CMD_GIMBAL_DOWN:
  case CMD_GIMBAL_LEFT:
  case CMD_GIMBAL_RIGHT:
  case CMD_GIMBAL_PITCH_CENTER:
  case CMD_GIMBAL_YAW_STOP:
  case CMD_GIMBAL_YAW_SHAKE:
  case CMD_GIMBAL_PITCH_NOD:
    gimbal_on_cmd(cmd);
    break;
  case CMD_FACE_HELLO:
  case CMD_FACE_HAPPY:
  case CMD_FACE_CUTE:
  case CMD_FACE_ANGRY:
  case CMD_FACE_SLEEP:
  case CMD_FACE_LISTEN:
  case CMD_FACE_CONFUSED:
  case CMD_FACE_SURPRISE:
  case CMD_FACE_SAD:
  case CMD_FACE_WINK:
  case CMD_FACE_REJECT:
    (void)vc_cmd_face_asset((uint8_t)cmd, &s_face_id);
    break;
  default:
    break;
  }
}

static void oled_refresh_model(void)
{
  oled_status_t st;
  uint8_t i;
  uint8_t j;

  st.motion = motion_state();
  st.last_cmd = s_last_cmd;
  st.seq_valid = s_seq_valid;
  st.seq = s_last_seq;
  st.pitch_deg = gimbal_pitch_deg();
  st.yaw = gimbal_yaw_state();
  st.motor_backend_on = (uint8_t)(MOTOR_GPIO_CONFIGURED ? 1u : 0u);
  st.servo_backend_on = servo_output_enabled();
  st.link_ok = s_link_ok;
  st.expression = (st.motion == MOTION_STATE_IDLE) ? 0u : 1u;
  st.face_id = s_face_id;
  oled_ui_format(&st, s_oled_lines);

  if (!OLED_ENABLED) return;

  for (i = 0; i < 4u; ++i) {
    j = 0;
    while (s_oled_lines[i][j] == s_oled_drawn[i][j] && s_oled_lines[i][j] != '\0') {
      ++j;
    }
    if (s_oled_lines[i][j] != s_oled_drawn[i][j]) {
      for (j = 0; j < 17u; ++j) {
        s_oled_drawn[i][j] = s_oled_lines[i][j];
      }
      s_oled_page = i;
      /* TODO: soft-I2C flush one 128-byte page on PB6/PB7 (bringup proven). */
      break;
    }
  }
  (void)s_oled_page;
  (void)OLED_SCL_PIN_PB;
  (void)OLED_SDA_PIN_PB;
  (void)OLED_ADDR_PRIMARY;
  (void)OLED_ADDR_FALLBACK;
  (void)OLED_COLUMN_OFFSET;
}

void app_loop(uint32_t elapsed_ms)
{
  uint8_t raw;
  vc_cmd_t cmd;
  uint8_t seq;

  while (uart_link_pop_byte(&raw)) {
    if (protocol_feed(raw, &cmd, &seq)) {
      s_last_seq = seq;
      s_seq_valid = 1u;
      s_link_ok = 1u;
      s_last_cmd = cmd;
      dispatch(cmd);
    }
  }

  motion_tick(elapsed_ms);
  gimbal_tick(elapsed_ms);
  oled_refresh_model();
}
