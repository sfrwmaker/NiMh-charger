#ifndef _TWIN_CHARGER_H
#define _TWIN_CHARGER_H

#include "config.h"
#include "types.h"
#include "stat.h"
#include <OneWire.h>
#include <Time.h>
#include <TimeLib.h>

typedef enum {
    MODE_STOP = 0, MODE_CHARGE, MODE_PAUSE, MODE_DISCHARGE, MODE_WAS_CHARGE
} TWCH_MODE;

/*  The PID algorithm 
 *  Un = Kp*(Xs - Xn) + Ki*summ{j=0; j<=n}(Xs - Xj),
 *  Where Xs - is the preset current, Xn - the current on n-iteration step
 *  In this program the interactive formula is used:
 *    Un = Un-1 + Kp*(Xn-1 - Xn) + Ki*(Xs - Xn)
 *  With the first step:
 *  U0 = Kp*(Xs - X0) + Ki*(Xs - X0); Xn-1 = Xn;
 */
class PID {
    public:
        PID(void)                                           { }
        void        init(uint8_t denominator_p = 11);
        void        resetPID(void);                         // reset PID algorithm history parameters
        int32_t     reqPower(int16_t current_set, int16_t actual_current);
        int         changePID(uint8_t p, int k);            // set or get (if parameter < 0) PID parameter
    private:
        bool        reached         = false;                // The required current value has reached
        int16_t     curr_h0         = -1;                   // previously measured current
        int16_t     curr_h1         = -1;
        int32_t     i_summ          = 0;                    // Ki summary multiplied by denominator
        int32_t     power           = 0;                    // The power iterative multiplied by denominator
        int32_t     previous_power  = 0;                    // The power that was applied before multiplied by denominator
        int32_t     Kp              = 128;                  // The PID proportional coefficient multiplied by denominator.
        int32_t     Ki              = 50;                  // The PID integral coefficient multiplied by denominator.
        int16_t     denominator_p   = 9;                    // The common coefficient denominator power of 2 (9 means 512)
};

//------------------------- 16-bits timer TIM1 generates PWM signals on pins D9 & D10 -------------------------
class LongPWM {
    public:
        LongPWM()                                           { }
        void        init(void);
        void        duty(uint8_t index, uint16_t d);
        void        off(void)                               { OCR1A = OCR1B = 0; PORTB &= ~0b00000110; }
};

class TWCHARGER {
    public:
        TWCHARGER(uint8_t one_wire_pin, uint8_t fan_pin);
        void        init(void);
        uint8_t     discoverSensors(void);
        bool        setSensorAddress(uint8_t index, const uint8_t addr[8]);
        bool        getSensorAddress(uint8_t index, uint8_t addr[8]);
        int16_t     temperature(uint8_t index);             // The battery sensor temperature, 1/10 of Celsius
        uint16_t    mV(uint8_t index);                      // cached battery voltage, updated in expiration_period
        uint16_t    mA(uint8_t index);
        void        discharge(uint8_t index, bool on);
        void        pulseDischarge(uint8_t index, uint16_t ms);
        void        setChargeCurrent(uint8_t index, uint16_t mA);
        void        pauseCharging(uint8_t index, bool on = true);
        void        keepCurrent(uint8_t index);             // Returns the current
        bool        isBatteryConnected(uint8_t index, uint8_t iteration);  // Apply some to the battery and check current is greater than BATT_DETECT_CURRENT
        int         changePID(uint8_t idx, uint8_t p, int k) { return ch_pid[idx].changePID(p, k); }
        void        orderSensors(tSensorOrder order);
        bool        manageFan(void);
        void        fan(bool fan_on);
        void        initChargeCounter(uint8_t index)        { if (index < 2) charge_ctr[index] = charge_mAh[index] = 0; }
        void        initDischargeCounter(uint8_t index)     { if (index < 2) dische_ctr[index] = dische_mAh[index] = 0; }
        uint16_t    charged(uint8_t index)                  { return (index < 2)?charge_mAh[index]:0; }
        uint16_t    discharged(uint8_t index)               { return (index < 2)?dische_mAh[index]:0; }
    private:
        void        clearSensors(void);
        void        changeSensors(uint8_t x, uint8_t y);
        uint32_t    milliVolts(uint8_t pin);
        uint8_t     enable_pin[2];
        uint8_t     discharge_pin[2];                       // The pin used to activate discharge
        uint8_t     voltage_pin[2];                         // The pin to test the battery voltage
        uint8_t     current_pin[2];                         // The pin to test the current through the battery
        uint8_t     fan_pin;                                // The pin to manage the Heat sink FAN
        uint8_t     ds1820b_addr[3][8];                     // Address of the temperature sensor on the One Wire bus
        OneWire     ds;                                     // One Wire protocol pin
        time_t      voltage_update = 0;                     // When the voltage of both batteries should be updated
        uint16_t    voltage[2] = {0};                       // The battery voltage cache values
        time_t      temp_update = 0;                        // When the temperature of both batteries should be updated
        int16_t     temp[3] = {0};                          // The battery temperature cache values  
        TWCH_MODE   mode[2] = {MODE_STOP};                  // Charger mode
        uint16_t    current[2];                             // The preset charging current
        PID         ch_pid[2];
        LongPWM     pwm;
        bool        fan_on;                                 // Current fan status
        volatile uint32_t charge_ctr[2]   = {0};            // charge power counter
        volatile uint32_t dische_ctr[2]   = {0};            // discharge power counter
        volatile uint32_t charge_mAh[2]   = {0};            // charged mAh
        volatile uint32_t dische_mAh[2]   = {0};            // discharge mAh
        const uint32_t voltage_expiration = 10;             // The battery voltage should be updated in this period (secs)
        const uint32_t temp_expiration    = 29;             // The battery temperature should be updated in this period (secs)
        const uint8_t  avg_length         = 4;
        const uint32_t power_mAh          = 14400;          // 3600 * 4;
};

#endif
