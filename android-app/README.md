# Voice Car Remote — Android V1.5.0

This branch delivers a controller-first Android app and the first ESP32 TCP endpoint for the Voice Car.

## Phone UI

- Large four-way D-pad: press and hold to move; release to send STOP.
- Prominent emergency STOP in the center of the D-pad.
- Compact Wi-Fi/TCP connection panel with the default ESP32 address `192.168.4.1:3333`.
- Separate head-action controls for nod, shake, greeting, and neutral; less-used gimbal controls stay collapsed.
- Simplified-Chinese Android SpeechRecognizer input. Recognition fills the command field; the user reviews it and explicitly taps Execute. This release uses an exact local phrase map; it does not call an LLM API or use the onboard CI1302 microphone.
- Exact command allow-list; negated movement phrases become STOP, negated head actions are rejected, and unknown phrases do not move the car.
- TCP JSONL response validation distinguishes ESP32 queue admission from STM32 App ACK.
- Fixed-rate motion refresh and ordered STOP on release, app pause, disconnect and shutdown.

## Build

Open this directory in Android Studio (JDK 17, Android SDK Platform 35), or run:

```sh
gradle -p android-app --no-daemon testDebugUnitTest assembleDebug
```

GitHub Actions builds the debug APK and publishes it as an artifact after the workflow passes.

## Connection protocol

Join the ESP32 SoftAP `VoiceCar-Setup`, then connect to `192.168.4.1:3333`. If Android reports that this Wi-Fi has no Internet, choose the option to stay connected/use it anyway; otherwise the phone may switch back to mobile data. Each request is one UTF-8 JSON line:

```json
{"source":"manual","action":"forward"}
```

The app expects one newline-terminated response with the same action, for example:

```json
{"ok":true,"action":"forward","stage":"esp32_queued"}
```

The ESP32 SoftAP server, action allow-list, command router and UART2 transmitter are included in this branch under `firmware/esp32`. This is a remote-only bring-up image: it does not initialize the CI1302 voice UART or replace the full production firmware. The response confirms ESP32 queue admission only; STM32 ACK reception and physical actuator feedback are not yet included.

## Remaining hardware gate

The ESP32 UART2 source currently uses TX GPIO5 at 115200 8N1. Verify that mapping against the actual PCB schematic/netlist before flashing. The branch's ESP-IDF build is not proof of the GPIO wiring or STM32 firmware compatibility.

## Safety

- A successful app response means ESP32 queue admission, not physical motor/servo movement.
- TCP STOP is best-effort; a broken Wi-Fi link can prevent delivery.
- Keep the STM32 local motion deadline and a physical power disconnect.
- First hardware tests: wheels lifted, current-limited supply, and an operator at the power switch.
- Do not expose the unauthenticated control socket to a public network.
