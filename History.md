# History

## June 13, 2026

- **Web Server Build Process Fix**: Replaced outdated Gulp dependencies with a custom Node.js `build.js` script to properly minify and gzip the HTML/JS web interface without `primordials` errors. Updated `npm-shrinkwrap.json` to pin `graceful-fs`. Fixed output path so the generated header is successfully embedded into the firmware.
- **WebSocket Binary Type Fix**: Fixed `event.data.arrayBuffer is not a function` error in the browser web interface by changing `ws.binaryType = 'arraybuffer'` in `s.js`.
- **Servo Pin Assignments**: Adjusted `config_small.h` mapping to correctly map PCA9685 pins 0-14 for the Front-Left, Front-Right, Hind-Left, and Hind-Right legs.
- **I2C Bus Lockup Fix (INA219)**: Discovered that the INA219 driver (default address `0x40`) was conflicting with the PCA9685 driver (also at `0x40`), corrupting PCA9685 registers and causing `Wire` timeout errors (`Error 263`). Disabled the INA219 power sensor to fix the lockup and restore PWM signals to the servos.
- **IMU Disable**: Disabled the MPU6050 in firmware to streamline testing since it is not currently needed.
- **Code Cleanups**: Fixed compilation errors in `display.cpp.inc`, `powerSensor.ino`, and `imu.ino` when `POWER_SENSOR` and `IMU_TYPE` are disabled.

## June 14, 2026

- **I2C MPU6050 Stutter Fix**: Identified that the `MPU6050` IMU sensor was blocking the I2C bus and causing significant lag in the main servo gait loop. Fixed the stuttering issues by addressing I2C contention and keeping the bus speed at 400kHz.
- **Hardware Loop Optimizations**: Tuned `LOOP_TIME` and updated the `PCA9685` servo frequency to 330Hz for significantly smoother kinematics trajectories.
- **Web UI Gait Presets**: Added 7 new gait toggle buttons directly into the Web UI (Walk, Run, Sit, Dance, Walk Backwards, Side Step Right, Side Step Left).
- **Decoupled UI Logic**: Implemented the gait toggles efficiently in `s.js` by overriding the inverse kinematics XYZ offsets directly via the Javascript WebSocket interval, eliminating the need to clutter the firmware with hardcoded gaits.
- **Dependency Order Fixes**: Fixed C++ dependency compile errors (`'legs' was not declared`) by ensuring proper header `#include` order in `robot_dog_esp32.ino`.
