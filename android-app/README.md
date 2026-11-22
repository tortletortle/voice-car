# Voice Car Remote — Android V1.5.0

This branch delivers a controller-first Android app for the Voice Car.

## Phone UI

- Large four-way D-pad: press and hold to move; release to send STOP.
- Prominent emergency STOP in the center of the D-pad.
- Compact Wi-Fi/TCP connection panel with the default ESP32 address `192.168.4.1:3333`.
- Separate head-action controls for nod, shake, greeting, and neutral; less-used gimbal actions stay collapsed.
- Simplified-Chinese SpeechRecognizer input. Recognition fills the command field; the user reviews it and explicitly taps Execute.
- Exact phrase allow-list; negated movement phrases become STOP, negated head actions are rejected, and unknown phrases do not move the car.
- TCP JSONL response validation distinguishes ESP32 queue admission from STM32 App ACK.
- Fixed-rate 400 ms motion refresh. Release, app pause, disconnect, and shutdown all queue a STOP attempt.

## Build

Open this directory in Android Studio (JDK 17, Android SDK Platform 35), or run:

```sh
gradle -p android-app --no-daemon testDebugUnitTest assembleDebug
```

The GitHub Actions workflow builds the debug APK and publishes it as an artifact.

## Connection protocol

Join the ESP32 Wi-Fi network first, then connect to its IP and TCP port. Each request is one UTF-8 JSON line:

```json
{"source":"manual","action":"forward"}
```

The app requires one newline-terminated response with the same action, for example:

```json
{"ok":true,"action":"forward","stage":"esp32_queued"}
```

The app does not implement BLE. It does not send arbitrary GPIO, raw serial bytes, speed, or duration parameters.

## Integration status — important

This is the Android client branch. The current `main` branch contains the shared six-byte command contract and router core, but its README explicitly records that a working ESP32 `remote_net` TCP server is not yet integrated. Therefore a successful Android build does **not** prove that the phone can connect to the current `main` firmware. The next firmware task is to add and start the ESP32 TCP server, connect it to the command router, and verify the JSONL response contract on hardware.

## Safety

- A software ACK is not motor or servo position feedback.
- TCP STOP is best-effort; a broken Wi-Fi link can prevent delivery.
- Keep the STM32 local motion deadline and a physical power disconnect.
- First hardware tests: wheels lifted, current-limited supply, low-risk motion duration, and an operator at the power switch.
- Do not expose the unauthenticated control socket to a public network.
