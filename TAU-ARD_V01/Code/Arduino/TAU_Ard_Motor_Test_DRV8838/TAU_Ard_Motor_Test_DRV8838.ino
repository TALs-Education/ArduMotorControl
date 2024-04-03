/*
  TAU-Ard Motor Test code
*/
// VL53L4CD Sparkfun library
#include <Wire.h>
#include "SparkFun_VL53L1X.h"
#include "ArduMotoControl.h"

// PinOut:
#define LINEAR_POT A0
#define RED_LED 9
#define GREEN_LED 10
#define BLUE_LED 11

// loop defines
#define DISTANCE_SAMPLE_DELAY 20 // sample time delay in milliseconds

// Variables
SFEVL53L1X distanceSensor;
float distance = 0;
float linearPotentiometer = 0;

///////////////////
// Motor control //
///////////////////

// define constants
#define DT_CONTROL_MILLIS 10  // Control DT suggested range 5-50 mSec --> 200-20 Hz
#define GEAR_RATIO 30.0     // Motor gear ratio
#define TIMEOUT 30            // Experiment length in seconds

// loop start time variable
unsigned long startTime = 0;
// timeout flag
boolean timeOutFlag = 0;

// Control Variables
float encoderRADSdt = 0;
float encoderRADSPulses = 0;
float motorRPMdt = 0;
float motorRPMPulses = 0;
float desiredCMD = 0;
float motorCMD = 0;

// control loop function
void MotorControlLoop(void);
// Control function
float MotorControl(float desiredCMD);

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin (115200);

  // intinitalize encoder
  InitArduMotorControl();

  // init io pins
  pinMode(LED_BUILTIN, OUTPUT); // arduino internal led
  pinMode(RED_LED, OUTPUT); // Red LED
  pinMode(GREEN_LED, OUTPUT); // Green LED
  pinMode(BLUE_LED, OUTPUT); // Blue LED
  // init i2c
  Wire.begin();
  // initialize distance sensor
  if (distanceSensor.begin() != 0) //Begin returns 0 on a good init
  {
    while (1) {
      Serial.println("Sensor failed to begin");
      delay(1000);
    }
  }
  // Short mode max distance is limited to 1.3 m
  distanceSensor.setDistanceModeShort();
  //distanceSensor.setDistanceModeLong(); // default
  //  * The minimum timing budget is 20 ms for the short distance mode and 33 ms for the medium and long distance modes.
  //  * Predefined values = 15, 20, 33, 50, 100(default), 200, 500.
  distanceSensor.setTimingBudgetInMs(20);
  // measure periodically. Intermeasurement period must be >/= timing budget.
  distanceSensor.setIntermeasurementPeriod(20); // 20 - yields about 50 samples per second
  distanceSensor.startRanging(); // Start once

  // serial plotter legend
  Serial.println("DesireCMD, MotorCMD, Error, RPMdt, slide[%], distance[mm], MCUload[%]");

  // after finish initializing start counter for running the main code.
  startTime = millis();
}

// the loop routine runs over and over again forever:
void loop() {
  // run loop for TIMEOUT seconds
  if (!timeOutFlag) {
    static unsigned long millisTick = millis();
    if (millisTick >= TIMEOUT * 1000 + startTime) timeOutFlag = 1;

    // control loop software implementation
    if ((millis() - millisTick) >= DT_CONTROL_MILLIS) {
      millisTick = millis();
      unsigned long microsTick = micros(); // simple loop cpu consumption measurement
      digitalWrite(LED_BUILTIN, HIGH);

      // update Motor RPM variables
      encoderRADSdt = ReadRadS(); // rad/s measured from dt between pulses with averaging
      encoderRADSPulses = readRadSEncoder(); // rad/s measured from number of pulses over control period
      motorRPMdt = encoderRADSdt / GEAR_RATIO * RADS_RPM;
      motorRPMPulses = encoderRADSPulses / GEAR_RATIO * RADS_RPM;

      // motor control loop function call
      MotorControlLoop();

      // read linear potentiometer convert to % 0-1
      linearPotentiometer = float(analogRead(LINEAR_POT)) / 1023.0;

      // visual effects:
      analogWrite(RED_LED, linearPotentiometer*255);
      analogWrite(GREEN_LED, linearPotentiometer*255);
      analogWrite(BLUE_LED, linearPotentiometer*255);
      analogWrite(LED_BUILTIN, linearPotentiometer*255);

      // send values to serial plotter
      Serial.print(desiredCMD);
      Serial.print(" , ");
      Serial.print(motorCMD * 100); // scaled to 100% to be visible on plot
      Serial.print(" , ");
      Serial.print(desiredCMD - motorRPMdt);
      Serial.print(" , ");
      Serial.print(motorRPMdt); // motorRPMPulses
      Serial.print(" , ");
      Serial.print(linearPotentiometer * 100); // scaled to 100% to be visible on plot
      Serial.print(" , ");
      Serial.print(distance); // mm

      //debug code consumption better alternativelyy measure d13 duty cycle / frequency
      Serial.print(" , ");
      Serial.println((micros() - microsTick) / 100); // value in % for control loop
      digitalWrite(LED_BUILTIN, LOW);
    }

    static unsigned long distMillisTick = millis();
    // distance sensor polling
    if (millis() - distMillisTick >= DISTANCE_SAMPLE_DELAY) {
      distMillisTick = millis();
      // read distance sensor:
      if (distanceSensor.checkForDataReady()) {
        byte rangeStatus = distanceSensor.getRangeStatus();
        distance = distanceSensor.getDistance(); //Get the result of the measurement from the sensor
        if (rangeStatus != 0) { // error detected
          // in Distance mode short we get error 4 (out of bounds) or 7 (wrap around) if the distance is greater than 1.3 meter.
          distance = rangeStatus;
        }
      }
    }
  } else {
    // turn off motor
    motorComand(0);
  }
}

// control loop function
void MotorControlLoop(void) {
  // Command profile
  // float desiredCMD = 500 * StepCMD(2); // step command, input period seconds
  // float desiredCMD = 100 * RampCMD(2); // Ramp command, input period seconds
  //desiredCMD = 600 * (SineCMD(0.25)-0.5);  // Sine command, input Hz
  desiredCMD = (300 * linearPotentiometer-150)*2; // potentiometer value
  
  // motor control function
  motorCMD = MotorControl(desiredCMD);

  // open loop
  //motorCMD = 0.75*StepCMD(0.5);
  
  // potentiometer
  //motorCMD = linearPotentiometer;
  
  // update motor, values range of [-1..1]
  motorComand(motorCMD);
}

// control loop function
float MotorControl(float desiredCMD) {
  // initialize variables
  static float dt = DT_CONTROL_MILLIS / 1000.0;
  static float ciEr = 0; // integral error
  static float cdEr = 0; // differamtial error
  static float cEr  = 0; // current error
  static float lEr  = 0; // last error

  // control coefficients
  float kp = 0.0025 ;
  float ki = 0.015 ;
  float kd = 0;

  // update error
  lEr = cEr;
  //float cEr = desiredCMD - motorRPMPulses;
  cEr = desiredCMD - motorRPMdt;
  ciEr = ciEr + cEr * dt;
  cdEr = (cEr - lEr) / dt;

  // clip integrator error
  if (ciEr * ki > 2.5) ciEr = 2.5 / ki;
  if (ciEr * ki < -2.5) ciEr = -2.5 / ki;

  // update control command
  float motorCMD = kp * cEr + ki * ciEr + kd * cdEr;

  // Optional - As it is clipped in library.
  // clip motor command
  // if (motorCMD > 1) motorCMD = 1;
  // if (motorCMD < -1) motorCMD = -1;

  // Optional
  // dead zone + preamp
  if (abs(motorCMD) < 0.05) motorCMD = 0; // dead zone
  //if ((motorCMD >= 0.05) && (motorCMD < 0.075)) motorCMD = 0.075; // preamp
  //if ((motorCMD <= -0.05) && (motorCMD > -0.075)) motorCMD = -0.075; // preamp

  return motorCMD;
}
// in case someone sent any char start the loop again
void serialEvent() {
  while (Serial.available()) {
    //clear buffer and reset timeout flag
    char inChar = (char)Serial.read();
    startTime = millis();
    timeOutFlag = 0;
  }
}
