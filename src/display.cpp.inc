#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET,
                         400000,   // clkDuring: push the framebuffer at 400 kHz
                         400000);  // clkAfter: leave the bus at 400 kHz so servo
                                   // writes (PCA9685) stay fast too. Default would
                                   // reset the whole bus back to 100 kHz here.

bool displayConnected = false;
unsigned long lastDisplayUpdate = 0;

double getPowerSensorVoltage();
double getPowerSensorCurrent();

void initDisplay() {
  Serial.print("Display ");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed or not found");
    displayConnected = false;
  } else {
    Serial.println("SSD1306 OK");
    displayConnected = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Booting robot...");
    display.display();
  }
}

void updateDisplay() {
  if (!displayConnected) return;

  unsigned long now = millis();
  if (now - lastDisplayUpdate < 500) {
    return; // update at 2 Hz max
  }
  lastDisplayUpdate = now;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Print WiFi IP
  display.print("IP: ");
  if (WiFiMode == AP_MODE) {
    display.println(WiFi.softAPIP());
  } else {
    display.println(WiFi.localIP());
  }

  // Print Power data
  #if defined(POWER_SENSOR) && POWER_SENSOR == INA219
  display.print("V: ");
  display.print(getPowerSensorVoltage(), 2);
  display.print("V I: ");
  display.print(getPowerSensorCurrent(), 2);
  display.println("A");
  #endif

  // Print IMU Roll/Pitch
  #if defined(IMU_TYPE) && IMU_TYPE == MPU6050
  display.print("R: ");
  display.print(IMU_DATA[ROLL], 1);
  display.print(" P: ");
  display.println(IMU_DATA[PITCH], 1);
  #endif

  // Print Loop Time
  extern unsigned long loopTime;
  extern unsigned long i2cWaitTime;
  display.print("Loop(us): ");
  display.println(loopTime);
  display.print("Wait(us): ");
  display.println(i2cWaitTime);

  // Print Calibration Mode
  extern bool isCalibrating;
  if (isCalibrating) {
    display.println("MODE: CALIBRATION");
  }

  // Print recent errors
  extern String lastError1;
  extern String lastError2;
  if (lastError1.length() > 0) {
    display.print("E1: ");
    display.println(lastError1);
  }
  if (lastError2.length() > 0) {
    display.print("E2: ");
    display.println(lastError2);
  }

  display.display();
}
