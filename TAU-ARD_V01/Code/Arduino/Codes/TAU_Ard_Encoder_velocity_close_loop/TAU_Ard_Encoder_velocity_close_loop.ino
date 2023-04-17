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

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

float dspeedRPM = 0;
void setup() {
  // initialize serial communication at 115200 bits per second:
  Serial.begin (115200);
  
  // initialize encoder, attache ISR functions
  pinMode(ENCODER_PINA, INPUT);
  pinMode(ENCODER_PINB, INPUT);
   // Attached interrupt to encoder pins
  attachInterrupt(digitalPinToInterrupt(ENCODER_PINA), encoderA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PINB), encoderB, CHANGE);

  Serial.print("RPM RPMFiltered");
}

void loop() {
  // print the string when a newline arrives:
  if (stringComplete) {
    //Serial.println(inputString);
    dspeedRPM = (float) inputString.toInt();
    // clear the string:
    inputString = "";
    stringComplete = false;
    
  }
  
  static long micro_tic = micros();
  // print encoder position
  long dt = micros() - micro_tic;
  float dt_f = dt / 1000000.0;
  micro_tic = micros();
  static float FspeedRPM = 0;
  float speedRPM = (float)encoderCounts/12/15*60/dt_f; // encoder pulses / pulses per rev / gear ratio * minute (60) / delta time (sec)
  encoderCounts = 0;
  FspeedRPM = FspeedRPM*0.9 + speedRPM*0.1; 
  // int linearPotentiometer = analogRead(LINEAR_POT);
  // control loop implementation
  static float i_err = 0;
  float err = dspeedRPM - FspeedRPM;
  i_err = i_err + err*dt_f;
  float motorCMD = err*2.0 + i_err*1.0;
  if (motorCMD >= 255) motorCMD = 255;
  if (motorCMD <= -255) motorCMD = -255;
  
  if (motorCMD <= -25){
    digitalWrite(PWM2_PIN, LOW);
    analogWrite(PWM1_PIN, (int)-motorCMD);
  } else if (motorCMD >= 25) {
    digitalWrite(PWM1_PIN, LOW);
    analogWrite(PWM2_PIN, (int)motorCMD);
  } else {
    digitalWrite(PWM2_PIN, LOW);
    digitalWrite(PWM1_PIN, LOW);
  }

    Serial.print (speedRPM);
    Serial.print (" ");
    Serial.println(FspeedRPM);
  delay(10);
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
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
