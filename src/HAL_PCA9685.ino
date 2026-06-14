#if PWM_CONTROLLER_TYPE == PCA9685

double pwmTicksPerUs = 0;

void initServoHAL() {
  pwm = Adafruit_PWMServoDriver();
  pwm.begin();
  pwm.setOscillatorFrequency(27000000); // The int.osc. is closer to 27MHz
  pwm.setPWMFreq(SERVO_FREQ); // This is the maximum PWM frequency of servo

  // Cache the ticks-per-microsecond multiplier to avoid doing an I2C read
  // inside angleToPulse
  double pulselength = 1000000.0;
  uint16_t prescale = pwm.readPrescale();
  prescale += 1;
  pulselength *= prescale;
  pulselength /= pwm.getOscillatorFrequency();
  pwmTicksPerUs = 1.0 / pulselength;
}

uint16_t angleToPulse(double angleRad) {
  if (angleRad < 0)
    return 0; // Catch invalid angle flag from limitServoAngle
  // Map angle expected between 0 and PI to pulse width in microseconds
  double pulse_us = SERVO_MIN + (angleRad / M_PI) * (SERVO_MAX - SERVO_MIN);
  return pulse_us * pwmTicksPerUs;
  // return pulse_us;
}

void setPWMIfChanged(uint8_t pin, uint16_t pulse) {
  if (pulse == 0)
    return; // Prevent shutting off servo due to IK math NaN
  pwm.setPWM(pin, 0, pulse);
}

void setLegPWM(leg &_leg) {
  setPWMIfChanged(_leg.hal.pin.alpha,
                  angleToPulse(limitServoAngle(getHALAngle(
                      _leg.angle.alpha, _leg.hal.mid.alpha, _leg.hal.trim.alpha,
                      _leg.hal.ratio.alpha, _leg.inverse.alpha))));
  setPWMIfChanged(_leg.hal.pin.beta,
                  angleToPulse(limitServoAngle(getHALAngle(
                      _leg.angle.beta, _leg.hal.mid.beta, _leg.hal.trim.beta,
                      _leg.hal.ratio.beta, _leg.inverse.beta))));
  setPWMIfChanged(_leg.hal.pin.gamma,
                  angleToPulse(limitServoAngle(getHALAngle(
                      _leg.angle.gamma, _leg.hal.mid.gamma, _leg.hal.trim.gamma,
                      _leg.hal.ratio.gamma, _leg.inverse.gamma))));
}

void runServoCalibrate(leg &_leg) {
  uint16_t pulse = angleToPulse(M_PI_2);
  setPWMIfChanged(_leg.hal.pin.alpha, pulse);
  setPWMIfChanged(_leg.hal.pin.beta, pulse);
  setPWMIfChanged(_leg.hal.pin.gamma, pulse);
}

#endif
