#include "kugelfall.h"
#include <Servo.h>

int i;
struct rotationAndHallMeasure m;
int oldTime = 0;
int newTime = 0;

volatile long timeBeforeLastPhoto = 0;
volatile long rotationTimePhoto = 0;


void photosensorISR() {
    rotationTimePhoto = millis() - timeBeforeLastPhoto;
    timeBeforeLastPhoto = millis();
}

void setup(){
  // put your setup code here, to run once:
  setupHardware();
  Serial.begin(9600);
  attachInterrupt(0, photosensorISR, RISING);
}

void loop(){
    delay(1000);
    Serial.println(rotationTimePhoto);
}
