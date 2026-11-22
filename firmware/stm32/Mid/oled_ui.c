#include "oled_ui.h"

static void fill16(char *dst, const char *src)
{
  uint8_t i = 0;
  if (!dst) return;
  if (src) {
    while (src[i] && i < 16u) {
      dst[i] = src[i];
      ++i;
    }
  }
  while (i < 16u) dst[i++] = ' ';
  dst[16] = '\0';
}

const char *oled_ui_cmd_tag(vc_cmd_t cmd)
{
  switch (cmd) {
  case CMD_STOP: return "STOP";
  case CMD_FWD: return "FWD";
  case CMD_BACK: return "BACK";
  case CMD_LEFT: return "LEFT";
  case CMD_RIGHT: return "RGHT";
  case CMD_GIMBAL_UP: return "G-UP";
  case CMD_GIMBAL_DOWN: return "G-DN";
  case CMD_GIMBAL_LEFT: return "G-L";
  case CMD_GIMBAL_RIGHT: return "G-R";
  case CMD_GIMBAL_PITCH_CENTER: return "G-C";
  case CMD_GIMBAL_YAW_STOP: return "YCTR";
  case CMD_GIMBAL_YAW_SHAKE: return "YSHK";
  case CMD_GIMBAL_PITCH_NOD: return "PNOD";
  case CMD_FACE_HELLO: return "HI";
  case CMD_FACE_HAPPY: return "HAPY";
  case CMD_FACE_CUTE: return "CUTE";
  case CMD_FACE_ANGRY: return "ANGR";
  case CMD_FACE_SLEEP: return "SLEP";
  case CMD_FACE_LISTEN: return "LSTN";
  case CMD_FACE_CONFUSED: return "CONF";
  case CMD_FACE_SURPRISE: return "SURP";
  case CMD_FACE_SAD: return "SAD";
  case CMD_FACE_WINK: return "WINK";
  case CMD_FACE_REJECT: return "NO";
  default: return "----";
  }
}

const char *oled_ui_yaw_tag(gimbal_yaw_state_t yaw)
{
  switch (yaw) {
  case GIMBAL_YAW_STATE_LEFT: return "LEFT";
  case GIMBAL_YAW_STATE_RIGHT: return "RGHT";
  case GIMBAL_YAW_STATE_SHAKE: return "SHAK";
  default: return "CTR";
  }
}

static const char *motion_tag(motion_state_t m)
{
  switch (m) {
  case MOTION_STATE_FWD: return "FWD";
  case MOTION_STATE_BACK: return "BACK";
  case MOTION_STATE_LEFT: return "LEFT";
  case MOTION_STATE_RIGHT: return "RIGHT";
  default: return "IDLE";
  }
}

void oled_ui_format(const oled_status_t *st, char lines[4][17])
{
  char buf[20];
  uint8_t i;
  const char *mot;
  const char *tag;
  if (!st || !lines) return;

  mot = motion_tag(st->motion);
  /* L0: VC  IDLE........... (face rendered separately by driver) */
  buf[0] = 'V'; buf[1] = 'C'; buf[2] = ' '; buf[3] = ' ';
  for (i = 0; mot[i] && (4u + i) < 16u; ++i) buf[4u + i] = mot[i];
  buf[4u + i] = '\0';
  fill16(lines[0], buf);

  /* L1: CMD ----  S-- */
  tag = oled_ui_cmd_tag(st->last_cmd);
  buf[0] = 'C'; buf[1] = 'M'; buf[2] = 'D'; buf[3] = ' ';
  for (i = 0; tag[i] && i < 4u; ++i) buf[4u + i] = tag[i];
  while (i < 4u) { buf[4u + i] = ' '; ++i; }
  buf[8] = ' '; buf[9] = ' ';
  buf[10] = 'S';
  if (st->seq_valid) {
    static const char hex[] = "0123456789ABCDEF";
    buf[11] = hex[(st->seq >> 4) & 0xFu];
    buf[12] = hex[st->seq & 0xFu];
  } else {
    buf[11] = '-';
    buf[12] = '-';
  }
  buf[13] = '\0';
  fill16(lines[1], buf);

  /* L2: P-45 Y STOP — pitch setpoint (0=低头, -45=抬头) */
  {
    int p = st->pitch_deg;
    const char *y = oled_ui_yaw_tag(st->yaw);
    char sign = '+';
    if (p < 0) {
      sign = '-';
      p = -p;
    }
    if (p > 99) p = 99;
    buf[0] = 'P';
    buf[1] = sign;
    buf[2] = (char)('0' + ((p / 10) % 10));
    buf[3] = (char)('0' + (p % 10));
    buf[4] = ' ';
    buf[5] = 'Y';
    buf[6] = ' ';
    for (i = 0; y[i] && (7u + i) < 16u; ++i) buf[7u + i] = y[i];
    buf[7u + i] = '\0';
    fill16(lines[2], buf);
  }

  /* L3: M:OFF S:OFF R:--  (16 chars) */
  buf[0] = 'M';
  buf[1] = ':';
  if (st->motor_backend_on) {
    buf[2] = 'O'; buf[3] = 'N'; buf[4] = ' ';
  } else {
    buf[2] = 'O'; buf[3] = 'F'; buf[4] = 'F';
  }
  buf[5] = ' ';
  buf[6] = 'S';
  buf[7] = ':';
  if (st->servo_backend_on) {
    buf[8] = 'O'; buf[9] = 'N'; buf[10] = ' ';
  } else {
    buf[8] = 'O'; buf[9] = 'F'; buf[10] = 'F';
  }
  buf[11] = ' ';
  buf[12] = 'R';
  buf[13] = ':';
  if (st->link_ok) {
    buf[14] = 'O'; buf[15] = 'K';
  } else {
    buf[14] = '-'; buf[15] = '-';
  }
  buf[16] = '\0';
  fill16(lines[3], buf);
}
