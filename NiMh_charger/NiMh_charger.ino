/*
 * On atmega328p running at 8 MHz
 * 
 * Created Feb 06 2021
 */
#include "hw.h"
#include "mode.h"
#include "phase.h"
#include "log.h"

// All hardware part together
HW          core;
BATTERY     batt[2];
MAIN       	mode_main(&core);
SETUP       mode_setup(&core);
MODE        *pMode  = &mode_main;

// Charge phases
CHECK       ph_check;
DISCHARGE   ph_discharge;
PRECHARGE   ph_precharge;
CHARGE      ph_charge;
POSTCHARGE  ph_postcharge;
KEEPCHARGE  ph_keepcharge;

static PHASE*      phase[6] = {&ph_check, &ph_discharge, &ph_precharge,
        &ph_charge, &ph_postcharge, &ph_keepcharge};

void disconnectBattery(uint8_t i) {
    tCfg rec;
    core.cfg.readConfig(rec);
    batt[i].init(rec.capacity[i], rec.type[i], rec.loops[i]); // The battery disconnected
    core.discharge(i, false);
    core.setChargeCurrent(i, false);  
    core.initChargeCounter(i);
    core.initDischargeCounter(i);   
}

void setup(void) {
    analogReference(EXTERNAL);
    logBegin();
    core.init(SO_BHA);
    mode_main.setup(&mode_setup);
    mode_setup.setup(&mode_main);

    pMode->init(batt);
    PHASE* p = phase[0];
    p->init(0, &core, &batt[0]);
    p->init(1, &core, &batt[1]);

    tCfg    rec;
    core.cfg.readConfig(rec);
    for (uint8_t i = 0; i < 2; ++i) {
        batt[i].init(rec.capacity[i], rec.type[i], rec.loops[i]);
    }

    core.dspl.aboutInfo(core.temperature(2));
    core.fan(true);
    delay(3000);
    core.fan(false);
    core.dspl.clear();
}

void loop(void) {
    static uint32_t next[2] = {0};                          // Next phase loop ms
    static time_t   over[2] = {0};
    static time_t   log_time = 0;

    core.manageFan();                                       // Prevent main heat sink overheating
        
    for (uint8_t i = 0; i < 2; ++i) {
        if (millis() < next[i]) continue;
        uint8_t phase_index = batt[i].phaseIndex();
        PHASE* p = phase[phase_index];
        if (phase_index == 5 && core.mV(i) < BATT_DETECT_VOLTAGE
                && batt[i].averageCurrent() < BATT_DETECT_CURRENT) {
            // Disconnected battery after charged
            disconnectBattery(i);                           // Initialize battery slot with parameters from EEPROM
            p = phase[0];
            over[i] = p->init(i, &core, &batt[i]);
            if (over[i] > 0) over[i] += now();
            next[i] = millis() + 10000;
            return;
        } else if (phase_index >= 1 && batt[i].averageVoltage() < BATT_DETECT_VOLTAGE 
                && batt[i].averageCurrent() < BATT_DETECT_CURRENT) { // disconnected battery while charging
            disconnectBattery(i);                           // Initialize battery slot with parameters from EEPROM
            p = phase[0];
            over[i] = p->init(i, &core, &batt[i]);
            if (over[i] > 0) over[i] += now();
            next[i] = millis() + 10000;
            return;
        }
        
        uint32_t next_ms = p->run(i, &core, &batt[i]);      // Process charging phase. next_ms - Time to run the phase next time
        if (over[i] > 0 && now() >= over[i]) {              // Phase is over, change to the next phase
            phase_index = batt[i].nextPhase(true);          // No longer charge the battery
            p = phase[phase_index];
            over[i] = 0;
        }
        if (next_ms == 0) {                                 // The phase finished
            phase_index = batt[i].nextPhase(false);         // Activate next charging phase
            if (phase_index == PH_DISCHARGE) {
                core.initDischargeCounter(i);
                core.initChargeCounter(i);
            } else if (phase_index == PH_PRECHARGE) {
                core.initChargeCounter(i);
            } else if (phase_index == PH_KEEP) {
                logComplete(i, batt[i].finishReason());
            }
            p = phase[phase_index];
            over[i] = p->init(i, &core, &batt[i]);
            if (over[i] > 0) over[i] += now();
            next[i] = millis() + 10000;
        } else {
            next[i] = next_ms;
        }
    }
    
    // User iteration
    MODE* new_mode = pMode->returnToMain();
    if (new_mode && new_mode != pMode) {
        pMode = new_mode;
        pMode->init(batt);
        return;
    };
    
    // Process current mode. Display battery status
    new_mode = pMode->loop(batt);
    if (new_mode != pMode) {
        if (new_mode == 0)
            new_mode = &mode_main;
        pMode = new_mode;
        pMode->init(batt);
    }

    // Log the battery status
    if (now() >= log_time) {
        log_time = now() + 300;
        for (uint8_t i =0 ; i < 2; ++i) {
            int16_t t = core.temperature(i);
            logBatteryStatus(i, &batt[i], &core, t);
        }
    }
    
}

/*
 * The TIM1 overflow interrupt handler.
 * Called 500 times per second
 * Used to manage the charging current via PID controller
 */
static volatile uint16_t counter = 0;
ISR(TIMER1_OVF_vect) {
    if (++counter > 500/4) {                                // 4 times per second. End of period, manage channel "A"
        counter = 0;
        TIMSK1 &= ~_BV(TOIE1);                              // disable the overflow interrupts
        core.keepCurrent(0);
        TIMSK1 |= _BV(TOIE1);                               // enable the the overflow interrupts
    } else if (counter == 500/8) {                          // Half of period, manage channel "B"
        TIMSK1 &= ~_BV(TOIE1);                              // disable the overflow interrupts
        core.keepCurrent(1);
        TIMSK1 |= _BV(TOIE1);                               // enable the the overflow interrupts
    }
}
