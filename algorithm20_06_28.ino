#include "kugelfall.h"
#include <Servo.h>

int pos;
Servo servo;
const int fallTime = 462;
int hallState = -1; //will be 0 or 1 depending on last hallSesor measurement
int lastHallFlip = -1; //timestamp of last hallSensor signal flip
int expHallFlip = -1;
int lastTurnTime = -1;
int expTurnTime = -1;
const int throwMarble[3][9] = {
  {1, 0, 1, 0, 1,-1,-1,-1,-1}, //fast velocity = 0
  {1, 0, 1, 1, 0, 0, 1, 1, 1}, //medium velocity = 1
  {1, 1, 1,-1,-1,-1,-1,-1,-1}  //slow velocity = 2
};

void setup() {
  // put your setup code here, to run once:
  setupHardware();
  servo.attach(ServoPin);
  closeMechanism(servo);
}

bool isValid(int thisTime, int lastTime) {
  int min = lastTime * 0.9;
  int max = lastTime * 1;
  if (min < thisTime && thisTime < max) {
    return true;
  }
  //return false;
  return true;
}

int velocityModeOrWait() {
  int accurateValues = 0;
  struct rotationAndHallMeasure measure;
  do {
    measure = measureRotationAndHall(2, hallState);
    if (accurateValues == 0) {
      //set last and expected turn time equal to the measured turn time
      expTurnTime = measure.rotation.time;
      lastTurnTime = expTurnTime;
      accurateValues++;
      
    } else {
      //update last and expected turn time and decide if they are valid
      lastTurnTime = expTurnTime;
      expTurnTime = measure.rotation.time;
      if (isValid(expTurnTime, lastTurnTime)) {
          accurateValues++;
      } else {
          accurateValues = 1;
      }
      //accurateValues = (isValid(expTurnTime, lastTurnTime)) ? (accurateValues++) : (1);
    }
  } while(accurateValues < 1);

  //5 values were accurate. The measurement is reliable. Compute other global variables:
  hallState = measure.hall.state;
  lastHallFlip = measure.hall.time;
  expHallFlip = lastHallFlip + expTurnTime;
  
  //decide velocity mode:
  if (expTurnTime <= 1000) {
    // more than 1 turn per second, return fast
    return 0;
  }
  if (3000 > expTurnTime && expTurnTime > 1000) {
    // a turn takes 1 to 3 seconds, return medium
    return 1;
  }
  if (expTurnTime >= 3000) {
    // a turn takes more than 3 seconds, return slow
    return 2;
  }
}  

void waitButListenToHallSensor(int waitUntil){
  int initialTime = millis();
  int timestamp = -1;
  int hallState = digitalRead(HallSensorPin);
  
  while (millis() < waitUntil){
    if (digitalRead(HallSensorPin) == 0 && hallState != 0){
      timestamp = millis();
      hallState = 0;
    } else if (digitalRead(HallSensorPin) == 1 && hallState != 1) {
      hallState = 1;
    }
  }
  if (timestamp > -1) {
    lastHallFlip = timestamp;
  }
  return;
}

bool validRotationMeasureBefore(int deadline) {
 /* Performs a rotation and hall sensor measurement in 1/3 of the turn time of the
  * plate, if the plate decelerates normally. If the plate is decelerated manually, the
  * measurement will be marked as not valid by returning false. The function also waits until
  * the deadline is over.
  * 
  * There are 3 black and white cycles needed for a solid rotation measurement. Those 6 fields
  * are 1/4 of the 24 color fields underneath the plate. Therefore a valid rotation measurement
  * should be doable in 1/4 of the expected rotation time. The minimum servo moving delay is
  * about 100ms which is a little less than 1/3 of the fastes rotation time. So for a rotation
  * measurement and waiting to close the servo, 1/3 of the expected turn time should be a good
  * value.
  * 
  */

  if (deadline - millis() < expTurnTime/2) {
    Serial.println("The deadline is unrealistic. Measurement aborts and will not be valid.");
    return false;
  }
  /* @Jakob, kann ich globale Variablen wie hallState auch ohne sie an die Fkt zu übergeben in
  * der kugelfall.h nutzen? Dann müsste ich das ganze updaten namlich nicht hier unten machen,
  * sondern könnte das die Fkt whilePhotoListenToHallSensor übernehmen lassen.
  */
  //measure rotation and hall sensor
  struct rotationAndHallMeasure m = measureRotationAndHallUntil(2, hallState, deadline);
  if (m.hall.state == -2) {
    Serial.println("Plate got abnormally decelerated, measurement took time past the deadline.");
    return false;
  }
  //now update all global variables
  hallState = (m.hall.state != -1) ? (m.hall.state) : (hallState);
  lastHallFlip = (m.hall.time != -1) ? (m.hall.time) : (lastHallFlip);
  if (!isValid(m.rotation.time, lastTurnTime)) {
    Serial.println("Plate decelerated or accelerated too much.");
    return false;
  }
  lastTurnTime = expTurnTime;
  expTurnTime = m.rotation.time;
  expHallFlip = lastHallFlip + expTurnTime;

  //wait until deadline
  while (millis() < deadline) {
    // chill
  }
  return true; //global variables are up to date and valid
}

void loop() {
  //Make sure the velocity is stable. Then compute
  //velo (velocity Mode), lastTurnTime, expTurnTime, hallState, lastHallFlip and expHallFlip.
  int velo = velocityModeOrWait();
  
  if (isTriggered()){
    //lastHallFlip = awaitHallSensorPosition(); //TODO might not be necessary
    bool patternDone = false;
    
    awaitHallSensorPosition();
    for (int turn = 0; turn < 9 && patternDone == false; turn++)  {
      // damn amount of prints:
      Serial.print("hallState: ");
      Serial.println(hallState);
      Serial.print("lastHallFlip: ");
      Serial.println(lastHallFlip);
      Serial.print("expHallFlip: ");
      Serial.println(expHallFlip);
      Serial.print("lastTurnTime: ");
      Serial.println(lastTurnTime);
      Serial.print("expTurnTime: ");
      Serial.println(expTurnTime);

      switch (throwMarble[velo][turn]) {
        case 1: {
          //release the marble
          expHallFlip = lastHallFlip + expTurnTime;
          int t = expHallFlip - fallTime;
          t = (t < millis()) ? (t + expTurnTime) : (t);
          waitButListenToHallSensor(t);
          openMechanism(servo);
          if (!validRotationMeasureBefore(millis() + expTurnTime/2)) {
            Serial.println("velocityModeOrWait case 1");
            int newVelo = velocityModeOrWait(); //wait until measurements are stable again
            if (newVelo != velo) {
              Serial.println("Done.");
              patternDone = true;
              velo = newVelo;
            }
          } else {
             Serial.println("Else case 1");
          }
          closeMechanism(servo);
          break;
        }
        case 0: {
          //no marble this turn
          if (!validRotationMeasureBefore(millis() + expTurnTime/2)){
              Serial.println("velocityModeOrWait case 0");
            int newVelo = velocityModeOrWait(); //wait until measurements are stable again
            if (newVelo != velo) {
              Serial.print("Done.");
              patternDone = true;
              velo = newVelo;
            }
          } else {
              Serial.println("Else case 0");
          }
          lastHallFlip = awaitHallSensorPosition();
          break;
        }

        case -1: {
          Serial.println("Done.");
          patternDone = true;
        }
      } // end switch - decided marble release
    } // end for - completed marble pattern
  } // end if - completed being triggered
} // end loop
