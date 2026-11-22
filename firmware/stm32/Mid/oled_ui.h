#ifndef OLED_UI_H
#define OLED_UI_H

#include <stdint.h>
#include "gimbal.h"
#include "motion.h"
#include "../../common/cmd_protocol.h"

typedef struct {
  motion_state_t motion;
  vc_cmd_t last_cmd;
  uint8_t seq_valid;
  uint8_t seq;
  int pitch_deg;
  gimbal_yaw_state_t yaw;
  uint8_t motor_backend_on;
  uint8_t servo_backend_on;
  uint8_t link_ok;
  uint8_t expression; /* 0=idle 1=move 2=listen-ish */
  uint8_t face_id;    /* 0=无；1..10 = 表情包静态脸 */
} oled_status_t;

void oled_ui_format(const oled_status_t *st, char lines[4][17]);
const char *oled_ui_cmd_tag(vc_cmd_t cmd);
const char *oled_ui_yaw_tag(gimbal_yaw_state_t yaw);

#endif
