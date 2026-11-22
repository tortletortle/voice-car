# OLED UI V0.3

Hardware (local): soft/bitbang I²C on **PB6=SCL / PB7=SDA**, probe `0x3C` then `0x3D`.  
Layout (cloud design):

```text
VC  IDLE              [face]
CMD ----  S--
P090 Y STOP
M:OFF S:OFF R:--
```

| Field | Meaning |
|---|---|
| L0 motion | Chassis only: IDLE/FWD/BACK/LEFT/RIGHT |
| L1 CMD | Last accepted `vc_cmd_t` tag + seq |
| L2 Pxxx | **Commanded** pitch degrees |
| L2 Y | `STOP` / `LEFT` / `RGHT` — continuous yaw **command**, not heading |
| L3 M | `ON` only if `MOTOR_GPIO_CONFIGURED` |
| L3 S | `ON` only if `VC_SERVO_OUTPUT_ENABLE` |
| L3 R | `OK` after ≥1 valid frame since boot (not heartbeat) |

Update policy: redraw when model changes; flush **at most one** 128-byte page per main-loop pass.

Implementation: `firmware/stm32/Mid/oled_ui.*` formats strings; App marks dirty page; BSP flush TODO (bringup already proved the bus).
