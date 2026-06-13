#if PWM_CONTROLLER_TYPE == PCA9685

double pwmTicksPerUs = 0;

void initServoHAL() {
  pwm = Adafruit_PWMServoDriver();
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);  // The int.osc. is closer to 27MHz
  pwm.setPWMFreq(SERVO_FREQ);  // This is the maximum PWM frequency of servo
  
  // Cache the ticks-per-microsecond multiplier to avoid doing an I2C read inside angleToPulse
  double pulselength = 1000000.0;
  uint16_t prescale = pwm.readPrescale();
  prescale += 1;
  pulselength *= prescale;
  pulselength /= pwm.getOscillatorFrequency();
  pwmTicksPerUs = 1.0 / pulselength;
}

uint16_t angleToPulse(double angleRad) {
  // Map angle expected between 0 and PI to pulse width in microseconds
  double pulse_us = mapf(angleRad, 0, M_PI, SERVO_MIN, SERVO_MAX);
  return pulse_us * pwmTicksPerUs;
}

void setLegPWM(leg &_leg)
{
  pwm.setPWM(_leg.hal.pin.alpha,  0,  angleToPulse(limitServoAngle(getHALAngle(_leg.angle.alpha, _leg.hal.mid.alpha, _leg.hal.trim.alpha, _leg.hal.ratio.alpha, _leg.inverse.alpha))));
  pwm.setPWM(_leg.hal.pin.beta,   0,  angleToPulse(limitServoAngle(getHALAngle(_leg.angle.beta,  _leg.hal.mid.beta,  _leg.hal.trim.beta,  _leg.hal.ratio.beta,  _leg.inverse.beta ))));
  pwm.setPWM(_leg.hal.pin.gamma,  0,  angleToPulse(limitServoAngle(getHALAngle(_leg.angle.gamma, _leg.hal.mid.gamma, _leg.hal.trim.gamma, _leg.hal.ratio.gamma, _leg.inverse.gamma))));
}

void runServoCalibrate(leg &_leg)
{
  uint16_t pulse = angleToPulse(M_PI_2);
  pwm.setPWM(_leg.hal.pin.alpha, 0, pulse);
  pwm.setPWM(_leg.hal.pin.beta,  0, pulse);
  pwm.setPWM(_leg.hal.pin.gamma, 0, pulse);
}


#endif
