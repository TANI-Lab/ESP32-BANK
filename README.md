# BLE Safe Lock with ESP32

This Arduino project implements a **BLE-controlled servo-based lock** system using an ESP32 and a mobile device (e.g., iPhone). It communicates via BLE and securely opens or closes a lock based on a registered password.

## 🔧 Features

- BLE communication with custom Service and Characteristic UUIDs
- Password-protected lock/unlock command via BLE
- Device name and password can be set dynamically over BLE
- BLE device name shown correctly in iOS/macOS scan lists
- Servo motor angle calibration included
- Status persistence via SPIFFS
- Compatible with BLE terminal apps (custom or generic)

## 📱 Use Case

- Combine with a mobile app (e.g. BLETM for iOS)
- Send password via BLE to unlock a safe box
- Servo moves between locked and unlocked positions

## 🧱 Hardware Requirements

- ESP32 module
- Servo motor (tested with MS18)
- External power (recommended for servo stability)
- BLE-compatible smartphone (for controlling)

## 📦 Libraries Required

Make sure to install these libraries in Arduino IDE:

- ESP32Servo
- BLEDevice (included in ESP32 core)
- SPIFFS (included in ESP32 core)
- FreeRTOS (included in ESP32 core)

## 📡 BLE Configuration

| Parameter         | Value                      |
|------------------|----------------------------|
| BLE Device Name   | `HAHA-NO-IKARI` *(default)* |
| Service UUID     | `55725ac1-066c-48b5-8700-2d9fb3603c5e` |
| Characteristic UUID | `69ddb59c-d601-4ea4-ba83-44f679a670ba` |

You can change the name and password by sending text commands over BLE.

## 🔑 BLE Commands

Once connected:

1. You’ll be asked to set:
   - Device name
   - Password
2. Password will be saved to SPIFFS
3. On next connection, send the password again to unlock

Send a newline-terminated password string (`\n`) from your BLE terminal to control the lock.

## 🔄 State Machine

- `gcSettingStatus = '0'` → Ask for name
- `'1'` → Ask for password
- `'2'` → Save settings
- `'5'` → Ready to unlock
- `'9'` → Waiting for password to toggle lock state

## 🔩 Servo Behavior

- Uses `correctAngle()` to calibrate physical range
- Opens at logical angle 90
- Closes at logical angle 0

You can customize servo pin or angle ranges in:

```cpp
myservo.attach(13, 545, 2500);


