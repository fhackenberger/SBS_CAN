// This was tested with:
// Arduino 101 on pins 0/1 (Serial1) for the serial connection to the CAN module
// Arduino Due on pins 19/18 (Serial1) for the serial connection to the CAN module
// Longan Serial CAN Bus 1.0 (MCP2551 and MCP2515 chips) https://docs.longan-labs.cc/can_bus/
// Wiring as documented in "2018-07-20 Report Leader 50V Battery CAN Readings": https://docs.google.com/document/d/1OmkW_UTGWCwyDriThNI7dI2qDYE3IVobrKOOHKtsnHs/edit#
// The hardware-serial branch of the CAN serial library https://github.com/fhackenberger/Serial_CAN_Arduino revision https://github.com/fhackenberger/Serial_CAN_Arduino/commit/2376ae2617c075af54695e48a02da84138ba8f14
//
// ATTENTION: During testing, after a re-flash, I have to completely powerdown the Arduino and the battery (two long presses) before powering it up again.
#ifndef _H_SBS_CAN_
#define _H_SBS_CAN_

#include "DueCANLayer.h"
// Expose CAN Layer functions from DueCANLayer.cpp
extern byte canInit(byte cPort, long lBaudRate);
extern byte canTx(byte cPort, long lMsgID, bool bExtendedFormat, byte* cData, byte cDataLen);
extern byte canRx(byte cPort, long* lMsgID, bool* bExtendedFormat, byte* cData, byte* cDataLen);

// The states that can be set on the BMS
static const unsigned int BMS_CTRL_STATE_INACTIVE  = 0;
static const unsigned int BMS_CTRL_STATE_DISCHARGE = 1;
static const unsigned int BMS_CTRL_STATE_CHARGE = 2;
static const unsigned int BMS_CTRL_STATE_ALARM_MODE = 3; // Infinite 12V active time, no reaction to button and charge-input
static const unsigned int BMS_CTRL_STATE_12V_MODE = 5;
static const unsigned int BMS_CTRL_STATE_DEEP_SLEEP = 6;

// The various states of the BMS as stored in the bmsState field
static const unsigned int BMS_STATE_INIT1 = 1;
static const unsigned int BMS_STATE_INIT2 = 2;
static const unsigned int BMS_STATE_INIT3 = 3;
static const unsigned int BMS_STATE_INIT4 = 4;
static const unsigned int BMS_STATE_IDLE  = 5;
static const unsigned int BMS_STATE_DISCHARGE = 6;
static const unsigned int BMS_STATE_CHARGE = 7;
static const unsigned int BMS_STATE_FAULT = 10;
static const unsigned int BMS_STATE_CRIT_ERR = 11;
static const unsigned int BMS_STATE_PREPARE_DEEPSLEEP = 99; // Boardnet supply will cut-off in 2 seconds
static const unsigned int BMS_STATE_DEEPSLEEP = 100;

/** Holds the state of the BMS / battery in one class. This structure is filled by receiving multiple messages */
class BMSState {
  public:
  unsigned long msgCountInfo01 = 0;
  unsigned long msgCountInfo02 = 0;
  unsigned long msgCountInfo03 = 0;
  unsigned long msgCountInfo04 = 0;
  unsigned long msgCountInfo05 = 0;
  unsigned long msgCountInfo06 = 0;
  float packVoltage;
  float packCurrent;
  unsigned int bmsState; // Shows different states of operation of the BMS state machine, see BMS_STATE_XXX
  int stateOfCharge; // Actual state of charge, percentage from 0-100
  unsigned int stateOfHealth; // State of health - unclear from the spec what this exactly means
  unsigned int remainingCapacity_mAh; // Remaining capacity
  unsigned int lastFullCapacity_mAh; // Full charge capacity. Adapts on each full charge cycles that start with a completly depleted pack)
  unsigned int cellVolt01; // actual cell-voltage, cell might not be installed depending on pack configuration
  unsigned int cellVolt02;
  unsigned int cellVolt03;
  unsigned int cellVolt04;
  unsigned int cellVolt05;
  unsigned int cellVolt06;
  unsigned int cellVolt07;
  unsigned int cellVolt08;
  unsigned int cellVolt09;
  unsigned int cellVolt10;
  unsigned int cellVolt11;
  unsigned int cellVolt12;
  unsigned int cellVolt13;
  unsigned int cellVolt14;
  unsigned int cellBalBits1; // Shows in bit-coded style which cell of the referring device is being balanced. These parameters can be seen as three front-ends. They are used to control the 14 serial cells.
  unsigned int cellBalBits2;
  unsigned int cellBalBits3;
  int tempPowerstage1; // Actual powerstage temperature measured
  int tempPowerstage2; // Actual powerstage temperature measured
  int tempMCU; // Actual microcontroller temperature measured
  int tempCell1; // Actual cell temperature measured
  int tempCell2; // Actual cell temperature measured
  unsigned int errField; // Holds the error bits, useful for a quick check if errors are present
  bool errTempPowerstage1; // Temperature at powerstage sensor 1 above limits
  bool errTempPowerstage2; // Temperature at powerstage sensor 2 above limits
  bool errChargeCurrent; // Charge current above limits
  bool errDischargeCurrent; // Discharge current above limits
  bool errPackVoltMax; // Sum of cell voltages (pack voltage) above max
  bool errAnalogOverVolt; // Overvoltage detected (analog)
  bool errCurrSensorOffset; // Offset of current measurement out of range
  bool errEeprom; // Error in EEPROM
  bool errCmCrc; // CRC-error in cell-monitoring comunication to host controller
  bool errExternalEnable; // External enable-input inhibtis output activation
  bool errCmAlert; // Alert from cell monitoring
  bool errCmFault; // Fault from cell monitoring
  bool errPowerstage; // Fault signal from powerstage driver
  bool errPreCharge; // Error during PreCharge. Reduce load!
  bool errOutputVoltHigh; // Voltage at charge/discharge terminal above limits
  bool errPackVoltMin; // Sum of cell voltages (pack voltage) below min
  bool errDischargeVolt; // Lowest cell voltage below minimum discharge limit
  bool errCmCellUntervolt; // Undervoltage on one or more cells detected by cell monitoring
  bool errCmCellOvervolt; // Overvoltage on one or more cells detected by cell monitoring
  bool errAnalogOvercurrent; // Overcurrent detected (analog)
  bool errOvertempCharge; // Temperature while charging above limits
  bool errOvertempDischarge; // Temperature while discharging above limits
  bool errUndertempCharge; // Temperature while charging below min
  bool errUnsertempDischarge; // Temperature while discharging below min
  bool errCurrFlowPassiveState; // Current flow detected when no current should flow
  bool errCanTimeout; // CAN Timeout detected
  bool errChargePlugDetection; // 1=plug detected

  /** Gets a String version of the bmsState for display */
  String getBmsStateStr();
  /** Returns if there is at least one error state set */
  bool isErrActive();
  /** Decodes a BMS CAN message and updates the data in this class
   * @returns the ID (according to spec 1-6) of the valid info message if one was detected, -1 if no message was detected */
  int decodeMsg(unsigned long id, unsigned char* data, unsigned int dataLen);
  /** Encodes a state change command into a CAN message
   * @param canState: One of the BMS_CTRL_STATE_ constants */
  void encodeSetStateMsg(unsigned int canState, unsigned long& id, unsigned char* msgData);
  /** Decodes a string into a BMS_CTRL_STATE_ constant
   * @param bmsCtrlStateStr The string to convert (without the BMS_CTRL_STATE_ prefix)
   * @returns UINT_MAX if no matching state was found */
  static unsigned int bmsCtrlStateFromStr(String bmsCtrlStateStr);
};
#endif
// END FILE

