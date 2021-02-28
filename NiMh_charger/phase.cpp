#include "phase.h"
#include "config.h"
#include "log.h"

/*
 * Check battery phase.
 * Check the slot voltage and try to charge and check the current
 */
uint32_t CHECK::run(uint8_t index, TWCHARGER *pCharger, BATTERY *b) {
    uint32_t next = millis() + period;
    if (!b->phaseComplete()) {                              // No minumum voltage detected yet
        int16_t mV  = pCharger->mV(index);
        int16_t avg = b->update(mV);
        if (mV > BATT_DETECT_VOLTAGE) {                     // Perhaps, the battery is connected
            if (abs(mV - avg) > 30)                         // Synchronize battery voltage
                return 500;
            b->setPhaseComplete(true);                      // Set flag indicating the minimum battery voltage detected on this slot
            pCharger->discharge(index, true);               // Start discharging the battery and try to detect current through it
        }
    } else {                                                // Minimum battery voltage detected on the slot
        uint8_t iteration = b->phaseError(false);
        if (iteration < 3) {                                // PhaseError increments error counter and returns it
            // First 3 iterations after minimum voltage has been detected
            if (pCharger->mA(index) >= MIN_DISCHARGE_CURRENT) { // The battery detected in the slot
                pCharger->discharge(index, false);          // Stop discharging the battery
                return 0;                                   // Finish this phase
            }
            return 100;
        } else if (iteration < 23) {                        // No current from battery detected, try to apply chagring current
            if (iteration == 3)
                pCharger->discharge(index, false);
            // Apply BATT_DETECT_POWER to the battery and check current is greater than BATT_DETECT_CURRENT
            if (pCharger->isBatteryConnected(index, iteration-3)) { // The charging current detected
                return 0;
            }
            return 200;
        } else {                                            // No battery detected
            b->setPhaseComplete(false);                     // Clear flag indicating the minimal voltage detected
            b->phaseError(true);                            // Clear iteration counter
            return 2000;
        }
    }
    return next;
}

time_t DISCHARGE::init(uint8_t index, TWCHARGER *pCharger, BATTERY *b) {
    logPhase(index, 0);
    b->registerOverheat(false);
    b->setPause(false);
    pCharger->discharge(index, true);
    pause_period = 1000;
    return SLOW_CHARGING_TIME * 3600;
}

/*
 * Discharge phase.
 * Discharging the battery preformed in two modes: 
 * 1. Discharge for 10 seconds
 * 2. Pause (1 second or more)
 * When the battery voltage (current value and average) reaches MIN_VOLTAGE, the pause would
 * increased in two times. If the voltage would be less than minimum after pause greater than 7 seconds, 
 * the discharge procedure finish.
 * The pause can be decreased by two if the voltage of the battery (after big pause) would increased.
 * So, the pause can be changed ike this:
 * 1, 1, 1, 2, 4, 2, 4, 8 or so.
 */
uint32_t DISCHARGE::run(uint8_t index, TWCHARGER *pCharger, BATTERY *b) {
    uint32_t next = millis() + pause_period;
    if (b->chargeOverheat()) {
        if (pCharger->temperature(index) < MAX_TEMPERATURE-50) {
            b->registerOverheat(false);                         // Next time we will toggle the pause
        }
        return next;
    }
    if (pCharger->temperature(index) > MAX_TEMPERATURE) {
        b->registerOverheat(true);
        pCharger->discharge(index, false);
        return next;
    }
    bool ok = false;                                            // Flag to stop discharge
    bool charge = b->togglePause();
    if (charge) {
        uint16_t mV = pCharger->mV(index);
        uint16_t avg = b->update(mV);
        if (mV > MIN_VOLTAGE && mV > avg && pause_period > 1000)
            pause_period >>= 1;
        ok = (mV < MIN_VOLTAGE && avg < MIN_VOLTAGE);
        if (ok && pause_period < 7000) {
            ok = false;
            pause_period = constrain(pause_period << 1, 0, 30000); // Wait some time before finish discharging
        }
        pCharger->discharge(index, true);
        next = millis() + disch_period;
    } else {
        uint16_t mA = pCharger->mA(index);
        uint16_t avg = b->updateCurrent(mA);
        ok = (mA < MIN_DISCHARGE_CURRENT && avg < MIN_DISCHARGE_CURRENT);
        pCharger->discharge(index, false);
        next = millis() + pause_period;
    }
    if (ok) {
        pCharger->discharge(index, false);
        next = 0;
    }
    return next;
}

/*
 * Precharge phase.
 * Apply small, PRECHARGE_CURRENT to the battery
 */
time_t PRECHARGE::init(uint8_t index, TWCHARGER *pCharger, BATTERY *b) {
    logPhase(index, 1);
    pCharger->setChargeCurrent(index, PRECHARGE_CURRENT);
    bool charge = b->togglePause();
    pCharger->pauseCharging(index, !charge);
    return 60 * 3600;
}

uint32_t PRECHARGE::run(uint8_t index, TWCHARGER *pCharger, BATTERY *b) {
    bool charge = b->togglePause();
    uint32_t next = millis();
    bool started    = b->prechargeCurrent();
    bool precharged = b->phaseComplete();
    if (charge) {
        uint16_t mV = pCharger->mV(index);
        mV = b->update(mV);
        if (started && precharged) {                        // Perhaps, the precharge completed
            if (mV > PRECHARGE_VOLTAGE) {                   // Precharge voltage reached after discharge impulse and pause
                return 0;                                   // Finish phase
            } else {
                b->setPhaseComplete(false);
            }
        }
        if (started && mV > PRECHARGE_VOLTAGE) {            // It seems, the precharge is completed
            pCharger->pulseDischarge(index, DISCHARGING_PULSE); // Apply discharging pulse
            charge = b->togglePause();                      // Activate long time pause
            b->setPhaseComplete(true);
            next += test_period;
        } else {
            next += charge_period;
        }
    } else {
        uint16_t mA = pCharger->mA(index);
        b->updateCurrent(mA);
        if (!started && mA > PRECHARGE_CURRENT-2)
            b->setPrechargeCurrent();                       // Precharge current detected
        next += pause_period;
    }
    pCharger->pauseCharging(index, !charge);
    return next;
}

time_t CHARGE::init(uint8_t index, TWCHARGER *pCharger, BATTERY *b) {
    logPhase(index, 2);
    tChargeType type = b->schedule();
    time_t finish_time = now() + b->remains();
    b->startCharging();
    pCharger->setChargeCurrent(index, b->chargeCurrent());
    b->setPause(false);
    return finish_time;
}

uint32_t CHARGE::run(uint8_t index, TWCHARGER *pCharger, BATTERY *b) {
    uint32_t next_step   = millis() + period;
    tChargeType type = b->schedule();
    if (type != CH_FAST)
        pCharger->pulseDischarge(index, DISCHARGING_PULSE); // Apply discharging impulse
    uint16_t mV = pCharger->mV(index);                      // Read cached voltage value sometimes update this cache
    uint16_t mA = pCharger->mA(index);
    uint16_t t = pCharger->temperature(index);
    mV = b->update(mV);
    b->updateCurrent(mA);

    uint16_t max_temp = b->getMaxTemp();
    if (max_temp > 0 && t >= max_temp) {
        b->finishCode(CODE_OVERHEAT);
        char buff[20];
        sprintf(buff, "Temp. over %2d.%d", max_temp/10, max_temp%10);
        logComplete(index, buff);
        return 0;  
    }
    if (t >= HOT_TEMPERATURE) {                             // Heavy overheat registered, stop charging
        b->finishCode(CODE_OVERHEAT);
        logComplete(index, F("Overheat"));
        return 0;
    }
    if (mV >= MAX_VOLTAGE) {                                // The battery has been overcharged. mV is an average battery voltage
        b->finishCode(CODE_MAX_VOLTAGE);
        logComplete(index, F("Max voltage"));
        return 0;
    }
    
    if (type != CH_RESTORE && b->voltageDrop()) {           // The battery voltage drop has been detected, stop charging
        b->finishCode(CODE_OK);
        logComplete(index, F("Voltage drop"));
        return 0;
    }
    if (t > MAX_TEMPERATURE) {
        b->registerOverheat(true);                          // Set overheat flag and pause charging
        pCharger->pauseCharging(index, true);               // Stop charging and wait till battery chills
        return next_step;
    }
    if (b->chargeOverheat() && t < MAX_TEMPERATURE - 20) {  // 2 degrees lower than maximum, restore charging
        b->registerOverheat(false);
        b->setPause(false);
        pCharger->pauseCharging(index, false);              // Restore charging
    }
    if (pCharger->temperature(2) > HS_OVERHEAT) {           // Check for the heat sink overheat
        b->finishCode(CODE_HS_OVERHEAT);
        logComplete(index, F("Charger overheat"));
         return 0;
    }
    if (max_temp == 0 && b->elapsed() > 1200) {             // The battery charged over 20 minutes and battery max temperature was not setup
        max_temp = t + MAX_CHARGE_TEMP;
        if (max_temp > MAX_TEMPERATURE)
            max_temp = MAX_TEMPERATURE;
        b->setMaxTemp(max_temp);                            // Setup maximal battery temperature
    }
    return next_step;
}

time_t POSTCHARGE::init(uint8_t index, TWCHARGER *pCharger, BATTERY *b) {
    logPhase(index, 3);
    uint16_t t = pCharger->temperature(index);
    uint16_t current = b->capacity() / 20;                  // 5% of battery capacity
    if (t >= HOT_TEMPERATURE)
        current = b->capacity() / 50;                       // 2% of battery capacity
    pCharger->setChargeCurrent(index, current);
    b->startCharging();
    b->setPause(false);
    return 20*60;                                           // 20 minutes
}

uint32_t POSTCHARGE::run(uint8_t index, TWCHARGER *pCharger, BATTERY *b) {
    uint32_t next_step  = millis() + period;
    uint16_t t  = pCharger->temperature(index);

    if (b->phaseComplete()) {
        pCharger->pulseDischarge(index, DISCHARGING_PULSE); // Apply discharging pulse
        uint16_t mV = pCharger->mV(index);
        b->update(mV);
        if (mV > BATT_POSTCHARGE_VOLTAGE) return 0;         // Post charging complete
        b->setPhaseComplete(false);
    }
    bool charge = b->togglePause();
    if (charge) {
        pCharger->pulseDischarge(index, DISCHARGING_PULSE); // Apply discharging pulse
        uint16_t mV = pCharger->mV(index);
        b->update(mV);
        if (mV > BATT_POSTCHARGE_VOLTAGE) {                 // Near complete
            b->setPhaseComplete(true);
            next_step = millis() + pause_period;
            charge = false;
            b->setPause(true);
        }
    } else {
        uint16_t mA = pCharger->mA(index);
        b->updateCurrent(mA);
    }
    if (t > MAX_TEMPERATURE) return 0;                      // The battery has been overheated
    if (b->voltageDrop())    return 0;                      // The battery voltage drop has been detected, stop charging
    pCharger->pauseCharging(index, !charge);
    return next_step;
}

time_t KEEPCHARGE::init(uint8_t index, TWCHARGER *pCharger, BATTERY *b) {
    logPhase(index, 4);
    b->setPause(true);
    pCharger->setChargeCurrent(index, KEEP_CURRENT);
    pCharger->pauseCharging(index, true);
    return 0;                                               // forever
}

uint32_t KEEPCHARGE::run(uint8_t index, TWCHARGER *pCharger, BATTERY *b) {
    uint32_t next_step = millis();
    bool charge = b->togglePause();
    if (charge) {
        uint16_t mV = pCharger->mV(index);
        b->update(mV);
        next_step += charge_period;
    } else {
        uint16_t mA = pCharger->mA(index);
        b->updateCurrent(mA);
        next_step += pause_period;
    }
    pCharger->pauseCharging(index, !charge);
    return next_step;
}
