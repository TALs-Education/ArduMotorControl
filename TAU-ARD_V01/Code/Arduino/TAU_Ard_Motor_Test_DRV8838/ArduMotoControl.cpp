/* ArduMotoControl
*/

#include "Arduino.h"
#include "ArduMotoControl.h"

// encoder variables
volatile int encoderCounts = 0;
volatile unsigned long lastMicrosENCTick = 0;              // initilize with inf value
volatile unsigned long encoderDTmicros = 10000000;         // initilize with inf value
volatile unsigned long encoderDTmicrosFiltered = 10000000; // initilize with inf value
volatile int rotationDir = 1;


// motion profiles
// getnerate step commands 0,1,0,1... with a set period in secodns
float StepCMD(float stepPeriod){
  unsigned long stepPeriodMillis = (unsigned long)(stepPeriod*1000);
  float stepCmdValue = 0;
  if ((millis() % (stepPeriodMillis * 2)) >= stepPeriodMillis) stepCmdValue = 1;
  return stepCmdValue;
}

// generate ramp command with a set period in secodns
float RampCMD(float rampPeriod){
  unsigned long stepPeriodMillis = (rampPeriod * 1000);
  float rampCmdValue = 0;
  unsigned long rampMillis = millis() % stepPeriodMillis;
  rampCmdValue = float(rampMillis) / (rampPeriod * 1000);
  return rampCmdValue;
}

// generate sine wave [0..1]
float SineCMD(float f_Hz){
   float w_rads = f_Hz * TWO_PI;
   float sineWave = (sin(w_rads*float(millis())/1000.0)+1)/2;
   return sineWave;
}

// motor command update
void motorComand(float cntrlValue){
  // set constrains
  if (cntrlValue > 1) cntrlValue = 1;
  if (cntrlValue < -1) cntrlValue =-1;
  // update pwm & direction
  if (cntrlValue >= 0){
    digitalWrite(PHASE_PIN,LOW);
    analogWrite(PWM_PIN,int(cntrlValue * 255));
  }else{
    digitalWrite(PHASE_PIN,HIGH);
    analogWrite(PWM_PIN,int(-cntrlValue * 255));
  }  
}

// initialize encoder, attache ISR functions
void InitArduMotorControl(){
  
  // initialize
  pinMode(ENCODER_PINA, INPUT);
  pinMode(ENCODER_PINB, INPUT);

  pinMode(PWM_PIN, OUTPUT); // Motor control PWM
  pinMode(PHASE_PIN, OUTPUT); // Motor control PHASE
  
  // init motor to off
  digitalWrite(PWM_PIN, LOW);
  digitalWrite(PHASE_PIN, LOW);
  
  // Attached interrupt to encoder pins
  attachInterrupt(digitalPinToInterrupt(ENCODER_PINA), encoderA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PINB), encoderB, CHANGE);
}

// conrol functions

// finalized read encoder functions based on averaging time between pulses
// read encoder RPM
float ReadRPM(){
    // calculate RPM from time between pulses
    if (encoderDTmicrosFiltered == 0) encoderDTmicrosFiltered = 1;
    // check if the motor has stopped
    unsigned long calculatedEncoderDT = encoderDTmicrosFiltered;
    if ((micros() - lastMicrosENCTick) > MINIMUM_ENCDT) { // analyze why its not perfect, 6 RPM MINIMUM_ENCDT 20000
      calculatedEncoderDT += (micros() - lastMicrosENCTick); // time from last pulse and the interval of the privious pulse
    }
    // float rpmEncoder = rotationDir * 60 * (1000000 / (float(encoderDTmicros) * ENCODER_PPR); // dt in microseconds with 3 pulses per revolution
    float rpmEncoder = rotationDir * ENCDT_RPM / float(calculatedEncoderDT); 
    return rpmEncoder;
}

// read encoder rad/s
float ReadRadS(){
    // calculate RadS from time between pulses
    if (encoderDTmicrosFiltered == 0) encoderDTmicrosFiltered = 1;
    // check if the motor has stopped
    unsigned long calculatedEncoderDT = encoderDTmicrosFiltered;
    if ((micros() - lastMicrosENCTick) > MINIMUM_ENCDT) { 
      calculatedEncoderDT += (micros() - lastMicrosENCTick); // time from last pulse and the interval of the privious pulse
    }
    // float rpmEncoder = rotationDir * (1000000 / (float(encoderDTmicros) * ENCODER_PPR) * TWO_PI); // dt in microseconds with 3 pulses per revolution
    float rpmEncoder = rotationDir * ENCDT_RADS / float(calculatedEncoderDT);
    return rpmEncoder;
}

// RPM measured from number of pulses between function calls
float readRPMEncoder(){
    static unsigned long lastMillisEnc = 0;
    static unsigned int lastEncoderPos = 0;
    static unsigned int encoderPos = 0;
    unsigned long dtMillisEnc = (millis() - lastMillisEnc);
    if (dtMillisEnc == 0) dtMillisEnc = 1;
    lastMillisEnc = millis(); 
    lastEncoderPos = encoderPos;
    encoderPos = encoderCounts;
    int encoderPulses = encoderPos - lastEncoderPos;
    // float rpmEncoder = 60 * (((float(encoderPulses)) * 1000 / float(dtMillisEnc)) / ENCODER_CPR); // 12 pulses per revolution
    float rpmEncoder =  ENCPULSES_RPM * (float(encoderPulses) / float(dtMillisEnc));
    return rpmEncoder;
} 

// RPM measured from number of pulses between function calls
float readRadSEncoder(){
    static unsigned long lastMillisEnc = 0;
    static unsigned int lastEncoderPos = 0;
    static unsigned int encoderPos = 0;
    unsigned long dtMillisEnc = (millis() - lastMillisEnc);
    if (dtMillisEnc == 0) dtMillisEnc = 1;
    lastMillisEnc = millis(); 
    lastEncoderPos = encoderPos;
    encoderPos = encoderCounts;
    int encoderPulses = encoderPos - lastEncoderPos;
    //float RadSEncoder = (((float(encoderPulses)) * 1000 / float(dtMillisEnc)) / ENCODER_CPR * TWO_PI); // 12 pulses per revolution
    float RadSEncoder = ENCPULSES_RADS * (float(encoderPulses) / float(dtMillisEnc));
    return RadSEncoder;
}

// RPM measured from dt between pulses more precise at lower speeds       
float readRPMdtEncoder(){
    // calculate RPM from time between pulses
    if (encoderDTmicros == 0) encoderDTmicros = 1;
    // check if the motor has stopped
    unsigned long calculatedEncoderDT = encoderDTmicros;
    if ((micros() - lastMicrosENCTick) > MINIMUM_ENCDT) { 
      calculatedEncoderDT += (micros() - lastMicrosENCTick); // time from last pulse and the interval of the privious pulse
    }
    // float rpmEncoder = rotationDir * 60 * (1000000 / (float(encoderDTmicros) * ENCODER_PPR); // dt in microseconds with 3 pulses per revolution
    float rpmEncoder = rotationDir * ENCDT_RPM / float(calculatedEncoderDT); 
    return rpmEncoder;
} 
// RPM measured from dt between pulses more precise at lower speeds 
float readRadSdtEncoder(){
    // calculate RPM from time between pulses
    if (encoderDTmicros == 0) encoderDTmicros = 1;
    // check if the motor has stopped
    unsigned long calculatedEncoderDT = encoderDTmicros;
    if ((micros() - lastMicrosENCTick) > MINIMUM_ENCDT) { 
      calculatedEncoderDT += (micros() - lastMicrosENCTick); // time from last pulse and the interval of the privious pulse
    }else{
      calculatedEncoderDT = encoderDTmicrosFiltered;
    }
    // float rpmEncoder = rotationDir * (1000000 / (float(encoderDTmicros) * ENCODER_PPR) * TWO_PI); // dt in microseconds with 3 pulses per revolution
    float rpmEncoder = rotationDir * ENCDT_RADS / float(calculatedEncoderDT);
    return rpmEncoder;
}

// EncoderA ISR
void encoderA() {
  #ifndef FILTER_PPR
    // sample time between pulses
    EncoderDTMicrosSample();
  #endif
  // look for a low-to-high on channel A
  if (digitalRead(ENCODER_PINA) == HIGH) {
    #ifdef FILTER_PPR
      // lower performance option - sample only 3 pulses per encoder rotation ENCODER_PPR
      EncoderDTMicrosSamplePPR();
    #endif
    // check channel B to see which way encoder is turning & set rotation direction
    if (digitalRead(ENCODER_PINB)){
      rotationDir = 1;
      encoderCounts++;
    }else{
      rotationDir = -1;
      encoderCounts--;
    }
    //digitalRead(ENCODER_PINB) ? encoderCounts++ : encoderCounts--;
  }else{
    // check channel B to see which way encoder is turning
    digitalRead(ENCODER_PINB) ? encoderCounts-- : encoderCounts++;
  }
}
// EncoderA ISR
void encoderB() {
  #ifndef FILTER_PPR
    // sample time between pulses
    EncoderDTMicrosSample();
  #endif
  // look for a low-to-high on channel B
  if (digitalRead(ENCODER_PINB) == HIGH) {
    // check channel A to see which way encoder is turning
    digitalRead(ENCODER_PINA) ? encoderCounts-- : encoderCounts++;    
  }else{
    // check channel A to see which way encoder is turning
    digitalRead(ENCODER_PINA) ? encoderCounts++ : encoderCounts--; 
  }
}

// sample time between pulses simplified for resources, for 3 pulses option ppr mode
void EncoderDTMicrosSamplePPR(){
    // calculate time passed between pulses and perform averaging
    unsigned long microsTick = micros();
    encoderDTmicros = microsTick - lastMicrosENCTick;
    lastMicrosENCTick = microsTick;
    // calculate filtered DTmicros
    static unsigned long lastENCdt [3] ={10000};
    lastENCdt[0] = lastENCdt[1];
    lastENCdt[1] = lastENCdt[2];
    lastENCdt[2] = encoderDTmicros;
    encoderDTmicrosFiltered = (lastENCdt[0] + lastENCdt[1] + lastENCdt[2]) / 3;
}

// // sample time between pulses more flexible solution but more taxing on CPU
 void EncoderDTMicrosSample(){
     // calculate time passed between pulses and perform averaging
     unsigned long microsTick = micros();
     encoderDTmicros = microsTick - lastMicrosENCTick;
     lastMicrosENCTick = microsTick;
    // calculate filtered DTmicros
    static unsigned long lastENCdt [ENCODER_FILTER] ={10000};
    unsigned long calcEncDT = encoderDTmicros;
    for (int ii = 0 ; ii < (ENCODER_FILTER-1) ; ii++){
      lastENCdt[ii] = lastENCdt[ii+1];
      calcEncDT = calcEncDT + lastENCdt[ii];
    }
    // update last buffer value and calculte filtered dt
    lastENCdt[(ENCODER_FILTER-1)] = encoderDTmicros;
    encoderDTmicrosFiltered = calcEncDT / ENCODER_FILTER;
}
