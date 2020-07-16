#include "kugelfall.h"

int i;
struct rotationAndHallMeasure m;

void setup(){
  // put your setup code here, to run once:
  setupHardware();
}

void loop(){
  awaitHallSensorPosition();
  m = measureRotationAndHallUntil(5, 0, 99999999);

  Serial.print(i++);
  Serial.print(",");
  Serial.print(m.rotation.time);
  Serial.print(",");
  Serial.print(m.rotation.deceleration);
  Serial.println("");
}
