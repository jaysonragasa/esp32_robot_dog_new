# ESP32 Robot Dog
![alt text](https://github.com/jaysonragasa/esp32_robot_dog_new/blob/main/images/20260614_181441.jpg?raw=true)

## Background

The whole codebase is originally based on [SovGVD/esp32-robot-dog-code](https://github.com/SovGVD/esp32-robot-dog-code), but due to severe build errors and outdated dependencies when compiling it in 2026, this repository was created to fix those issues and maintain the original code. Everything else has been modified either manually or through AI assistance.

This project is aimed at building a quadruped robot dog driven by an ESP32 microcontroller, utilizing a PCA9685 PWM servo driver to control its 12 joints. The project features an embedded Web UI served over WiFi to control the robot's movements remotely.

**Hardware Architecture & Safety Overhaul**: In the original Instructables guide ([ESP32 Small Robot Dog](https://www.instructables.com/ESP32-Small-Robot-Dog/)) that this codebase is based on, the developer used the ESP32 logic pins to directly drive all 12 servos, and powered them using 3 Mini360 buck converters. 

That approach is inherently dangerous and unstable. A single MG90S servo can draw upwards of 700mA+ under load or stall conditions. This means 12 servos could theoretically draw over 8 Amps simultaneously! Three small Mini360 buck converters (which are typically rated for 1.8A continuous) are vastly insufficient for this kind of load. Attempting to route this kind of current will result in severe voltage drops, brownouts, erratic behavior, overheating, and can permanently fry the microcontroller. 

To resolve this, our build fundamentally revamps the hardware architecture by introducing:
1. **A dedicated 300W 20A DC-DC Buck Converter** to safely provide the massive amperage required by 12 servos moving at once. This completely eliminates the bottleneck of using multiple small, underpowered converters.
2. **A PCA9685 16-Channel PWM Servo Driver** to offload the PWM signal generation from the ESP32 logic pins. This ensures rock-solid signal timing and keeps the high-current servo power lines safely isolated from the sensitive microcontroller circuitry.
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

![alt text](https://github.com/jaysonragasa/esp32_robot_dog_new/blob/main/images/20260614_181546.jpg?raw=true)

## Recent Changes (June 14, 2026)

- **I2C MPU6050 Stutter Fix**: Identified that the `MPU6050` IMU sensor was blocking the I2C bus and causing significant lag in the main servo gait loop. Fixed the stuttering issues by addressing I2C contention and keeping the bus speed at 400kHz.
- **Hardware Loop Optimizations**: Tuned `LOOP_TIME` and updated the `PCA9685` servo frequency to 330Hz for significantly smoother kinematics trajectories.
- **Web UI Gait Presets**: Added 7 new gait toggle buttons directly into the Web UI (Walk, Run, Sit, Dance, Walk Backwards, Side Step Right, Side Step Left).
- **Decoupled UI Logic**: Implemented the gait toggles efficiently in `s.js` by overriding the inverse kinematics XYZ offsets directly via the Javascript WebSocket interval, eliminating the need to clutter the firmware with hardcoded gaits.
- **Dependency Order Fixes**: Fixed C++ dependency compile errors (`'legs' was not declared`) by ensuring proper header `#include` order in `robot_dog_esp32.ino`.

Please refer to `History.md` for a full breakdown of the revisions.

![alt text](https://github.com/jaysonragasa/esp32_robot_dog_new/blob/main/images/20260614_182707.jpg?raw=true)
