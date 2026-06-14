#include <math.h>

#include "libs/IK/geometry.h"
#include "libs/IK/leg.h"
#include "def.h"
#include "config.h"
#include "config_small.h"
#include "config_wifi.h"
#include "libs/IK/IK_simple.h"  // TODO this is for small dog only!!!
#include "libs/planner/planner.h"
#include "libs/balance/balance.h"
#include "libs/gait/gait.h"
#include "libs/HAL_body/HAL_body.h"

#include <EEPROM.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "web/index.html.gz.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "cli.h"
#include "subscription.h"

#include "libs/IK/IK_simple.cpp.inc"  // TODO this is for small dog only!!!
#include "libs/planner/planner.cpp.inc"
#include "libs/balance/balance.cpp.inc"
#include "libs/gait/gait.cpp.inc"
#include "libs/HAL_body/HAL_body.cpp.inc"

/**
 * Hardware libraries
 */
#if PWM_CONTROLLER_TYPE == PCA9685
  #include <Adafruit_PWMServoDriver.h>
#endif

#if PWM_CONTROLLER_TYPE == ESP32PWM
  #define USE_ESP32_TIMER_NO 3
  #include "ESP32_ISR_Servo.h"
#endif

#ifdef POWER_SENSOR
  float voltage_V = 0.0;
  float current_A = 0.0;

  #if POWER_SENSOR == INA219
    #include <INA219_WE.h>
    INA219_WE ina219;
  #endif
#endif

float IMU_DATA[3] = {0, 0, 0};
#if IMU_TYPE == MPU6050
Adafruit_MPU6050 IMU;
#endif

// run commands on diferent cores (FAST for main, SLOW for services)
bool runCommandFASTCore = false;
bool runCommandSLOWCore = false;
cliFunction cliFunctionFAST;
cliFunction cliFunctionSLOW;
double cliFunctionFASTVar = 0.0;
double cliFunctionSLOWVar = 0.0;

TaskHandle_t ServicesTask;

#if PWM_CONTROLLER_TYPE == PCA9685
  Adafruit_PWMServoDriver pwm;
#endif

unsigned long currentTime;
unsigned long previousTime;
unsigned long loopTime;

unsigned long serviceCurrentTime;

unsigned long servicePreviousTime;
unsigned long serviceLoopTime;

unsigned long serviceFastPreviousTime;
unsigned long serviceFastLoopTime;

bool HALEnabled = true;

// Gait
uint16_t ticksPerGaitItem    = 0;
uint16_t ticksToNextGaitItem = 0;
uint8_t  currentGait         = 0;
uint8_t  nextGait            = 0;
double   gaitItemProgress    = 0;
double   gaitProgress[]      = {0, 0, 0, 0};

transition bodyTransition;
transitionParameters bodyTransitionParams = {{0,0,0}, {0,0,0}, 0};

//Move
moveVector vector = {
  {0, 0, 0},
  {0, 0, 0}
};

//Failsafe
bool FS_FAIL = false;
uint16_t FS_WS_count = 0;

// HAL
figure body = {
  {  0,  0,  0},
  {  0,  0,  0}
};

IK ikLegLF(legs[LEGLF], body);
IK ikLegRF(legs[LEGRF], body);
IK ikLegLH(legs[LEGLH], body);
IK ikLegRH(legs[LEGRH], body);

/* We need predict future position of legs/body */
planner walkPlanner(
  vector,
  body,
  legs[LEGLF],
  legs[LEGRF],
  legs[LEGLH],
  legs[LEGRH]
);

/* and balance it someway */
balance bodyBalance(
  balanceOffset,
  body,
  legs[LEGLF],
  legs[LEGRF],
  legs[LEGLH],
  legs[LEGRH]
);

HAL_body bodyUpdate(vector, body, legs);

// WebServer
bool clientOnline = false;
int WiFiMode = AP_MODE;
IPAddress WiFiIP;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
uint8_t telemetryPackage[P_TELEMETRY_LEN];

// CLI
Stream *cliSerial;

// Subscriptions
bool subscriptionEnabled = false;
bool subscriptionBinary = false;

bool mainLoopReady = false;
bool serviceLoopReady = false;
SemaphoreHandle_t i2c_mutex;

String lastError1 = "";
String lastError2 = "";
bool isCalibrating = false;

// Firmware-level Gait Flags
bool isGaitPlaying = false;
uint8_t activeGaitMode = 0; // 1 = Walk

void logError(String err) {
  if (err == lastError1) return;
  lastError2 = lastError1;
  lastError1 = err;
  Serial.println(err);
}

#include "display.cpp.inc"

void setup()
{
  i2c_mutex = xSemaphoreCreateMutex();
  Serial.begin(SERIAL_BAUD);
  delay(100);

  initSettings();
  delay(100);
  
  initHAL();
  delay(100);
  
  initGait();
  delay(100);
  
  servicesSetup();
  
  xTaskCreatePinnedToCore(
    servicesLoop,   /* Task function. */
    "Services",     /* name of task. */
    100000,         /* Stack size of task */
    NULL,           /* parameter of the task */
    1,              /* priority of the task */
    &ServicesTask,  /* Task handle to keep track of created task */
    0);             /* pin task to core 0 */

  mainLoopReady = true;
}

/**
   Main loop for all major things
   Core 1
*/
void loop()
{
  currentTime = micros();
  if (mainLoopReady && serviceLoopReady) {
    if (currentTime - previousTime >= LOOP_TIME) {
      previousTime = currentTime;

      updateFailsafe();

      if (isGaitPlaying) {
        if (activeGaitMode == 1) { // Walk
          vector.move.y = 0.5;
        }
      }

      updateGait();
      
      if (xSemaphoreTake(i2c_mutex, portMAX_DELAY)) {
        updateIMU();
        updateHAL();
        doHAL();
        xSemaphoreGive(i2c_mutex);
      }

      FS_WS_count++;

      loopTime = micros() - currentTime;
      if (loopTime > LOOP_TIME) {
        logError("Slow Loop");
      }
    }
  } else {
    delay(1);
  }
}

/**
   Loop for service things, like CLI
   Core 0
*/
void servicesSetup() {
  cliSerial = &Serial;
  initCLI();
  initSubscription();
  Wire.begin();
  Wire.setClock(400000); // 400kHz I2C for faster servo updates and OLED drawing
  delay(100);

  Serial.println("Scanning I2C bus...");
  byte error, address;
  int nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) Serial.print("0");
      Serial.println(address,HEX);
      nDevices++;
    }
  }
  if (nDevices == 0) Serial.println("No I2C devices found\n");
  else Serial.println("done\n");

  initIMU();
  delay(100);

  initPowerSensor();
  delay(100);

  initWiFi();
  delay(100);
  
  initDisplay();
  delay(100);

  initWebServer();
  delay(100);

  serviceLoopReady = true;
}

void servicesLoop(void * pvParameters) {
  while(1) {
    if (mainLoopReady && serviceLoopReady) {
      serviceCurrentTime = micros();

    if (serviceCurrentTime - serviceFastPreviousTime >= SERVICE_FAST_LOOP_TIME) {
      serviceFastPreviousTime = serviceCurrentTime;
      
      runFASTCommand();
      doFASTSubscription();

      serviceFastLoopTime = micros() - serviceCurrentTime;
      if (serviceFastLoopTime > SERVICE_FAST_LOOP_TIME) {
        logError("Slow SFast L");
      }      
    }

    if (serviceCurrentTime - servicePreviousTime >= SERVICE_LOOP_TIME) {
      servicePreviousTime = serviceCurrentTime;

      updateWiFi();
      if (xSemaphoreTake(i2c_mutex, portMAX_DELAY)) {
        updatePower();
        updateDisplay();
        xSemaphoreGive(i2c_mutex);
      }
      runSLOWCommand();
      updateCLI();
      doSLOWSubscription();

      serviceLoopTime = micros() - serviceCurrentTime;  // this loop + service fast loop
      if (serviceLoopTime > SERVICE_LOOP_TIME) {
        logError("Slow SL L");
      }

      }
    }
    vTaskDelay(1);  // https://github.com/espressif/arduino-esp32/issues/595
  }
}

void runFASTCommand()
{
  if (runCommandFASTCore) {
    runCommandFASTCore = false;
    cliFunctionFAST(cliFunctionFASTVar);
  }
}

void runSLOWCommand()
{
  if (runCommandSLOWCore) {
    runCommandSLOWCore = false;
    cliFunctionSLOW(cliFunctionSLOWVar);
  }
}

/**
   TODO
    - calculate center of mass and use it for balance
    - make the queue of tasks by core, e.g. not just   cliFunctionFAST = runI2CScanFAST; but array/list with commands
    - i2c pwm broken, i2c in service loop

*/
