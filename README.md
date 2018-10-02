= CAN lib for Smart Battery Solutions Li-Ion

The following files are copied from https://github.com/ebrady1/Due_2CAN/commit/a61e95522e44bd011f3b8f3e3524b246ab853cfc
```
DueCANLayer.cpp
DueCANLayer.h
due_can.cpp
due_can.h
```
and due_can is probably in turn copied from an older version of https://github.com/collin80/due_can

== Usage
Clone to ``~/arduino/portable/sketchbook/libraries/`` or ``~/arduino/libraries`` and use
```
#include <SbsCAN.h>

byte canPort = 0; // Port 0 is on CANRX/TX, Port 1 is on DAC0 (CANRX1) und Pin 53 (CANTX1) of the Arduino Due
void setup() {
  // Set the serial interface baud rate
  Serial.begin(115200);
  
  // Initialize the CAN controller
  if(canInit(canPort, CAN_BPS_500K) == CAN_OK) {
    Serial.print("CAN"); Serial.print(canPort); Serial.print(": Initialized Successfully.\n\r");
  }else {
    Serial.print("CAN"); Serial.print(canPort); Serial.print(": Initialization Failed.\n\r");
  }
}
```
