#include "kugelfall.h"
#include <Servo.h>

int i;
struct rotationAndHallMeasure m;
int oldTime = 0;
int newTime = 0;


void setup(){
  // put your setup code here, to run once:
  setupHardware();
}

bool isValid(int thisTime, int lastTime) {
    Serial.print("Entering isValid. Diff: ");
  int diff = thisTime - lastTime;
  Serial.println(diff);
  bool ret;
  int threshold;
  
  // below 900 ms, accept differences of up to 20 ms
  if (lastTime <= 900) {
      if (diff <= 20) {
         ret = true;
       } else {
         ret = false;
      }
  }
  
  // between 900 and 2200 ms, use a linear function to get accepted deceleration
  if (lastTime > 900 && lastTime <= 2200) {
      threshold = (lastTime-900)/20 + 20;
      if (diff <= threshold) {
         ret = true;
      } else {
         ret = false;
      }
  }
  
  // between 2200 and 2500 ms, use a different linear function to get accepted deceleration
  if (lastTime > 2200 && lastTime <= 2500) {
      threshold = (lastTime-2200)/10 + 85;
      if (diff <= threshold) {
         ret = true;
      } else {
         ret = false;
      }
  }
  
  // accept anything if the turn time is above 2500 ms
  if (lastTime > 2500) {
      ret = true;
  }
  
  // never accept negative difference above measurement uncertainty
  if (diff < -10) {
      ret = false;
  }
  
  // one of either values not initialized, accept it
  if (thisTime == 0 || lastTime == 0) {
      ret = true;
  }
  
  if (ret == false) {
      Serial.print("thisTime: ");
      Serial.println(thisTime);
      Serial.print("lastTime: ");
      Serial.println(lastTime);
   
  }
      Serial.print("Thresholds: ");
      Serial.println(threshold);
  
  digitalWrite(BlackboxLEDPin, !ret);
  return(ret);
}

void loop(){
  m = measureRotationAndHallUntil(2, 0, 99999999);
  newTime = m.rotation.time;
  isValid(newTime, oldTime);
  oldTime = newTime;
}
