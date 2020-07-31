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
    // Serial.print in an ISR or a function called from an ISR is illegal!

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
    servo.write(60);
}

void closeMechanism(){
    servo.write(35);
}

bool isTriggered(){
    return(digitalRead(TriggerPin));
}

void awaitTrigger(){
    while (!isTriggered());
}

void awaitHallSensorPosition(){
    while(digitalRead(HallSensorPin) == 0);
    while(digitalRead(HallSensorPin) == 1);
    return;
}

void HallSensorISR(){
    lastHallFlip = millis();
    
    expHallFlip = lastHallFlip + currentTurnTime;
}

void PhotoSensorISR(){
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
