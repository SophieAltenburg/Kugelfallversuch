#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#ifndef _KUGELFALL_H
#define _KUGELFALL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <Servo.h>

int PhotoSensorInterruptPin = 0;
int HallSensorInterruptPin = 1;
int PhotoSensorPin = 2;
int HallSensorPin = 3;
int TriggerPin = 4;
int SwitchPin = 5;
int BlackboxLEDPin = 7;
int ServoPin = 9;
int Button1Pin = 10;
int Button2Pin = 11;
int LED1Pin = 12;
int LED2Pin = 13;

volatile long lastPhotoTimestamp = 0;
volatile int lastTurnTime = 0;
volatile int currentTurnTime = 0;
volatile long lastHallFlip = 0;
volatile long expHallFlip = 0;
volatile int velocityMode = 0;
volatile bool isValidFlag = false;

bool isValid(int thisTime, int lastTime){
    int diff = thisTime - lastTime;
    int threshold;

    // never accept negative difference above measurement uncertainty
    if (diff < -10) {
        Serial.println("Invalid deceleration time. Measured negative deceleration above measurement uncertainty");
        return(false);
    }

    // below 900 ms, accept differences of up to 20 ms
    if (lastTime <= 900) {
        if (diff <= 20) {
            return(true);
        } else {
            Serial.print("Invalid deceleration time. Measured difference ");
            Serial.print(diff);
            Serial.println(" but threshold was 20");
            return(false);
        }
    } 

    // between 900 and 2200 ms, use a linear function to get accepted deceleration
    if (lastTime > 900 && lastTime <= 2200) {
        threshold = (lastTime-900)/20 + 20;
        if (diff <= threshold) {
            return(true);
        } else {
            Serial.print("Invalid deceleration time. Measured difference ");
            Serial.print(diff);
            Serial.print(" but threshold was ");
            Serial.println(threshold);
            return(false);
        }
    }

    // between 2200 and 2500 ms, use a different linear function to get accepted deceleration
    if (lastTime > 2200 && lastTime <= 2500) {
        threshold = (lastTime-2200)/10 + 85;
        if (diff <= threshold) {
            return(true);
        } else {
            Serial.print("Invalid deceleration time. Measured difference ");
            Serial.print(diff);
            Serial.print(" but threshold was ");
            Serial.println(threshold);			
            return(false);
        }
    }

    // accept anything if the turn time is above 2500 ms
    if (lastTime > 2500) {
        return(true);
    }
}

void openMechanism(Servo servo){
    servo.write(60);
}

void closeMechanism(Servo servo){
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

    if (currentTurnTime < 1000) {
        // fast
        velocityMode = 0;
        digitalWrite(LED1Pin, 1);
        digitalWrite(LED2Pin, 0);
    } else if (currentTurnTime > 3000) {
        // slow
        velocityMode = 2;
        digitalWrite(LED1Pin, 0);
        digitalWrite(LED2Pin, 1);
    } else {
        // medium
        velocityMode = 1;
        digitalWrite(LED1Pin, 1);
        digitalWrite(LED2Pin, 1);
    }

    isValidFlag = isValid(currentTurnTime, lastTurnTime);
    digitalWrite(BlackboxLEDPin, isValidFlag);
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

    attachInterrupt(PhotoSensorInterruptPin, PhotoSensorISR, RISING);
    attachInterrupt(HallSensorInterruptPin, HallSensorISR, FALLING);
}

#ifdef __cplusplus
}
#endif

#endif
