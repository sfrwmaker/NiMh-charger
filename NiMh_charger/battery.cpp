#include "battery.h"
#include "log.h"

void BATTERY::init(uint16_t mAh, tChargeType type, uint8_t loops, bool no_discharge){
    this->mAh = mAh;
    charge_type = type;
    phase_index = 0;
    count       = loops;
    no_disch    = no_discharge;
    if (count > 0)                                          // When charging loops enabled, do discharge battery every loop
        no_discharge    = false;
    pause       = false;
    ph_complete = false;
    start       = 0;
    max_temp    = 0;
    reason      = CODE_UNKNOWN;
    reset();
}
/*
 * The main charge phase should be completed when voltage drop detected
 * Voltage drop means the average voltage gradient become negative
 * The post-charge phase uses this method also.
 * As soon as post-charge current is less than current in the main charging phase,
 * the battery voltage can decrease at the begining of the post-charge phase. 
 * To prevent wrong 'complete' condition detection, the volt_incr flag implemented.
 * When the pahse start, this flag reseted to false. And when positive gradient of average
 * voltage detected the volt_incr flag set to true.
 */
bool BATTERY::voltageDrop(void) {
    if (mV.length() < B_MV_SIZE/2) return false;
    int32_t g = mV.gradient();
#ifdef LOG_VOLTAGE_DUMP
    logTimestamp();
    Serial.print(F("vdump: "));
    mV.dump();
#endif
    if (!volt_incr) {
        if (g > 4) {
            volt_incr = true;                               // voltage increment detected!
            logMessage(F("Voltage increment detected"));
        }
        return false; 
    }
    return (g < -10);
}

uint16_t BATTERY::chargeCurrent(void) {
    uint16_t current = 0;
    if (charge_type == CH_RESTORE) {
        current = mAh / 20;
        if (current > RESTORE_CURRENT)
            current = RESTORE_CURRENT;
        uint32_t h = mAh / current + 1;
        finish = now() + h*3600;
    } else if (charge_type == CH_SLOW) {
        current = mAh / 10;
        finish  = now () + 39600;                           // 11*3600;
    } else {
        current = mAh / 4;
        finish  = now () + 18000;                           // 5*3600;
    }
    return current;
}

uint8_t BATTERY::nextPhase(bool fin) {
    if (fin && phase_index != max_phase_index) {
        phase_index = max_phase_index;                      // Finish charging
        finish = 0;                                         // No timeout in this phase
    } else if (phase_index == 2 && count == 0 && elapsed() < 60) { // Precharge too short, baterry is bad
        phase_index = max_phase_index;                      // Finish charging
        finish = 0;                                         // No timeout in this phase
    } else if (phase_index < max_phase_index) {
        if (count > 0 && phase_index == 4) {                // Loop counter means several time to discharge/charge
            phase_index = 0;                                // phase_index == 4 - means postcharge. Start over again
            --count;
        }
        if (phase_index == 0 && no_disch) {                 // Skip discharge phase
            phase_index = (uint8_t)PH_PRECHARGE;
        } else {
            ++phase_index;
        }
        if (phase_index == (uint8_t)PH_PRECHARGE) {         // Start the battery charging process
            start = now();
            chargeCurrent();                                // Also calculate finish time
        }
        mA.reset();                                         // Reset battery current history
    }
    ph_error    = 0;
    ph_complete = false;
    max_temp    = 0;
    return phase_index;
}

tPhase BATTERY::phaseID(void) {
    if (phase_index < 3) {                                  // No charge yet
        return (tPhase)phase_index;
    } else if (phase_index == 3) {                          // Charging
        uint8_t t = phase_index + (uint8_t)charge_type;
        return (tPhase)t;
    } else {
        return (tPhase)(phase_index + 2);
    }
}

void BATTERY::startCharging(void) {
    overheat    = false;
    volt_incr   = false;                                    // Voltage increment flag reset at phase start
    // Truncate history data
    uint16_t tmp = mV.read();
    mV.reset();
    mV.update(tmp);
    tmp = mA.read();
    mA.reset();
    mA.update(tmp);
}

time_t BATTERY::remains(void) {
    if (finish <= now()) {
        chargeCurrent();                                    // Calculate finish time also
    }
    return finish - now();   
}

uint8_t BATTERY::phaseError(bool reset_error) {
    if (reset_error) {
        ph_error = 0;
    } else {
        ++ph_error;
    }
    return ph_error;
}

void BATTERY::reset(void) {
    mV.reset();
    mA.reset();
}

void  BATTERY::finishCode(tFinish code) {
    reason  = code;
    finish  = now();
}
