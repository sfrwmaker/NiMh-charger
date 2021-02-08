#ifndef _BATTERY_H_
#define _BATTERY_H_
#include <Arduino.h>
#include <Time.h>
#include <TimeLib.h>
#include "types.h"
#include "stat.h"
#include "config.h"

#define B_MV_SIZE    (16)
#define B_MA_SIZE    (4)

class BATTERY {
    public:
        BATTERY(void) : mV(B_MV_SIZE), mA(B_MA_SIZE)        { }
        void        init(uint16_t mAh, tChargeType type, uint8_t loops);
        tChargeType schedule(void)                          { return charge_type; }
        uint8_t     phaseIndex(void)                        { return phase_index; }
        void        setPhaseIndex(uint8_t phase)            { phase_index = phase; }
        uint16_t    averageCurrent(void)                    { return mA.read(); }
        uint16_t    averageVoltage(void)                    { return mV.read(); }
        uint16_t    capacity(void)                          { return mAh; }
        bool        togglePause(void)                       { pause = !pause; return pause; }
        void        setPause(bool pause)                    { this->pause = pause; }
        uint16_t    update(uint16_t mV)                     { return this->mV.average(mV); }
        uint16_t    updateCurrent(uint16_t mA)              { return this->mA.average(mA); }
        time_t      elapsed(void)                           { if (start) return now() - start; return 0; }
        bool        prechargeCurrent(void)                  { return c_reg; }
        void        setPrechargeCurrent(void)               { c_reg = true; }
        bool        chargeOverheat(void)                    { return overheat; }
        uint16_t    chargeStartTemp(void)                   { return start_temp; }
        uint32_t    chargeSaveTempTime(void)                { return save_temp; }
        void        registerOverheat(bool over)             { overheat = over; if (over) pause = true; }
        bool        phaseComplete(void)                     { return ph_complete; }
        void        setPhaseComplete(bool c)                { ph_complete = c; }
        tFinish     finishReason(void)                      { return reason; }
        void        finishCode(tFinish code);
        void        reset(void);
        uint8_t     phaseError(bool reset_error);
        time_t      remains(void);
        uint16_t    chargeCurrent(void);
        bool        voltageDrop(void);
        uint8_t     nextPhase(bool fin);                    // fin flag indicating the charging finished
        tPhase      phaseID(void);
        void        startCharging(bool reset_temp);
        void        saveStartTemp(uint16_t temp);
    private:
        HISTORY     mV;                                     // The battery voltage history data
        HISTORY     mA;                                     // The battery current history data
        uint16_t    mAh         = BATT_CAPACITY;            // The batterry capacity
        uint8_t     phase_index = 0;                        // Charging phase index
        tChargeType charge_type = CH_SLOW;                  // Charging type
        uint8_t     count       = 0;                        // Number of charging loops; 0 means do not loop
        time_t      finish      = 0;                        // Time when battery scheduler should be finished
        time_t      start       = 0;                        // Time when charging started
        bool        pause       = false;                    // Flag of current phase
        bool        c_reg       = false;                    // The charging current registered
        bool        overheat    = false;                    // The battery overheats while charging
        bool        ph_complete = false;                    // Phase complete temporary flag variable
        bool        no_batery   = false;
        uint8_t     ph_error    = 0;                        // Error counter in phase execution
        uint8_t     reason      = CODE_UNKNOWN;             // Charge complete code
        uint16_t    start_temp  = 0;                        // The battery temperature at the charge beggining
        uint32_t    save_temp   = 0;                        // Time when to save battery start temperature (ms)
        const uint8_t  max_phase_index = 5;
        const uint32_t save_temp_period = 1000*60*15;       // Save battery temperature in 15 minutes
};

#endif
