// This was tested with:
// Arduino 101 on pins 0/1 (Serial1) for the serial connection to the CAN module
// Arduino Due on pins 19/18 (Serial1) for the serial connection to the CAN module
// Longan Serial CAN Bus 1.0 (MCP2551 and MCP2515 chips) https://docs.longan-labs.cc/can_bus/
// Wiring as documented in "2018-07-20 Report Leader 50V Battery CAN Readings": https://docs.google.com/document/d/1OmkW_UTGWCwyDriThNI7dI2qDYE3IVobrKOOHKtsnHs/edit#
// The hardware-serial branch of the CAN serial library https://github.com/fhackenberger/Serial_CAN_Arduino revision https://github.com/fhackenberger/Serial_CAN_Arduino/commit/2376ae2617c075af54695e48a02da84138ba8f14
//
// ATTENTION: During testing, after a re-flash, I have to completely powerdown the Arduino and the battery (two long presses) before powering it up again.
#include "SbsCAN.h"

/** According to Spec: Smart Battery Solutions "Specification CAN Bus (General) Project: 10bis14s BMS", 29.07.2015 */
unsigned long BMS_INFO_IDS[] = {
  0x171, 0x172, 0x173, 0x174, 0x175, 0x176 // CAN message IDs for the various informational messages the battery sends
};
unsigned long BMS_CTRL_ID = 0x160; // CAN message for the control message to switch switch the battery state
// Bitmasks for various bits within the CAN messages
char MASK_BMS_ERR_Temp_Powerstage_1       = 0b00000001;
char MASK_BMS_ERR_Temp_Powerstage_2       = 0b00000010;
char MASK_BMS_ERR_Charge_Current          = 0b00000100;
char MASK_BMS_ERR_Discharge_Current       = 0b00001000;
char MASK_BMS_ERR_Pack_Voltage_Max        = 0b00010000;
char MASK_BMS_ERR_Analog_Overvoltage      = 0b00100000;
char MASK_BMS_ERR_Curr_Sensor_Offset      = 0b01000000;
char MASK_BMS_ERR_EEPROM                  = 0b10000000;
char MASK_BMS_ERR_CM_CRC                  = 0b00000001;
char MASK_BMS_ERR_External_Enable         = 0b00000010;
char MASK_BMS_ERR_CM_Alert                = 0b00000100;
char MASK_BMS_ERR_CM_Fault                = 0b00001000;
char MASK_BMS_ERR_Powerstage              = 0b00010000;
char MASK_BMS_ERR_PreCharge               = 0b00100000;
char MASK_BMS_ERR_Output_Voltage_High     = 0b01000000;
char MASK_BMS_ERR_Pack_Voltage_Min        = 0b10000000;
char MASK_BMS_ERR_Discharge_Voltage       = 0b00000001;
char MASK_BMS_ERR_CM_Cell_Undervoltage    = 0b00000010;
char MASK_BMS_ERR_CM_Cell_Overvoltage     = 0b00000100;
char MASK_BMS_ERR_Analog_Overcurrent      = 0b00001000;
char MASK_BMS_ERR_Overtemp_Charge         = 0b00010000;
char MASK_BMS_ERR_Overtemp_Discharge      = 0b00100000;
char MASK_BMS_ERR_Undertemp_Charge        = 0b01000000;
char MASK_BMS_ERR_Undertemp_Discharge     = 0b10000000;
char MASK_BMS_ERR_Curr_Flow_Passive_State = 0b00000001;
char MASK_BMS_ERR_CAN_Timeout             = 0b00000010;
char MASK_BMS_Charge_Plug_Detection       = 0b00000100;

unsigned int bytesToUInt16(unsigned char* bytes, int startIdx) {
  unsigned int val = 0;
  val = bytes[startIdx] + (bytes[startIdx+1] << 8);
  return val;
}
unsigned int bytesToInt16(unsigned char* bytes, int startIdx) {
  int val = 0;
  val = bytes[startIdx] + (bytes[startIdx+1] << 8);
  return val;
}
// Used for debugging, put it into the decodeMsg method if required
/*
void printMsg(unsigned long id, unsigned char data[8]) {
  Serial.print("DATA FROM ID: ");
  Serial.println(id, HEX);
  for(int i=0; i<8; i++) {
    Serial.print("0x");
    Serial.print(data[i], HEX);
    Serial.print('\t');
  }
  Serial.println();
}*/

int BMSState::decodeMsg(unsigned long id, unsigned char* data, unsigned int dataLen) {
  if(id == BMS_INFO_IDS[0]) { // BMS_Info_01 message
    // TODO Validate status bits, BMS_Charge_Plug_Detection might help to test the code
    this->packVoltage = bytesToUInt16(data, 0) * 0.0078125; // Factor according to battery CAN spec page 6
    this->packCurrent = bytesToUInt16(data, 2) * 0.03125; // Factor according to battery CAN spec page 6
    this->errTempPowerstage1      = data[4] & MASK_BMS_ERR_Temp_Powerstage_1 ? true : false;
    this->errTempPowerstage2      = data[4] & MASK_BMS_ERR_Temp_Powerstage_2 ? true : false;
    this->errChargeCurrent        = data[4] & MASK_BMS_ERR_Charge_Current ? true : false;
    this->errDischargeCurrent     = data[4] & MASK_BMS_ERR_Discharge_Current ? true : false;
    this->errPackVoltMax          = data[4] & MASK_BMS_ERR_Pack_Voltage_Max ? true : false;
    this->errAnalogOverVolt       = data[4] & MASK_BMS_ERR_Analog_Overvoltage ? true : false;
    this->errCurrSensorOffset     = data[4] & MASK_BMS_ERR_Curr_Sensor_Offset ? true : false;
    this->errEeprom               = data[4] & MASK_BMS_ERR_EEPROM ? true : false;
    this->errCmCrc                = data[5] & MASK_BMS_ERR_CM_CRC ? true : false;
    this->errExternalEnable       = data[5] & MASK_BMS_ERR_External_Enable ? true : false;
    this->errCmAlert              = data[5] & MASK_BMS_ERR_CM_Alert ? true : false;
    this->errCmFault              = data[5] & MASK_BMS_ERR_CM_Fault ? true : false;
    this->errPowerstage           = data[5] & MASK_BMS_ERR_Powerstage ? true : false;
    this->errPreCharge            = data[5] & MASK_BMS_ERR_PreCharge ? true : false;
    this->errOutputVoltHigh       = data[5] & MASK_BMS_ERR_Output_Voltage_High ? true : false;
    this->errPackVoltMin          = data[5] & MASK_BMS_ERR_Pack_Voltage_Min ? true : false;
    this->errDischargeVolt        = data[6] & MASK_BMS_ERR_Discharge_Voltage ? true : false;
    this->errCmCellUntervolt      = data[6] & MASK_BMS_ERR_CM_Cell_Undervoltage ? true : false;
    this->errCmCellOvervolt       = data[6] & MASK_BMS_ERR_CM_Cell_Overvoltage ? true : false;
    this->errAnalogOvercurrent    = data[6] & MASK_BMS_ERR_Analog_Overcurrent ? true : false;
    this->errOvertempCharge       = data[6] & MASK_BMS_ERR_Overtemp_Charge ? true : false;
    this->errOvertempDischarge    = data[6] & MASK_BMS_ERR_Overtemp_Discharge ? true : false;
    this->errUndertempCharge      = data[6] & MASK_BMS_ERR_Undertemp_Charge ? true : false;
    this->errUnsertempDischarge   = data[6] & MASK_BMS_ERR_Undertemp_Discharge ? true : false;
    this->errCurrFlowPassiveState = data[7] & MASK_BMS_ERR_Curr_Flow_Passive_State ? true : false;
    this->errCanTimeout           = data[7] & MASK_BMS_ERR_CAN_Timeout ? true : false;
    this->errChargePlugDetection  = data[7] & MASK_BMS_Charge_Plug_Detection ? true : false;
    return 1;
  }else if(id == BMS_INFO_IDS[1]) { // BMS_Info_02 message
    //printMsg(id, data);
    this->bmsState = bytesToUInt16(data, 0);
    this->stateOfCharge = data[2];
    this->stateOfHealth = 0.5 * (unsigned int)data[3]; // Factor according to battery CAN spec page 9
    this->remainingCapacity_mAh = bytesToUInt16(data, 4);
    this->lastFullCapacity_mAh = bytesToUInt16(data, 6);
    return 2;
  }else if(id == BMS_INFO_IDS[2]) { // BMS_INFO_03 message
    this->cellVolt01 = data[0];
    this->cellVolt02 = data[1];
    this->cellVolt03 = data[2];
    this->cellVolt04 = data[3];
    this->cellVolt05 = data[4];
    this->cellVolt06 = data[5];
    this->cellVolt07 = data[6];
    this->cellVolt08 = data[7];
    return 3;
  }else if(id == BMS_INFO_IDS[3]) { // BMS_INFO_04 message
    this->cellVolt09 = data[0];
    this->cellVolt10 = data[1];
    this->cellVolt11 = data[2];
    this->cellVolt12 = data[3];
    this->cellVolt13 = data[4];
    this->cellVolt14 = data[5];
    return 4;
  }else if(id == BMS_INFO_IDS[4]) { // BMS_INFO_05 message
    this->cellBalBits1 = data[0]; // Cells  1-6
    this->cellBalBits2 = data[1]; // Cells  7-11
    this->cellBalBits3 = data[2]; // Cells 12-14
    return 5;
  }else if(id == BMS_INFO_IDS[5]) { // BMS_INFO_06 message
    this->tempPowerstage1 = bytesToInt16(data, 0);
    this->tempPowerstage2 = bytesToInt16(data, 2);
    this->tempMCU = bytesToInt16(data, 4);
    this->tempCell1 = data[6];
    this->tempCell2 = data[7];
    return 6;
  }else {
    return -1;
  }
  return -1;
}

void BMSState::encodeSetStateMsg(unsigned int canState, unsigned long id, unsigned char* msgData) {
  msgData = { 0 };
  msgData[0] = canState;
  id = BMS_CTRL_ID;
  return;
}
// END FILE

