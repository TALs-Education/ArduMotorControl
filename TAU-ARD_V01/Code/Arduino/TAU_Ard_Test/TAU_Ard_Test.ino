/*
  TAU-Ard Test code
*/
// VL53L4CD Sparkfun library
#include <Wire.h>
#include "SparkFun_VL53L1X.h"

// PinOut:
#define LINEAR_POT A0
#define RED_LED 9
#define GREEN_LED 10
#define BLUE_LED 11

#define PWM1_PIN 6 // when jumper resistor is placed 4-->6 then switch to pin 6
#define PWM2_PIN 5

// loop defines
#define SAMPLE_DELAY 20 // sample time delay in milliseconds


// Variables
SFEVL53L1X distanceSensor;
unsigned int distance = 0;
unsigned long timeStamp = 0;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  // init io pins
  pinMode(LED_BUILTIN, OUTPUT); // arduino internal led
  pinMode(RED_LED, OUTPUT); // Red LED
  pinMode(GREEN_LED, OUTPUT); // Green LED
  pinMode(BLUE_LED, OUTPUT); // Blue LED

  pinMode(PWM1_PIN, OUTPUT); // Motor control PWM1
  pinMode(PWM2_PIN, OUTPUT); // Motor control PWM2

  // init motor to off
  digitalWrite(PWM1_PIN, LOW);
  digitalWrite(PWM2_PIN, LOW);

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
  Serial.println("slide[adc] distance[mm]");
}

// the loop routine runs over and over again forever:
void loop() {
  // simple loop delay implementation
  if ((millis() - timeStamp >= SAMPLE_DELAY) || (millis() < timeStamp)) {
    timeStamp = millis();
    // read linear potentiometer
    int linearPotentiometer = analogRead(LINEAR_POT);

    // read distance sensor:
    if (distanceSensor.checkForDataReady()) {
      byte rangeStatus = distanceSensor.getRangeStatus();
      distance = distanceSensor.getDistance(); //Get the result of the measurement from the sensor
      if (rangeStatus != 0) { // error detected
        // in Distance mode short we get error 4 (out of bounds) or 7 (wrap around) if the distance is greater than 1.3 meter.
        distance = rangeStatus;
      }
    }

    // print out the values:
    Serial.print(linearPotentiometer);
    Serial.print(" , ");
    Serial.println(distance);

    // visual effects:
    analogWrite(RED_LED, linearPotentiometer / 4);
    analogWrite(GREEN_LED, linearPotentiometer / 4);
    analogWrite(BLUE_LED, linearPotentiometer / 4);
    analogWrite(LED_BUILTIN, linearPotentiometer / 4);

    // set motor speed
    if (linearPotentiometer >= 525) {
      digitalWrite(PWM2_PIN, LOW);
      analogWrite(PWM1_PIN, (linearPotentiometer - 513) / 2);
    } else if (linearPotentiometer <= 500) {
      digitalWrite(PWM1_PIN, LOW);
      analogWrite(PWM2_PIN, (510 - linearPotentiometer) / 2);
    } else {
      digitalWrite(PWM2_PIN, LOW);
      digitalWrite(PWM1_PIN, LOW);
    }
  }
}
