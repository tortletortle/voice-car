# ESP32-S3 phone remote endpoint

This ESP-IDF project adds the missing phone-to-ESP32 remote path.

## Network

- SoftAP SSID: `VoiceCar-Setup`
- Default password: `ChangeMe123` (change before public/demo use)
- Default SoftAP address: `192.168.4.1`
- TCP JSON-lines port: `3333`
- One phone client at a time.

Join the ESP32 Wi-Fi from the phone, then use the Android app's connection panel with `192.168.4.1:3333`. If Android reports that the network has no Internet, choose to stay connected/use the network anyway.

Request:
```json
{"source":"manual","action":"forward"}
```

Accepted response:
```json
{"ok":true,"action":"forward","stage":"esp32_queued"}
```

The response confirms only that the ESP32 command router accepted the action. This first server milestone does not read STM32 ACK frames and does not report physical actuator feedback.

## UART link

The current source configures ESP32-S3 UART2 TX on GPIO5 at 115200 baud, 8N1 and sends the existing six-byte `AA 55 CMD LEN SEQ CHK` command frame. RX/ACK is not enabled in this milestone.

**Board check required:** GPIO5 and the STM32 UART RX connection must be confirmed against the actual PCB schematic/netlist before flashing. Do not infer that a successful ESP-IDF build proves the board wiring.

## Build

With ESP-IDF 5.3.2 installed:

```sh
cd firmware/esp32
idf.py set-target esp32s3
idf.py build
```

GitHub Actions runs the same ESP-IDF target build on this branch.

## Safety and remaining validation

- STM32 motor/servo firmware must already understand the shared command IDs.
- The phone's STOP is best-effort over Wi-Fi. Keep the STM32 local motion deadline and a physical power disconnect.
- First test with drive wheels lifted, current-limited supply, and an operator at the power switch.
- The default SoftAP password is a development credential, not suitable for an exposed/public deployment.
- No APK-on-phone, ESP32 flash, UART electrical link, STM32 action, motor, servo, or end-to-end physical test is claimed by this source/build milestone.
