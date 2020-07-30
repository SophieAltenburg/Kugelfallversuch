/* kugelfall.h
* 
* Header file providing an abstractation layer for the "Kugelfall" setup.
*/

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


struct rotationMeasure {
    /* Data from measuring the rotations.
    *
    * Attributes
    * ----------
    * time : int
    *     Time for one rotation.
    *     Unit: ms/R
    * deceleration : int
    *     Time by which the time for one rotation increases per rotation.
    *     Unit: ms/(R^2).
    */

    int time;
    int deceleration;
};

struct hallMeasure {
   /* Data from measuring the hall sensor.
    *  
    *  Attributes
    *  ----------
    *  state: int
    *      State of the hall sensor.
    *      1 for the hole, 0 for anywhere else.
    *  time: int
    *      Timestamp of the last 1-to-0 edge of the hall sensor in millis. 
    */

    int state;
    long time;
};

struct rotationAndHallMeasure {
 /* Combines the rotationMeasure and hallMeasure struct.
  *  
  *  Attributes
  *  ----------
  *  rot: struct rotationMeasure
  *  hall: struct hallMeasure
  */
  struct rotationMeasure rotation;
  struct hallMeasure hall;
};

void setupHardware(){
    /* Setup the hardware.
    *
    * Declares IO direction for each pin, sets the UART up.
    * 
    * Arguments
    * ---------
    * nothing.
    *
    * Returns
    * -------
    * nothing.
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
}

struct hallMeasure whilePhotoListenToHallSensor(int color, int hallState, long deadline) {
    /* Waits for the photo sensor and notes hall sensor changes.
    *  
    * Returns as soon as the photo sensor changes color. If the hall sensor should flip in
    * the mean time, compared to the hallState before, the change and time will be noted.
    * Returns -1 if the hall sensor did not change and the timestamp of the change otherwise.
    * 
    * Arguments
    * ---------
    * color: current color of the photo sensor
    * 
    * Returns
    * -------
    * struct hallMeasure hall:
    *    {-1;-1} for no change OR 
    *    {-2;-2} for missed deadline OR
    *    hall.time:
    *        Timestamp of last 1-to-0 edge of hall sensor in millis.
    *    hall.state:
    *        Current input value of the hall sensor, either 0 or 1.
    */
    struct hallMeasure hall;
    hall.time = -1;
    hall.state = -1;
    
    while (digitalRead(PhotoSensorPin) == color){
      if (millis() >= deadline) {
        Serial.println("Deadline for measurements is over. Measurement is not valid.");
        hall.time = -2;
        hall.state = -2;
        return hall;
      }
      if (digitalRead(HallSensorPin) == 0 && hallState != 0){
        hall.time = millis();
        hallState = 0;
        hall.state = 0;
      } else if (digitalRead(HallSensorPin) == 1 && hallState != 1) {
        hallState = 1;
        hall.state = 1;
      }
    }
    return(hall);
}

struct rotationAndHallMeasure measureRotationAndHallUntil(int cycles, int hallState, long deadline){
    /* Measure the rotation of the disc.
    *
    * Uses the photosensor to measure the time per rotation as well as the
    * deceleration of the disc. While the measurement is performed (at least two
    * black-white cycles), the software thread will be blocked. The number of 
    * cycles to obtain the measurement may be specified in the function's
    * argument. If the value is less than 1, the function will return {0, 0}
    * immediately. If the value is one, the function will only measure the
    * rotation time and return 0 for the deceleration, because there are at least
    * two cycles necessary to calculate the deceleration.
    *
    * In addition to that, the two onboard LEDs will be turned on or off
    * depending on the time per rotation:
    * Slow (>3 s/R) : green LED on
    * Medium (<3 s/R, >1 s/R) : both LEDs on
    * Fast (<1 s/R): yellow LED on
    * 
    * Arguments
    * ---------
    * cycles : int
    *     Number of Cycles that will be used to obtain the measurement.
    * hallState : int
    *     Current state of the hall sensor to detect changes.
    *
    * Returns
    * -------
    * ret : rotationMeasure
    *     A struct containing the time per rotation and the time difference
    *     between two rotations (deceleration).
    *     ret.time : int
    *         Time per rotation [ms/R]
    *     ret.deceleration : int
    *         Time by which the time per rotation increases per rotation.
    *         [ms/R^2]
    *
    * Example
    * -------
    * > struct rotationMeasure result = measureRotations(2);
    * > int time = result.time;
    * > int deceleration = result.deceleration; 
    */

    struct rotationMeasure ret;
    struct hallMeasure hall;
    hall.state = hallState;
    struct rotationAndHallMeasure both;
    both.rotation = ret;
    both.hall = hall;
    //kann ich jetzt weiterhin ret.time ändern, so dass both.rotation.time genau so geändert wird?

    if (cycles < 1) {
        both.rotation.time, both.rotation.deceleration = 0;
        return(both);
    }

    // ensure that the measurement doesn't start in the middle of a cycle.
    hall = whilePhotoListenToHallSensor(1, hall.state, deadline);
    //ensure that the deadline is not over yet
    if (hall.time == -2) {
      //deadline is over, measurement is not valid
      return both;
    }
    hall = whilePhotoListenToHallSensor(0, hall.state, deadline);
    if (hall.time == -2) {
      //deadline is over, measurement is not valid
      return both;
    }

    long timeAfterFirstCycle;
    long timeBeforeLastCycle;
    long timeAfterLastCycle;

    long timeBeforeFirstCycle = millis();

    for (int i=0; i<cycles; i++){

        hall = whilePhotoListenToHallSensor(1, hall.state, deadline);
        if (hall.time == -2) {
          //deadline is over, measurement is not valid
          return both;
        }
        hall = whilePhotoListenToHallSensor(0, hall.state, deadline);
        if (hall.time == -2) {
          //deadline is over, measurement is not valid
          return both;
        }

        //deadline is still ok, continue measurement
        if (i == 0){
            timeAfterFirstCycle = millis();
        }       

        if (i == cycles-2){
            timeBeforeLastCycle = millis();
        }

        if (i == cycles-1){
            timeAfterLastCycle = millis();
        }
    }

    ret.time = 6*(timeAfterLastCycle - timeBeforeFirstCycle)/cycles;
    
    if (cycles == 1) {
        ret.deceleration = 0;
    } else {
        ret.deceleration = 6*((timeAfterLastCycle - timeBeforeLastCycle) - (timeAfterFirstCycle - timeBeforeFirstCycle))/(cycles-1);
    }
    
    if (ret.time > 3000){
        digitalWrite(LED1Pin, 0);
        digitalWrite(LED2Pin, 1);
    } else if (ret.time < 1000){
        digitalWrite(LED1Pin, 1);
        digitalWrite(LED2Pin, 0);
    } else {
        digitalWrite(LED1Pin, 1);
        digitalWrite(LED2Pin, 1);
    }            

    // just to be sure:
    both.rotation = ret;
    both.hall = hall;
    return(both);
}

struct rotationAndHallMeasure measureRotationAndHall(int cycles, int hallState) {
    /*
    * Proxy function to make the deadline in measureRotationAndHall(cycles, hallState, deadline)
    * optional.
    */
    long maximum_millis = 2147483647; 
    return measureRotationAndHallUntil(cycles, hallState, maximum_millis);
}

long awaitHallSensorPosition(){
    /* Await the position of the hole using the Hall sensor.
    *
    * This function awaits the next 1-0 edge of the Hall sensor, which is the 
    * position of the hole in the rotating disc. This function purely serves as 
    * a blocking "wait until" function and as such accepts no arguments.
    *
    * Arguments
    * ---------
    * nothing.
    *
    * Returns
    * -------
    * timestamp of 1-0 edge
    */

    // await the next 1 level (if there's a 1 level already, the loop will exit immediately)
    while(digitalRead(HallSensorPin) == 0);

    // wait until the 1 level ends
    while(digitalRead(HallSensorPin) == 1);

    // return as soon as the Hall sensor reads 0
    return millis();
}

int isTriggered(){
    /* Determines whether the trigger button is currently being pressed.
    *
    * Arguments
    * ---------
    * nothing.
    *
    * Returns
    * -------
    * ret : int
    *     1 if the trigger button is being pressed, 0 otherwise.
    */
    
    return(digitalRead(TriggerPin));
}

void awaitTrigger(){
    /* Await a trigger button press.
    *
    * This function waits until isTriggered() is true, then returns immediately.
    * Like with the function awaitHallSensorPosition(), this function serves as
    * a blocking "wait until" function and as such accepts no arguments and 
    * returns nothing.
    *
    * Arguments
    * ---------
    * nothing.
    *
    * Returns
    * -------
    * nothing.
    */

    while (!isTriggered());
}

void openMechanism(Servo servo){
    /* Open the mechanism to release a ball.
    *
    * Sets the control input of the servo drive to 50° and return immediately.
    *
    * Arguments
    * ---------
    * servo : Servo
    *     Instance of the servo drive to use. (see: <Servo.h>)
    *
    * Returns
    * -------
    * nothing.
    */
    servo.write(60);
}

void closeMechanism(Servo servo){
    /* Close the mechanism that releases the balls.
    *
    * Sets the control input of the servo drive to 35° and return immediately.
    *
    * Arguments
    * ---------
    * servo : Servo
    *     Instance of the servo drive to use. (see: <Servo.h>)
    *
    * Returns
    * -------
    * nothing.
    */
    servo.write(35);
}

#ifdef __cplusplus
}
#endif

#endif
