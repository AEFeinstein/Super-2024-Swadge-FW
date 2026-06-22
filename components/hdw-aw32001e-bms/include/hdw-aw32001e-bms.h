/*! \file hdw-aw32001e-bms.h
 *
 * \section aw32001e-bms Design Philosophy
 *
 * This is one of a few BMS to be evaluated for use in the Swadge. The Swadge has historically been powered by 2x or 3x AA batteries, but due to mass and size concerns we are evaluating switching to a lithium cell.
 * The AW32001E is a BMS designed for a single lithium cell with the following features:
 * - VIN OVP (over-voltage protection) and UVP (under-voltage protection)
 * - Battery OVP
 * - Over-current protection
 * - Short-circuit protection
 * - Thermal protection
 * - PCB over-temperature protection
 * - Reverse leakage protection
 * - Pre-charge, constant-current fast charge, and constant-voltage regulation
 * - Simultaneous charging and system load support via PPMF (Power Path Management Function)
 * - Programmable charge current up to 500mA
 * - Programmable BATFET disconnect for shipping-mode
 * 
 * The protection circuit design supporting this BMS also includes:
 * - 500mA output current limit controlled by a PTC fuse
 * - ESD (electro-static discharge) protection via TVS diodes
 * - Reverse-voltage protection on system output
 * 
 * BMS code is (loosely) based on <a
 * href="https://github.com/arduino-libraries/Arduino_Nesso_N1">Arduino's Nesso N1 library, which uses this BMS.</a>.
 * 
 *
 * \section bms_usage Usage
 *
 * BMS parameters are set at the first flashing and should not be modified.
 * 
 * \section bms_example Example
 *
 * Set the BMS parameters to:
 * \code{.c}
 * TODO code;
 * \endcode
 */
#pragma once 
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <hal/gpio_types.h>
#include <soc/gpio_num.h>

#ifndef _HDW_AW32001E_BMS_H_

/**
 * @brief BMS register addresses
 */
typedef enum __attribute__((packed))
{
    INPUT_SRC = 0x00,        //VIN dynamic power management and IIN limit
    POWER_ON_CFG = 0x01,     //BAT disconnect delay time after WD timeout, shipping mode BAT disconnect, HIZ enable, charge enable, UVLO BAT threshold
    CHG_CURRENT = 0x02,      //REG and WD reset, charge current setting
    TERM_CURRENT = 0x03,     //BAT FET discharge current limit, pre-charge current limit
    CHG_VOLTAGE = 0x04,      //BAT charge voltage, BAT precharge voltage, recharge voltage threshold
    TIMER_WD = 0x05,         //Watchdog timer setting, charge termination, charge timer setting
    MAIN_CTRL = 0x06,        //NTC thermistor enable, shipping mode FET control, charge control, BAT OVP control
    SYS_CTRL = 0x07,         //system voltage control, disable PCB OTP, disable voltage input limit, thermal regulation threshold, VOUT threshold
    SYS_STATUS = 0x08,       //watchdog fault status, charge, PPM, power good, thermal regulation status
    FAULT_STATUS = 0x09,     //shipping mode delay time, VIN fault, thermal shutdown, BAT OVP, safety timer fault, NTC faults
    CHIP_ID = 0x0A,          //chip ID
} AW32001Reg;

/**
 * @brief BMS Charge Status
 */
  typedef enum __attribute__((packed))
  {
    NOT_CHARGING,
    PRE_CHARGE,
    CHARGING,
    FULL_CHARGE,
  } ChargeStatus;


/**
 * @brief BMS Under Voltage Lockout (UVLO) thresholds
 */

  typedef enum __attribute__((packed))
  {
    UVLO_2430mV,
    UVLO_2490mV,
    UVLO_2580mV,
    UVLO_2670mV,
    UVLO_2760mV,
    UVLO_2850mV,
    UVLO_2940mV,
    UVLO_3030mV,
  } UnderVoltageLockout;

/**
 * @brief BMS Junction Temperature Limit (TJ) thresholds
 */

  typedef enum __attribute__((packed))
  {
    TJ_60C,
    TJ_80C,
    TJ_100C,
    TJ_120C, //default setting, 120C
  } JunctionTemperatureLimit;

/**
 * @brief Input Current Limit in mA
 */

  typedef enum __attribute__((packed))
  {
    INPUTCURRENT_50mA,
    INPUTCURRENT_80mA,
    INPUTCURRENT_110mA,
    INPUTCURRENT_140mA,
    INPUTCURRENT_170mA,
    INPUTCURRENT_200mA,
    INPUTCURRENT_230mA,
    INPUTCURRENT_260mA,
    INPUTCURRENT_290mA,
    INPUTCURRENT_320mA,
    INPUTCURRENT_350mA, //do not go below 350mA for the Swadge, as the Swadge draws approximately 250mA max at 5V input and Fast Charge is set to 128mA default. Going below this limit will prevent Dynamic Power Mode from functioning properly and the USB input may not charge the Battery.
    INPUTCURRENT_380mA,
    INPUTCURRENT_410mA,
    INPUTCURRENT_440mA,
    INPUTCURRENT_470mA,
    INPUTCURRENT_500mA, //default setting, 500mA
  } InputCurrent;

/**
 * @brief Charge Rate Thresholds in mA
 */

  typedef enum __attribute__((packed))
  {
    FASTCHARGE_8mA = 0,
    FASTCHARGE_16mA = 1,
    FASTCHARGE_64mA = 7,
    FASTCHARGE_128mA = 15, //default setting
    FASTCHARGE_256mA = 31,
    FASTCHARGE_512mA = 63, //recommended swadge setting
  } FastChargeCurrent;

/**
 * @brief Discharge Current Thresholds in mA from BAT to System Output
 */

  typedef enum __attribute__((packed)) {
    DISCHARGE_200mA, //do not use, the Swadge draws approximately 250mA max at 5V input or ~275mA at 4.6V and ~340mA at 3.7V.
    DISCHARGE_400mA,
    DISCHARGE_600mA, //recommended Swadge setting
    DISCHARGE_800mA,
    DISCHARGE_1000mA,
    DISCHARGE_1200mA,
    DISCHARGE_1400mA,
    DISCHARGE_1600mA,
    DISCHARGE_1800mA,
    DISCHARGE_2000mA, //default setting
    DISCHARGE_2200mA,
    DISCHARGE_2400mA,
    DISCHARGE_2600mA,
    DISCHARGE_2800mA,
    DISCHARGE_3000mA,
    DISCHARGE_3200mA,
  } DischargeCurrent;

/**
 * @brief Input Voltage Clamp
 */

 typedef enum __attribute__((packed))
 {
    VIN_DPM_3880mV,
    VIN_DPM_3960mV,
    VIN_DPM_4040mV,
    VIN_DPM_4120mV,
    VIN_DPM_4200mV, 
    VIN_DPM_4280mV, 
    VIN_DPM_4360mV,
    VIN_DPM_4440mV,
    VIN_DPM_4520mV, //default setting; do not go below 4520mV without changing Charge Voltage Threshold to be >250mV below this value.
    VIN_DPM_4600mV, 
    VIN_DPM_4680mV,
    VIN_DPM_4760mV,
    VIN_DPM_4840mV,
    VIN_DPM_4920mV,
    VIN_DPM_5000mV,
    VIN_DPM_5080mV,
 } VinDPMVoltage;

/**
 * @brief Battery Charge Voltage Threshold in mV
 */

 typedef enum __attribute__((packed)) 
 {
    CHARGEVOLTAGE_3600mV = 0,
    CHARGEVOLTAGE_3750mV = 10,
    CHARGEVOLTAGE_3900mV = 20,
    CHARGEVOLTAGE_4050mV = 30,
    CHARGEVOLTAGE_4200mV = 40, //default setting
    CHARGEVOLTAGE_4350mV = 50,
    CHARGEVOLTAGE_4500mV = 60,
  } ChargeVoltageThreshold;

  /**
 * @brief System Regulated Output Voltage in mV
 */ 
typedef enum __attribute__((packed)) 
{
  SYSVOLTAGE_4200mV,
  SYSVOLTAGE_4250mV,
  SYSVOLTAGE_4300mV,
  SYSVOLTAGE_4350mV,
  SYSVOLTAGE_4400mV,
  SYSVOLTAGE_4450mV,
  SYSVOLTAGE_4500mV,
  SYSVOLTAGE_4550mV,
  SYSVOLTAGE_4600mV,   //default output voltage
  SYSVOLTAGE_4650mV,
  SYSVOLTAGE_4700mV,
  SYSVOLTAGE_4750mV,
  SYSVOLTAGE_4800mV,
  SYSVOLTAGE_4850mV,
  SYSVOLTAGE_4900mV,
  SYSVOLTAGE_4950mV,
} SystemRegulatedVoltage;

//==============================================================================
// Functions
//==============================================================================

  bool initBMS(gpio_num_t sda, gpio_num_t scl, gpio_pullup_t pullup);
  esp_err_t deinitBMS(void);
  static esp_err_t AW32001Set(int reg, uint8_t val);
  static esp_err_t AW32001Get(uint8_t* data, uint8_t reg);
  esp_err_t BMSSetRegistersAndReset(void);
  //ChargeStatus getChargeStatus();                               // get charge status
  //void enableCharge();                                          // enable charging
 

  //currently unused functions taken from the Arduino library:
  
  /*
  void setVinDPMVoltage(SystemRegulatedVoltage voltage);        // set input voltage limit
  void setIinLimitCurrent(FastChargeCurrent current);           // set input current limit, 50mA ~ 500mA(step 30mA, default 500mA)
  void setBatUVLO(UnderVoltageLockout uvlo);                    // set battery under voltage lockout(2430mV, 2490mV, 2580mV, 2670mV, 2760mV, 2850mV, 2940mV, 3030mV)
  void setChargeCurrent(FastChargeCurrent current);             // set charging current, 8mA ~ 456mA(step 8mA, default 128mA)
  void setDischargeCurrent(DischargeCurrent current);           // set discharging current, 200mA ~ 3200mA(step 200mA, default 2000mA)
  void setChargeVoltage(ChargeVoltageThreshold voltage);        // set charging voltage, 3600mV ~ 4545mV(step 15mV, default 4200mV)
  void setWatchdogTimer(uint8_t sec);                           // set charge watchdog timeout(0s, 40s, 80s, 160s, default 160s, 0 to disable)
  void feedWatchdog();                                          // feed watchdog timer
  void setThermistorControl(bool en);                           // set thermistor and PCB OTP mode (default enable NTC and enable PCB OTP)
  void setTJLimit(JunctionTemperatureLimit tj);                 // set junction temperature limit (default 120C)
  void setShipMode(bool en);                                    // set ship mode (default disable ship mode)
  void setHiZ(bool enable);                                     // set Hi-Z mode, true: USB -x-> SYS, false: USB -> SYS
  */
  
  
#endif 