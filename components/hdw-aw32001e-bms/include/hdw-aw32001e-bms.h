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
 * BMS code is based on <a
 * href="https://github.com/arduino-libraries/Arduino_Nesso_N1">Arduino's Nesso N1 library, which uses this BMS.</a>.
 * 
 *
 * \section bms_usage Usage
 *
 * BMS parameters are set at the first flashing and should not be modified.
 * 
 *
 *
 * \section bms_example Example
 *
 * Set the BMS parameters to:
 * \code{.c}
 * TODO code;
 * \endcode
 */

#ifndef _HDW_AW32001E_BMS_H_
#define _HDW_AW32001E_BMS_H_

#include <stdint.h>
#include <hal/gpio_types.h>
#include <soc/gpio_num.h>
#include <esp_err.h>

/**
 * @brief BMS register addresses
 */
typedef enum {
    AW3200_INPUT_SRC = 0x00,        //VIN dynamic power management and IIN limit
    AW3200_POWER_ON_CFG = 0x01,     //BAT disconnect delay time after WD timeout, shipping mode BAT disconnect, HIZ enable, charge enable, UVLO BAT threshold
    AW3200_CHG_CURRENT = 0x02,      //REG and WD reset, charge current setting
    AW3200_TERM_CURRENT = 0x03,     //BAT FET discharge current limit, pre-charge current limit
    AW3200_CHG_VOLTAGE = 0x04,      //BAT charge voltage, BAT precharge voltage, recharge voltage threshold
    AW3200_TIMER_WD = 0x05,         //Watchdog timer setting, charge termination, charge timer setting
    AW3200_MAIN_CTRL = 0x06,        //NTC thermistor enable, shipping mode FET control, charge control, BAT OVP control
    AW3200_SYS_CTRL = 0x07,         //system voltage control, disable PCB OTP, disable voltage input limit, thermal regulation threshold, VOUT threshold
    AW3200_SYS_STATUS = 0x08,       //watchdog fault status, charge, PPM, power good, thermal regulation status
    AW3200_FAULT_STATUS = 0x09,     //shipping mode delay time, VIN fault, thermal shutdown, BAT OVP, safety timer fault, NTC faults
    AW3200_CHIP_ID = 0x0A,          //chip ID
} AW32001Reg;

/**
 * @brief BMS Charge Status
 */
  typedef enum {
    NOT_CHARGING,
    PRE_CHARGE,
    CHARGING,
    FULL_CHARGE,
  } ChargeStatus;


/**
 * @brief BMS Under Voltage Lockout (UVLO) thresholds
 */

  typedef enum {
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

  typedef enum {
    TJ_60C,
    TJ_80C,
    TJ_100C,
    TJ_120C, //default setting, 120C
  } JunctionTemperatureLimit;

/**
 * @brief Charge Rate Thresholds in mA
 */

  typedef enum {
    FastCharge_8 = 0,
    FastCharge_16 = 1,
    FastCharge_64 = 7,
    FastCharge_128 = 15, //default setting, 128mA
    FastCharge_256 = 31,
    FastCharge_512 = 63,
  } FastChargeCurrent;

/**
 * @brief Discharge Current Thresholds in mA from BAT to System Output
 */

  typedef enum {
    Discharge_200mA, //do not use, the Swadge draws approximately 250mA max at 5V input or ~275mA at 4.6V and ~340mA at 3.7V.
    Discharge_400mA,
    Discharge_600mA, //recommended Swadge setting
    Discharge_800mA,
    Discharge_1000mA,
    Discharge_1200mA,
    Discharge_1400mA,
    Discharge_1600mA,
    Discharge_1800mA,
    Discharge_2000mA, //default setting
    Discharge_2200mA,
    Discharge_2400mA,
    Discharge_2600mA,
    Discharge_2800mA,
    Discharge_3000mA,
    Discharge_3200mA,
  } DischargeCurrent;

/**
 * @brief Battery Charge Voltage Threshold in mV
 */

 typedef enum {
    ChargeVoltage_3600mV = 0,
    ChargeVoltage_3750mV = 10,
    ChargeVoltage_3900mV = 20,
    ChargeVoltage_4050mV = 30,
    ChargeVoltage_4200mV = 40, //default setting
    ChargeVoltage_4350mV = 50,
    ChargeVoltage_4500mV = 60,
  } ChargeVoltageThreshold;

  /**
 * @brief System Regulated Output Voltage in mV
 */ 
typedef enum {
  SysVoltage4200,
  SysVoltage4250,
  SysVoltage4300,
  SysVoltage4350,
  SysVoltage4400,
  SysVoltage4450,
  SysVoltage4500,
  SysVoltage4550,
  SysVoltage4600,   //default output voltage
  SysVoltage4650,
  SysVoltage4700,
  SysVoltage4750,
  SysVoltage4800,
  SysVoltage4850,
  SysVoltage4900,
  SysVoltage4950,
} SystemRegulatedVoltage;

esp_err_t initBMS(FastChargeCurrent current, ChargeVoltageThreshold voltage, UnderVoltageLockout uvlo, JunctionTemperatureLimit tj, SystemRegulatedVoltage sys_voltage, uint8_t timeout);


  void enableCharge();                                          // enable charging
  void setChargeEnable(bool enable);                            // charge control
  void setVinDPMVoltage(SystemRegulatedVoltage voltage);        // set input voltage limit
  void setIinLimitCurrent(FastChargeCurrent current);           // set input current limit, 50mA ~ 500mA(step 30mA, default 500mA)
  void setBatUVLO(UnderVoltageLockout uvlo);                    // set battery under voltage lockout(2430mV, 2490mV, 2580mV, 2670mV, 2760mV, 2850mV, 2940mV, 3030mV)
  void setChargeCurrent(FastChargeCurrent current);             // set charging current, 8mA ~ 456mA(step 8mA, default 128mA)
  void setDischargeCurrent(DischargeCurrent current);           // set discharging current, 200mA ~ 3200mA(step 200mA, default 2000mA)
  void setChargeVoltage(ChargeVoltageThreshold voltage);        // set charging voltage, 3600mV ~ 4545mV(step 15mV, default 4200mV)
  void setWatchdogTimer(uint8_t sec);                           // set charge watchdog timeout(0s, 40s, 80s, 160s, default 160s, 0 to disable)
  void feedWatchdog();                                          // feed watchdog timer
  void setShipMode(bool en);                                    // set ship mode (default disable ship mode)
  void setThermistorControl(bool en);                           // set thermistor and PCB OTP mode (default enable NTC and enable PCB OTP)
  void setTJLimit(JunctionTemperatureLimit tj);                 // set junction temperature limit (default 120C)
  ChargeStatus getChargeStatus();                               // get charge status
  void setHiZ(bool enable);                                     // set Hi-Z mode, true: USB -x-> SYS, false: USB -> SYS


#endif 