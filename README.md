# ESP32 Robot Dog

## Background

The whole codebase is originally based on [SovGVD/esp32-robot-dog-code](https://github.com/SovGVD/esp32-robot-dog-code), but due to severe build errors and outdated dependencies when compiling it in 2026, this repository was created to fix those issues and maintain the original code. Everything else has been modified either manually or through AI assistance.

This project is aimed at building a quadruped robot dog driven by an ESP32 microcontroller, utilizing a PCA9685 PWM servo driver to control its 12 joints. The project features an embedded Web UI served over WiFi to control the robot's movements remotely.

During the development and testing phase, we encountered and fixed several significant issues:
1. **Web Build Pipeline**: The Node.js build scripts were failing due to outdated dependencies (`primordials` error). We fixed this by rewriting the pipeline to properly minify, gzip, and embed the web assets directly into a C++ header file.
2. **WebSockets**: The web interface was failing to parse incoming binary telemetry from the ESP32 due to incorrect WebSocket binary type settings.
3. **I2C Bus Lockups**: The PCA9685 servo driver was not receiving PWM commands. We discovered that a dormant INA219 power sensor driver in the code was sharing the same default I2C address (`0x40`) as the PCA9685, causing severe hardware lockups and disabling the servos entirely.

## Bill of Materials (BOM)

| Component | Description | Link |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32 DEVKIT V1 - CH340C | [Shopee Link](https://shopee.ph/ESP32-KIT-ESP32-Development-Board-ESP32-WROOM-32-with-WiFi-and-Bluetooth-i.1021339444.41157297633) |
| **Servo Driver** | PCA9685 16 Channel 12-bit PWM | [Shopee Link](https://shopee.ph/PCA9685-16-Channel-12-bit-PWM-Servo-Driver-i.1090440049.24825601112) |
| **IMU** | MPU6050 3 Axis Gyro/Accelerometer | [Shopee Link](https://shopee.ph/Original-GY-521-MPU-6050-MPU6050-3-Axis-Analog-Gyroscope-Sensors-3-Axis-Accelerometer-Module-IIC-I2C-i.1090440049.26936261122) |
| **Servos** | 12x MG90S Metal Gear Servos (180 degrees) | [Shopee Link](https://shopee.ph/ENGLAB%E2%98%85Micro-Servo-Motor-SG90-MG90S-Servo-Metal-Gear-Servo-Motor-Servo-For-RC-Robot-Ship-Toy-i.1021339444.22480954645) |
| **Display** | OLED I2C Module (0.91/0.96/1.3 Inch) | [Shopee Link](https://shopee.ph/Circuitrocks-Oled-I2c-Module-Lcd-Display-Screen-0.91-0.96-1.3-Inch-Blue-Communicate-For-Arduino-i.20469516.827064213) |
| **Power Supply** | 300W 20A DC-DC Buck Converter Step Down Module | [Shopee Link](https://shopee.ph/300W-20A-DC-DC-Buck-Converter-Step-Down-Module-Constant-Current-LED-Driver-Power-Step-Down-Voltage-Module-Electrolytic-Capacitor-i.580325202.14789230069) |
| **Chassis** | 3D Printed Parts (96.24g, 4h 37m print time) | [Thingiverse Link](https://www.thingiverse.com/thing:4822059) |

## Recent Changes (June 13, 2026)

- **Web Server Build Process Fix**: Replaced outdated Gulp dependencies with a custom Node.js `build.js` script to properly minify and gzip the HTML/JS web interface without `primordials` errors. Updated `npm-shrinkwrap.json` to pin `graceful-fs`. Fixed output path so the generated header is successfully embedded into the firmware.
- **WebSocket Binary Type Fix**: Fixed `event.data.arrayBuffer is not a function` error in the browser web interface by changing `ws.binaryType = 'arraybuffer'` in `s.js`.
- **Servo Pin Assignments**: Adjusted `config_small.h` mapping to correctly map PCA9685 pins 0-14 for the Front-Left, Front-Right, Hind-Left, and Hind-Right legs.
- **I2C Bus Lockup Fix (INA219)**: Discovered that the INA219 driver (default address `0x40`) was conflicting with the PCA9685 driver (also at `0x40`), corrupting PCA9685 registers and causing `Wire` timeout errors (`Error 263`). Disabled the INA219 power sensor to fix the lockup and restore PWM signals to the servos.
- **IMU Disable**: Disabled the MPU6050 in firmware to streamline testing since it is not currently needed.
- **Code Cleanups**: Fixed compilation errors in `display.cpp.inc`, `powerSensor.ino`, and `imu.ino` when `POWER_SENSOR` and `IMU_TYPE` are disabled.

Please refer to `History.md` for a full breakdown of the revisions.
