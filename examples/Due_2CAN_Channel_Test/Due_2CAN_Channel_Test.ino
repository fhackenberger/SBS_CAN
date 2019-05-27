// This was tested with:
// Arduino Due on pins CANRX/TX connected to the CAN transceiver
// CJMCU-1051 based on NXP TJA1051 CAN Transceiver
// Wiring as documented in "2018-07-20 Report Leader 50V Battery CAN Readings": https://docs.google.com/document/d/1OmkW_UTGWCwyDriThNI7dI2qDYE3IVobrKOOHKtsnHs/edit#
// SbsCAN.h and its dependencies are from https://github.com/fhackenberger/SBS_CAN @ 7de365e
#include <SbsCAN.h>

byte canPort = 0; // Port 0 is on CANRX/TX, Port 1 is on DAC0 (CANRX1) und Pin 53 (CANTX1) of the Arduino Due
unsigned long lastKeepAliveMs;
unsigned long lastValidMsgMs = 0L;
unsigned long highPowerOnSinceMs;
unsigned long battOnSinceMs = 0L;
long id = 0;
byte dta[8];
unsigned int canStateOut = BMS_CTRL_STATE_INACTIVE;
long idOut = 0;
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
  // XXX force sending the 48V enable msg for testing
  bmsState.bmsState = BMS_STATE_IDLE;
}

void loop() {
  byte cDataLen;
  unsigned long now = millis();
  // Check for received message
  if(canRx(canPort, &id, false, &dta[0], &cDataLen) == CAN_OK) {
    int msgRcv = bmsState.decodeMsg(id, dta, 8);
    if(msgRcv > 0)
      lastValidMsgMs = now;
    if(msgRcv == 2) {
      Serial.print("Info02: State "); Serial.print(bmsState.bmsState); Serial.print(" SoC "); Serial.print(bmsState.stateOfCharge); Serial.print(" SoH "); Serial.print(bmsState.stateOfHealth); Serial.print(" remCapa "); Serial.print(bmsState.remainingCapacity_mAh); Serial.print("mAh lastFullCapa "); Serial.print(bmsState.lastFullCapacity_mAh); Serial.println("mAh");
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
    Serial.print("checking for CAN data... Last valid msg "); Serial.print(millis() - lastValidMsgMs); Serial.println("ms ago");
    lastKeepAliveMs = millis();
    if(battOnSinceMs != 0L && (now - battOnSinceMs) >= (60 * 1000)) {
      canStateOut = BMS_CTRL_STATE_DEEP_SLEEP;
      dbgMsg = "put battery to sleep";
      sendCtrlMsg = true;
    }else if(highPowerOnSinceMs == 0L && bmsState.bmsState == BMS_STATE_IDLE) {
      canStateOut = BMS_CTRL_STATE_DISCHARGE;
      dbgMsg = "enable High Voltage output";
      sendCtrlMsg = true;
    }else if((now - highPowerOnSinceMs) > (10 * 1000)) {
      canStateOut = BMS_CTRL_STATE_INACTIVE;
      dbgMsg = "disable High Voltage output";
      sendCtrlMsg = true;
    }
    if(sendCtrlMsg) {
      bmsState.encodeSetStateMsg(canStateOut, idOut, dtaOut);
      if(canTx(canPort, idOut, false, dtaOut, 8) != CAN_OK)
        Serial.println("Sending ctrl message failed");
      Serial.print("Sent ctrl message to "); Serial.print(dbgMsg); Serial.println("");
    }
  }
}
