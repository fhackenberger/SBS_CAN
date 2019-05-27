// This was tested with:
// Arduino Due on pins CANRX/TX connected to the CAN transceiver
// CJMCU-1051 based on NXP TJA1051 CAN Transceiver
// Wiring as documented in "2018-07-20 Report Leader 50V Battery CAN Readings": https://docs.google.com/document/d/1OmkW_UTGWCwyDriThNI7dI2qDYE3IVobrKOOHKtsnHs/edit#
// SbsCAN.h and its dependencies are from https://github.com/fhackenberger/SBS_CAN @ 7de365e
#include <SbsCAN.h>

byte canPort = 0; // Port 0 is on CANRX/TX, Port 1 is on DAC0 (CANRX1) und Pin 53 (CANTX1) of the Arduino Due
int outLioIonBtnPin = 24; // The Arduino pin which is connected to the Bat-enable pin of the battery. Pull to GND (LOW) to simulate pressing on the button
unsigned long battEnPushedMs = 0L;
unsigned long lastKeepAliveMs;
unsigned long lastValidMsgMs = 0L;
unsigned long highPowerOnSinceMs;
unsigned long battOnSinceMs = 0L;
long id = 0;
byte dta[8];
unsigned int canStateOut = BMS_CTRL_STATE_INACTIVE;
unsigned long idOut = 0;
byte dtaOut[8] = {0};
BMSState bmsState;

void setup() {
  // Set the serial interface baud rate
  Serial.begin(115200);
  
  // Initialize the CAN controller
  if(canInit(canPort, CAN_BPS_500K) == CAN_OK) {
    Serial.print("CAN"); Serial.print(canPort); Serial.print(": Initialized Successfully.\n\r");
  }else {
    Serial.print("CAN"); Serial.print(canPort); Serial.print(": Initialization Failed.\n\r");
  }
  lastKeepAliveMs = millis();
  lastValidMsgMs = 0;
  pinMode(outLioIonBtnPin, INPUT); // Don't press the button (setting the output to HIGH does not work as expected after it was LOW)
  // XXX force sending the 48V enable msg for testing
  bmsState.bmsState = BMS_STATE_IDLE;
}

void loop() {
  byte cDataLen;
  unsigned long now = millis();
  if((lastValidMsgMs == 0L || (now - lastValidMsgMs) > 10000) && // No CAN msg or dead for more than 10 seconds
      now > 2500 && // Wait for a CAN msg 2.5s after booting
      (battEnPushedMs == 0L || (now - battEnPushedMs) > 5000)) { // Retry 5s after the last try, if still no CAN msg
    // 2.5 seconds running and still no CAN msg, battery is probably in deep sleep
    Serial.println("Waking up the battery using battery enable line");
    pinMode(outLioIonBtnPin, OUTPUT);
    digitalWrite(outLioIonBtnPin, LOW);
    delay(300);
    pinMode(outLioIonBtnPin, INPUT);
    battEnPushedMs = now;
  }
  // Check for received message
  if(canRx(canPort, &id, false, &dta[0], &cDataLen) == CAN_OK) {
    int msgRcv = bmsState.decodeMsg(id, dta, 8);
    if(msgRcv > 0)
      lastValidMsgMs = now;
    if(msgRcv == 2) {
      Serial.print("Info02: State "); Serial.print(bmsState.getBmsStateStr()); Serial.print(" SoC "); Serial.print(bmsState.stateOfCharge); Serial.print(" SoH "); Serial.print(bmsState.stateOfHealth); Serial.print(" remCapa "); Serial.print(bmsState.remainingCapacity_mAh); Serial.print("mAh lastFullCapa "); Serial.print(bmsState.lastFullCapacity_mAh); Serial.println("mAh");
    }
  }
  if(battOnSinceMs == 0L && lastValidMsgMs != 0L)
    battOnSinceMs = now;
  // Cycle the battery between high power on for 10 seconds and off for 2 seconds for 60 seconds
  if(bmsState.bmsState == BMS_STATE_DISCHARGE && highPowerOnSinceMs == 0L)
    highPowerOnSinceMs = now;
  else if(bmsState.bmsState != BMS_STATE_DISCHARGE)
    highPowerOnSinceMs = 0L;
  if((millis() - lastKeepAliveMs) > 2000) {
    String dbgMsg = "";
    boolean sendCtrlMsg = false;
    Serial.print("checking for CAN data... Last valid msg "); Serial.print(millis() - lastValidMsgMs); Serial.print("ms ago. Err present: "); Serial.println(bmsState.isErrActive() ? "true" : "false");
    lastKeepAliveMs = millis();
    if(battOnSinceMs != 0L && (now - battOnSinceMs) >= (60 * 1000)) { // If battery was on for more than 60 seconds
      canStateOut = BMS_CTRL_STATE_DEEP_SLEEP;
      dbgMsg = "put battery to sleep";
      sendCtrlMsg = true;
    }else if(highPowerOnSinceMs == 0L && bmsState.bmsState == BMS_STATE_IDLE) {
      canStateOut = BMS_CTRL_STATE_DISCHARGE;
      dbgMsg = "enable High Voltage output";
      sendCtrlMsg = true;
    }else if((now - highPowerOnSinceMs) > (10 * 1000)) { // If high power was on for more than 10 seconds
      canStateOut = BMS_CTRL_STATE_INACTIVE;
      dbgMsg = "disable High Voltage output";
      sendCtrlMsg = true;
    }
    if(sendCtrlMsg) {// && (now - lastValidMsgMs) < 5000) {
      bmsState.encodeSetStateMsg(canStateOut, idOut, dtaOut);
      if(canTx(canPort, idOut, false, dtaOut, 8) != CAN_OK)
        Serial.println("Sending ctrl message failed");
      Serial.print("Sent ctrl message to battery for state "); Serial.println(canStateOut);
    }
  }
}
