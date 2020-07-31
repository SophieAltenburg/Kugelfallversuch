#include "kugelfall_interrupt.h"
#include <Servo.h>

const int fallTime = 470;
const int throwMarble[3][10] = {
    {1, 0, 1,  0,  1, -1, -1, -1, -1, -1}, 	// fast velocity = 0
    {1, 0, 1,  1,  0,  0,  1,  1,  1, -1}, 	// medium velocity = 1
    {1, 1, 1, -1, -1, -1, -1, -1, -1, -1} 	// slow velocity = 2
};

void setup() {
    /*
    Setup the hardware when booting the Arduino.
    */

    setupHardware();
}

void loop() {
    /*
    Infinite loop will be run after booting the Arduino. Use polling on the
    trigger pin and begin a pattern when the button gets pressed.
    
    By checking the isValid flag, unexpected changes in deceleration or 
    acceleration can be detected. In this case, wait until the deceleration is
    valid again, but at least one full rotation.

    In case the velocity mode changes during a pattern, the old pattern will be
    continued nonetheless, but a warning will be written to the serial port.
    */
    if (isTriggered()) {
        Serial.println("Trigger detected.");
        bool patternDone = false;
        int velocityModeAtBegin = velocityMode;

        for (int turn = 0; turn <= 9 && patternDone == false; turn++)  {

            if (velocityMode != velocityModeAtBegin){
                Serial.print("Warning: Mode changed during pattern. Old pattern ");
                Serial.print("be continued, but may not be conforming to new ");
                Serial.println("mode anymore.");
            }

            while (!isValidFlag){
                // invalid acceleration or deceleration detected --> wait until
                // valid again, but at least one full rotation
                Serial.print("Invalid acceleration or deceleration detected.");
                Serial.println("Waiting until rotations are predictable again.");
                delay(currentTurnTime);
            }

            switch (throwMarble[velocityModeAtBegin][turn]) {
                case 1: {
                    // throw a marble this turn
                    int t = expHallFlip - millis() - fallTime;
                    
                    if (t < 0) {
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
                    delay(currentTurnTime);
                    break;
                }

                case -1: {
                    Serial.println("Pattern done.");
                    patternDone = true;
                    break;
                }
            } // end switch - decided marble release
        } // end for - completed marble pattern
    } // end if - completed being triggered
} // end loop
