//Encoder Example

// Define Pins
#define ENCODER_PINA 2
#define ENCODER_PINB 3
#define LINEAR_POT A0
#define PWM1_PIN 6 // when jumper resistor is placed 4-->6 then switch to pin 6
#define PWM2_PIN 5
// encoder variables
volatile int encoderCounts = 0;

// Encoder ISR functions - Interupt Service Routine
void encoderA();
void encoderB();


void setup() {
  // initialize serial communication at 115200 bits per second:
  Serial.begin (115200);
  
  // initialize encoder, attache ISR functions
  pinMode(ENCODER_PINA, INPUT);
  pinMode(ENCODER_PINB, INPUT);
   // Attached interrupt to encoder pins
  attachInterrupt(digitalPinToInterrupt(ENCODER_PINA), encoderA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PINB), encoderB, CHANGE);

  Serial.print("Encoder_Value");
}

void loop() {
  static long micro_tic = micros();
  // print encoder position
  long dt = micros() - micro_tic;
  float dt_f = dt / 1000000.0;
  micro_tic = micros();
  float speedRPM = (float)encoderCounts/12/30*60/dt_f; // encoder pulses / pulses per rev / gear ratio * minute (60) / delta time (sec)
  encoderCounts = 0;
  
  int linearPotentiometer = analogRead(LINEAR_POT);
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

    Serial.println(speedRPM);
  delay(100);
}

// EncoderA ISR
void encoderA() {
  // look for a low-to-high on channel B
  if (digitalRead(ENCODER_PINA) == HIGH) {
    // check channel A to see which way encoder is turning
    digitalRead(ENCODER_PINB) ? encoderCounts++ : encoderCounts--;    
  }else{
    // check channel A to see which way encoder is turning
    digitalRead(ENCODER_PINB) ? encoderCounts-- : encoderCounts++; 
  } 
} // End EncoderA ISR

// EncoderB ISR
void encoderB() {
  // look for a low-to-high on channel B
  if (digitalRead(ENCODER_PINB) == HIGH) {
    // check channel A to see which way encoder is turning
    digitalRead(ENCODER_PINA) ? encoderCounts-- : encoderCounts++;    
  }else{
    // check channel A to see which way encoder is turning
    digitalRead(ENCODER_PINA) ? encoderCounts++ : encoderCounts--; 
  }
} // End EncoderB ISR
