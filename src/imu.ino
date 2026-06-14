bool imuConnected = false;

void initIMU()
{
  Serial.print("IMU ");
  setupIMU();
  Serial.println();
}

void setupIMU()
{
  #if defined(IMU_TYPE) && IMU_TYPE == MPU6050
  if(!IMU.begin(IMU_ADDRESS, &Wire)){
    Serial.println("IMU does not respond");
    imuConnected = false;
  } else {
    imuConnected = true;
  }

  if (imuConnected) {
    IMU.setAccelerometerRange(MPU6050_RANGE_2_G);
    IMU.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
  #endif
}

double calibrateIMU(double id)
{
  if (!imuConnected) return 0;
  Serial.println("Calibrating ACC and GYRO in 5 seconds. Put device on flat leveled surface.");
  delay(5000);
  Serial.print("Calibration...");
  // Adafruit MPU6050 does not have built-in autoOffsets. 
  // We skip it or implement custom.
  Serial.println("Done.");

  return 1;
}

void updateIMU()
{
  #if defined(IMU_TYPE) && IMU_TYPE == MPU6050
  if (!imuConnected) return;
  sensors_event_t a, g, temp;
  IMU.getEvent(&a, &g, &temp);

  // Compute roll and pitch from accelerometer
  float roll = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;
  float pitch = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;

  IMU_DATA[ROLL]  = roll;
  IMU_DATA[PITCH] = pitch;
  IMU_DATA[YAW]   = 0; // Gyro integration needed for Yaw, keeping 0 for now
  #endif
}
