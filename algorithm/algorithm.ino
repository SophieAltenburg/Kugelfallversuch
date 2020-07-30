#include "kugelfall.h"
#include <Servo.h>

Servo servo;
const int fallTime = 462;
int hallState = -1; //will be 0 or 1 depending on last hallSesor measurement
long lastHallFlip = -1; //timestamp of last hallSensor signal flip
long expHallFlip = -1; //expected next hallSensor flip
long guessedLastHallFlip = -1; //if we missed the last hallSensor flip, we guess it
int lastTurnTime = -1;
int expTurnTime = -1;
const int throwMarble[3][9] = {
  {1, 0, 1,  0,  1, -1, -1, -1, -1}, //fast velocity = 0
  {1, 0, 1,  1,  0,  0,  1,  1,  1}, //medium velocity = 1
  {1, 1, 1, -1, -1, -1, -1, -1, -1} //slow velocity = 2
};

void setup() {
  // put your setup code here, to run once:
  setupHardware();
  servo.attach(ServoPin);
  closeMechanism(servo);
}

bool isValid(int thisTime, int lastTime) {
    Serial.print("Entering isValid. Diff: ");
  int diff = thisTime - lastTime;
  Serial.println(diff);
  bool ret;
  int threshold;
  
  // below 900 ms, accept differences of up to 20 ms
  if (thisTime <= 900) {
      if (diff <= 20) {
         ret = true;
       } else {
         ret = false;
      }
  }
  
  // between 900 and 2200 ms, use a linear function to get accepted deceleration
  if (thisTime > 900 && thisTime <= 2200) {
      threshold = (thisTime-900)/20 + 20;
      if (diff <= threshold) {
         ret = true;
      } else {
         ret = false;
      }
  }
  
  // between 2200 and 2500 ms, use a different linear function to get accepted deceleration
  if (thisTime > 2200 && thisTime <= 2500) {
      threshold = (thisTime-2200)/100 + 85;
      if (diff <= threshold) {
         ret = true;
      } else {
         ret = false;
      }
  }
  
  // accept anything if the turn time is above 2500 ms
  if (thisTime > 2500) {
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

int velocityModeOrWait() {
  Serial.println("Entering velocityModeOrWait.");
  int accurateValues = 0;
  struct rotationAndHallMeasure measure;
  do {
    measure = measureRotationAndHall(2, hallState);
    if (accurateValues == 0) {
      //set last and expected turn time equal to the measured turn time
      expTurnTime = measure.rotation.time;
      lastTurnTime = expTurnTime;
      accurateValues++;
      Serial.println("First measurement, no comparison.");
    } else {
      //update last and expected turn time and decide if they are valid
      lastTurnTime = expTurnTime;
      expTurnTime = measure.rotation.time;
      if (isValid(expTurnTime, lastTurnTime)) {
        accurateValues++;
        Serial.println("Second measurement is valid.");
      } else {
        accurateValues = 1;
        Serial.println("Second measurement is not valid.");
      }
      //accurateValues = (isValid(expTurnTime, lastTurnTime)) ? (accurateValues++) : (1);
    }
  } while (accurateValues < 1);

  //5 values were accurate. The measurement is reliable. Compute other global variables:
  if (measure.hall.state >= 0) {
    hallState = measure.hall.state;  
  }
  if (measure.hall.time > 0) {
    lastHallFlip = measure.hall.time;
    expHallFlip = lastHallFlip + expTurnTime;  
  }
  

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

void waitButListenToHallSensor(long waitUntil) {
  long initialTime = millis();
  long timestamp = -1;
  int hallState = digitalRead(HallSensorPin);

  while (millis() < waitUntil) {
    if (digitalRead(HallSensorPin) == 0 && hallState != 0) {
      timestamp = millis();
      hallState = 0;
    } else if (digitalRead(HallSensorPin) == 1 && hallState != 1) {
      hallState = 1;
    }
  }
  if (timestamp > -1) {
    lastHallFlip = timestamp;
    expHallFlip = lastHallFlip + expTurnTime;
  }
  return;
}

bool validRotationMeasureBefore(long deadline) {
  /* Performs a rotation and hall sensor measurement in 1/3 of the turn time of the
     plate, if the plate decelerates normally. If the plate is decelerated manually, the
     measurement will be marked as not valid by returning false. The function also waits until
     the deadline is over.

     There are 3 black and white cycles needed for a solid rotation measurement. Those 6 fields
     are 1/4 of the 24 color fields underneath the plate. Therefore a valid rotation measurement
     should be doable in 1/4 of the expected rotation time. The minimum servo moving delay is
     about 100ms which is a little less than 1/3 of the fastes rotation time. So for a rotation
     measurement and waiting to close the servo, 1/3 of the expected turn time should be a good
     value.

  */
  Serial.print("Entering validRotationMeasureBefore ");
  Serial.print(deadline);
  /*if (deadline - millis() <= expTurnTime / 2) {
    Serial.println("The deadline is unrealistic. Measurement aborts and will not be valid.");
    return false;
  }*/
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
  if (m.rotation.time > 0) {
    lastTurnTime = expTurnTime;
    expTurnTime = m.rotation.time;
    expHallFlip = lastHallFlip + expTurnTime;
  }

  //wait until deadline
  while (millis() < deadline) {
    // chill
  }
  Serial.println("Measurement was valid.");
  return true; //global variables are up to date and valid
}

void loop() {
  //Make sure the velocity is stable. Then compute
  //velo (velocity Mode), lastTurnTime, expTurnTime, hallState, lastHallFlip and expHallFlip.
  int velo = velocityModeOrWait();
  Serial.print("Velocity mode is: ");
  Serial.println(velo);

  if (isTriggered()) {
    Serial.println("Trigger detected.");
    bool patternDone = false;
    
    lastHallFlip = awaitHallSensorPosition();
    expHallFlip = lastHallFlip + expTurnTime;
    Serial.print("Got hall sensor 1-0 endge at: ");
    Serial.println(millis());
    
    for (int turn = 0; turn < 9 && patternDone == false; turn++)  {
      // damn amount of prints:
      Serial.print("velo Nr: ");
      Serial.println(velo);
      Serial.print("turn Nr: ");
      Serial.println(turn);
      Serial.print("hallState: ");
      Serial.println(hallState);
      Serial.print("lastHallFlip: ");
      Serial.println(lastHallFlip);
      Serial.print("expHallFlip: "); // sometimes not up to date
      Serial.println(expHallFlip);
      Serial.print("lastTurnTime: ");
      Serial.println(lastTurnTime);
      Serial.print("expTurnTime: ");
      Serial.println(expTurnTime);

      switch (throwMarble[velo][turn]) {
        case 1: {
            Serial.println("Release the kraken... marble!");
            //release the marble
            //expHallFlip = lastHallFlip + expTurnTime;
            long t = expHallFlip - fallTime;
            Serial.println(t);  
            if (t < millis()) {
              t = t + expTurnTime;          
            }
            if (t < millis()) {
              t = t + lastTurnTime;
            }
            Serial.print("Waiting for ");
            Serial.println(t);          
            waitButListenToHallSensor(t);
            //we never get the hall sensor between hallsensor and throw-time
            openMechanism(servo);
            
            if (!validRotationMeasureBefore(millis() + expTurnTime / 2)) {
              Serial.println("no valid Measurement in case 1 -> velocityModeOrWait");
              int newVelo = velocityModeOrWait(); //wait until measurements are stable again
              if (newVelo != velo) {
                Serial.println("Abort pattern because of different velocity mode.");
                patternDone = true;
                velo = newVelo;
              }
            } else {
              Serial.println("Valid rotation measure and update before deadline in case 1");
            }
            closeMechanism(servo);
            break;
          }
        case 0: {
            //no marble this turn
            if (!validRotationMeasureBefore(millis() + expTurnTime / 2)) {
              Serial.println("velocityModeOrWait case 0");
              int newVelo = velocityModeOrWait(); //wait until measurements are stable again
              if (newVelo != velo) {
                Serial.print("Abort pattern because of different velocity mode.");
                patternDone = true;
                velo = newVelo;
              }
            } else {
              Serial.println("Valid rotation measure and update before deadline in case 0");
              //TODO get new expHallFlip
            }
            lastHallFlip = awaitHallSensorPosition();
            expHallFlip = lastHallFlip + expTurnTime;
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
