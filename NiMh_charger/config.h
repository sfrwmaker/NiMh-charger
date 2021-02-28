#ifndef _CONFIG_H_
#define _CONFIG_H_

#define VERSION         (" 1.01")

// Aref voltage (setup by voltage source), mV
#define AREF_MV         (2487)

// The LCD I2C interface address
#define LCD_I2C_ADDR    (0x27)

// LCD brightness pin number (PWM capable)
#define LCD_BR_PIN      (6)
#define RENC_M_PIN      (2)
#define RENC_B_PIN      (3)
#define RENC_S_PIN      (4)

/* 
 *  The Twin Charger configuration
 *  2 enable pins are are used to switch the charger on/off 
 *  2 PWM pins (9 & 10) are used to manage the voltage applied th the charger (A - op-amp_5, B - op-amp_3)
 *  2 dischage pins
 *  4 Analog pins to control the battery voltage and the current through the battery
 *  1 temperature sensor pin of "One wire" DS18b20 sensors
 */
#define TWCH_ENABL_A    (7)
#define TWCH_ENABL_B    (8)
#define TWCH_DISCH_A    (5)
#define TWCH_DISCH_B    (11)
#define TWCH_VOLTG_A    (A0)
#define TWCH_VOLTG_B    (A2)
#define TWCH_CURRT_A    (A1)
#define TWCH_CURRT_B    (A3)
#define TWCH_FAN_PIN    (12)
#define TWCH_ONE_WIRE   (13)

// Comment out next line to disable log output on the serial port
#define LOG_ENABLE          (1)
// Comment out next line to disable log ouptut of battery voltage dump. Use it together with LOG_ENABLE
#define LOG_VOLTAGE_DUMP    (1)
// Log battery status period (seconds)
#define LOG_STATUS_PERIOD   (600)


// Charge resistors resistance, 1/10 Ohms. i.e. 31 for 3.1 Ohm
#define TWCH_CHARGE_RES_A   (30)
#define TWCH_CHARGE_RES_B   (31)
// Discharge resistors resistance, 1/10 Ohms
#define TWCH_DISCH_RES_A    (11)
#define TWCH_DISCH_RES_B    (10)

// Default battery capacity. Just for reference. Actual capacity can be adjusted through menu setup
#define BATT_CAPACITY           (2000)
// Minimum discharge current, otherwise stop discharging
#define MIN_DISCHARGE_CURRENT   (20)
// The minimum battery detect power (PWM)
#define BATT_DETECT_POWER       (4000)
// The maximum battery detect power (PWM)
#define MAX_BATT_DETECT_POWER   (7000)
// The minimum current value through battery to detect it
#define BATT_DETECT_CURRENT     (50)
// The minimum battery voltage to start detect the battery
#define BATT_DETECT_VOLTAGE     (100)
// The minimum battery voltage to discharge the battery
#define MIN_VOLTAGE             (900)
// The precarge voltage: minimul battery voltage to start refular charging
#define PRECHARGE_VOLTAGE       (1000)
// The maximum battery charging voltage on main phase
#define MAX_VOLTAGE             (1580)
// The maximum battery voltage in post charging phase (after main charinig phase)
#define BATT_POSTCHARGE_VOLTAGE (1590)

// The maximum temperature difference betwen start chagring one and finish charging
#define MAX_CHARGE_TEMP         (150)
// Hot temperature, when pause charging. 1/10 Celsius. i.e 395 for 39.5 degrees Celsius
#define HOT_TEMPERATURE         (500)
// Maximum temperature, when stop charging. 1/10 Celsius
#define MAX_TEMPERATURE         (550)

// Time required for slow charging, hours
#define SLOW_CHARGING_TIME		(10)

// Current values, mA
#define RESTORE_CURRENT         (40)
#define PRECHARGE_CURRENT       (30)
#define KEEP_CURRENT            (10)

// Discharging impulse time, ms
#define DISCHARGING_PULSE       (20)

// Heat sink temperature to turn on the FAN
#define HS_HOT_TEMP             (500)
// Heat sink temperature difference to on-off FAN, Celsius * 10
#define HS_DIFF_TEMP            (50)
// Heat sink overheat temperature. Emergency turn-off
#define HS_OVERHEAT             (600)

// LCD Celsius degree code (aka ASCII)
#define DEGREE_CODE             (223)

#endif
