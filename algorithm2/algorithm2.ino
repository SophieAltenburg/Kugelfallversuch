#include "kugelfall_interrupt.h"
#include <Servo.h>

const int fallTime = 462;
const int throwMarble[3][9] = {
    {1, 0, 1,  0,  1, -1, -1, -1, -1}, 	// fast velocity = 0
    {1, 0, 1,  1,  0,  0,  1,  1,  1}, 	// medium velocity = 1
    {1, 1, 1, -1, -1, -1, -1, -1, -1} 	// slow velocity = 2
};

void setup() {
    setupHardware();
}

void loop() {

    if (isTriggered()) {
        Serial.println("Trigger detected.");
        bool patternDone = false;
        
        expHallFlip = lastHallFlip + currentTurnTime;
        
        for (int turn = 0; turn < 9 && patternDone == false; turn++)  {

            if (!isValidFlag){
                // invalid deceleration detected --> wait until valid again
                while (!isValidFlag);
                awaitHallSensorPosition();
            }

            switch (throwMarble[velocityMode][turn]) {
                case 1: {
                    // throw a marble this turn
                    long t = expHallFlip - fallTime;
                    
                    if (t < millis()) {
                        t = t + currentTurnTime;          
                    }

                    Serial.print("Waiting for ");
                    Serial.println(t);          
                    delay(t);

                    openMechanism();
                    // wait either half a turn or 500 ms, whichever comes first,
                    // to close the servo mechanism
                    delay(min(currentTurnTime/2, 500));
                    closeMechanism();
                    break;
                }
                case 0: {
                    awaitHallSensorPosition();
                    break;
                }

                case -1: {
                    Serial.println("Pattern done.");
                    patternDone = true;
                }
            } // end switch - decided marble release
        } // end for - completed marble pattern
    } // end if - completed being triggered
} // end loop
