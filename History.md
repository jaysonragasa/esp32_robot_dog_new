# History

## June 13, 2026

- **Web Server Build Process Fix**: Replaced outdated Gulp dependencies with a custom Node.js `build.js` script to properly minify and gzip the HTML/JS web interface without `primordials` errors. Updated `npm-shrinkwrap.json` to pin `graceful-fs`. Fixed output path so the generated header is successfully embedded into the firmware.
- **WebSocket Binary Type Fix**: Fixed `event.data.arrayBuffer is not a function` error in the browser web interface by changing `ws.binaryType = 'arraybuffer'` in `s.js`.
- **Servo Pin Assignments**: Adjusted `config_small.h` mapping to correctly map PCA9685 pins 0-14 for the Front-Left, Front-Right, Hind-Left, and Hind-Right legs.
- **I2C Bus Lockup Fix (INA219)**: Discovered that the INA219 driver (default address `0x40`) was conflicting with the PCA9685 driver (also at `0x40`), corrupting PCA9685 registers and causing `Wire` timeout errors (`Error 263`). Disabled the INA219 power sensor to fix the lockup and restore PWM signals to the servos.
- **IMU Disable**: Disabled the MPU6050 in firmware to streamline testing since it is not currently needed.
- **Code Cleanups**: Fixed compilation errors in `display.cpp.inc`, `powerSensor.ino`, and `imu.ino` when `POWER_SENSOR` and `IMU_TYPE` are disabled.
