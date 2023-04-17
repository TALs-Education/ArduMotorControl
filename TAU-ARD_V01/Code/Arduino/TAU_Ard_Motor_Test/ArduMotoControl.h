/* ArduMotoControl

    Arduino Uno
    Sparkfun Ardumoto shield Dev-14129
    SparkfunShield cut jp DIRA PWMA, Connect to MotorB
        
    LedPin   D13
    UartRx   D0
    TuartTx  D1

    DirPin   D4
    PWMPin   D11
    EncoderA D2
    EncoderB D3

    Version update:
    V02 - Moved gear ratio to main code, updated conversion constants accordingly, Added timeout variable
    todo:

    sometime todo:
    add chirp profile
    add position control
    readRPM isnt smooth enough for motor stop condition
    analyze why its not perfect, problematic settings 6 RPM MINIMUM_ENCDT 20000
*/
#ifndef ArduMotoControl_H_
#define ArduMotoControl_H_

#include "Arduino.h"

// define pins
#define PWM2_PIN 6 // when jumper resistor is placed 4-->6 then switch to pin 6
#define PWM1_PIN 5
#define ENCODER_PINA 2
#define ENCODER_PINB 3

// define filter mode PPR (3 pulses per revolution - less controller resources), for CPR mode (12 counts per revolution) - uncomment #define FILTER_PPR
// at ppr mode there are less encoder missing pulses
#define FILTER_PPR

// define constants
#define ENCODER_PPR 3         // encoder puplses per revolution
#define ENCODER_CPR 12        // encoder counts per revolution
#define MINIMUM_ENCDT 72000   // define maximum time between pulses to check if the motor has stopped. 1200 --> 60RPM --> 1 sample per control loop CPR Mode 

// define conversion constants 
#define ENCPULSES_RPM 5000     // 12 pulses per revolution -->60/12 = 5 --> 5*1000, since samples are at milliseconds
#define ENCPULSES_RADS 523.59  // ENCPULSES_RPM*2*pi/60
// Convertion constants
#define RADS_RPM 9.5493       // convert from rad/s to RPM

// define Encoder dt to RPM conversion coeficients
#ifdef FILTER_PPR
    // coeficients for encoder sample at PPR 3 pulses
    #define ENCDT_RPM 20000000 
    #define ENCDT_RADS 2094392 
    #define ENCODER_FILTER 3     // define the number of values to filter for the encoder measurement.
#else
    // coeficients for encoder sample at CPR 12 pulses
    #define ENCDT_RPM 5000000 
    #define ENCDT_RADS 523598 
    #define ENCODER_FILTER 12     // define the number of values to filter for the encoder measurement.
#endif
// conversion coeficients calculation
// float rpmEncoder = 60 * (((float(encoderPulses)) * 1000 / float(dtMillisEnc)) / ENCODER_CPR); // 12 pulses per revolution
// float rpmEncoder = rotationDir * 60 * (1000000 / (float(encoderDTmicros) * ENCODER_CPR)); // dt in microseconds with 3 pulses per revolution
// float RadSEncoder = (((float(encoderPulses)) * 1000 / float(dtMillisEnc)) / ENCODER_CPR * TWO_PI); // 12 pulses per revolution
// float rpmEncoder = rotationDir * (1000000 / (float(encoderDTmicros) * ENCODER_CPR) * TWO_PI); // dt in microseconds with 3 pulses per revolution

// encoder variables
extern volatile int encoderCounts;
extern volatile unsigned long encoderDTmicros;
extern volatile unsigned long encoderDTmicrosFiltered;

// initialize encoder, Motor, attach ISR functions
void InitArduMotorControl();

// finalized read encoder functions based on averaging time between pulses
float ReadRPM();
float ReadRadS();

// motor command update
void motorComand(float cntrlValue);

// motion profiles
float StepCMD(float stepPeriod);
float RampCMD(float rampPeriod);
float SineCMD(float f_Hz);


// Encoder ISR functions - Interupt Service Routin
void encoderA();
void encoderB();

// sample encoder time between pulses
void EncoderDTMicrosSample();
// sample time between pulses simplified for resources, for 3 pulses option ppr mode
void EncoderDTMicrosSamplePPR();

// read encoder functions
float readRPMEncoder();       // RPM measured from number of pulses over control period
float readRPMdtEncoder();     // RPM measured from dt between pulses more precise at lower speeds
float readRadSEncoder();      // rad/s measured from number of pulses over control period  
float readRadSdtEncoder();    // rad/s measured from dt between pulses more precise at lower speeds
#endif
