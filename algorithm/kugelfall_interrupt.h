#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#ifndef _KUGELFALL_INTERRUPT_H
#define _KUGELFALL_INTERRUPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <Servo.h>

const int PhotoSensorInterruptPin = 0;
const int HallSensorInterruptPin = 1;
const int PhotoSensorPin = 2;
const int HallSensorPin = 3;
const int TriggerPin = 4;
const int SwitchPin = 5;
const int BlackboxLEDPin = 7;
const int ServoPin = 9;
const int Button1Pin = 10;
const int Button2Pin = 11;
const int LED1Pin = 12;
const int LED2Pin = 13;

Servo servo;

volatile long lastPhotoTimestamp = 0;
volatile int lastTurnTime = 0;
volatile int currentTurnTime = 0;
volatile long lastHallFlip = 0;
volatile long expHallFlip = 0;
volatile int velocityMode = 0;
volatile bool isValidFlag = 0;

bool isValid(int thisTime, int lastTime){
    /*
    Checks if two turn times are valid by determining the deceleration between
    them and comparing them to the acceptable deceleration threshold.

    Decelerations below -20 ms (i.e. accelerations above measurement
    uncertainty) will always be rejected. If the last time was below 900 ms,
    then decelerations of up to 20 ms will be accepted. Betwee 900 and 2200, a
    linear function will be used for the threshold. Between 2200 and 2500 ms, a
    different linear function (with twice the gradient) will be used. Above 2500
    ms, the disc becomes so slow that decelerations through outside influence
    can no longer be detected, so all will be accepted.

    Arguments
    ---------
    thisTime : int
        The time for the last full rotation.
    lastTime : int
        The time for the rotation before that.

    Returns
    -------
    return : bool
        True if the deceleration is acceptable, false otherwise.
    */

    // serial prints in an ISR or a function called from an ISR is illegal!

    int diff = thisTime - lastTime;
    int threshold;

    // never accept negative difference above measurement uncertainty
    if (diff < -20) {
        return(false);
    }

    // below 900 ms, accept differences of up to 20 ms
    if (lastTime <= 900) {
        if (diff <= 20) {
            return(true);
        } else {
            return(false);
        }
    } 

    // between 900 and 2200 ms, use a linear function to determine acceptable
    // deceleration
    if (lastTime > 900 && lastTime <= 2200) {
        threshold = (lastTime-900)/20 + 20;
        if (diff <= threshold) {
            return(true);
        } else {
            return(false);
        }
    }

    // between 2200 and 2500 ms, use a different linear function to determine
    // acceptable deceleration
    if (lastTime > 2200 && lastTime <= 2500) {
        threshold = (lastTime-2200)/10 + 85;
        if (diff <= threshold) {
            return(true);
        } else {
            return(false);
        }
    }

    // accept anything if the turn time is above 2500 ms
    if (lastTime > 2500) {
        return(true);
    }
}

void openMechanism(){
    /*
    Sets the servo drive to 60° to open the mechanism to release a marble.
    */

    servo.write(60);
}

void closeMechanism(){
    /*
    Sets the servo drive to 35° to close the mechanism after throwing a marble.
    */

    servo.write(35);
}

bool isTriggered(){
    /*
    Checks if the trigger button on the handheld control is being pressed.

    Returns
    -------
    return : bool
        True if the button is being pressed, false otherwise.
    */

    return(digitalRead(TriggerPin));
}

void awaitTrigger(){
    /*
    Waits until the trigger button on the handheld control gets pressed, then 
    returns.
    */

    while (!isTriggered());
}

void awaitHallSensorPosition(){
    /*
    Wait until the next falling edge on the Hall sensor pin, then returns.
    */

    while(digitalRead(HallSensorPin) == 0);
    while(digitalRead(HallSensorPin) == 1);
    return;
}

void HallSensorISR(){
    /*
    Interrupt service routine for the Hall sensor pin.

    This function updates two variables -- lastHallFlip and expHallFlip. The 
    last Hall flip is the timestamp when the ISR gets called, the expected
    Hall flip is the estimated timestamp when the next falling edge will be
    detected.
    */

    // serial prints in an ISR or a function called from an ISR is illegal!

    lastHallFlip = millis();
    
    expHallFlip = lastHallFlip + currentTurnTime;
}

void PhotoSensorISR(){
    /*
    Iterrupt service routine for the photo sensor pin.

    This function uses the photo sensor to determine the rotation time by 
    memorizing the timestamp this function has been called the last time and
    calculating the difference between these two. Depending on the rotation time
    the velocity mode (0 - fast, 1 - medium, 2 - slow) will be set and the LEDs
    will be illuminated accordingly. 

    This ISR also calls the isValid function to determine if the measured time 
    is acceptable and illuminates the LED on the black box if it's not.
    */

    // serial prints in an ISR or a function called from an ISR is illegal!
    
    lastTurnTime = currentTurnTime;

    currentTurnTime = 6*(millis() - lastPhotoTimestamp);

    lastPhotoTimestamp = millis();

    if (currentTurnTime < 1000) {
        // fast
        velocityMode = 0;
        digitalWrite(LED1Pin, 1);
        digitalWrite(LED2Pin, 0);
    }
    if (currentTurnTime <= 3000 && currentTurnTime >= 1000) {
        // medium
        velocityMode = 1;
        digitalWrite(LED1Pin, 1);
        digitalWrite(LED2Pin, 1);
    }
    if (currentTurnTime > 3000) {
        // slow
        velocityMode = 2;
        digitalWrite(LED1Pin, 0);
        digitalWrite(LED2Pin, 1);
    }

    isValidFlag = isValid(currentTurnTime, lastTurnTime);
    digitalWrite(BlackboxLEDPin, !isValidFlag);
}

void setupHardware(){
    /*
    Initializes the hardware by initializing the serial port, declaring the all
    the pins, initializing the servo drive and attaching the interrupt routines
    HallSensorISR and PhotoSensorISR to the respective pins.
    */

    Serial.begin(9600);

    pinMode(PhotoSensorPin, INPUT);
    pinMode(HallSensorPin, INPUT);
    pinMode(TriggerPin, INPUT);
    pinMode(SwitchPin, INPUT);
    pinMode(Button1Pin, INPUT);
    pinMode(Button2Pin, OUTPUT);

    pinMode(LED1Pin, OUTPUT);
    pinMode(LED2Pin, OUTPUT);
    pinMode(BlackboxLEDPin, OUTPUT);

    servo.attach(ServoPin);
    closeMechanism();

    attachInterrupt(PhotoSensorInterruptPin, PhotoSensorISR, RISING);
    attachInterrupt(HallSensorInterruptPin, HallSensorISR, FALLING);
}

#ifdef __cplusplus
}
#endif

#endif
