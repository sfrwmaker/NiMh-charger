#include "mode.h"
#include "cfg.h"

//---------------------- The Menu mode -------------------------------------------
MODE* MODE::returnToMain(void) {
    if (mode_next && time_to_return && millis() >= time_to_return)
        return mode_next;
    return this;
}

void MODE::resetTimeout(void) {
    if (timeout_secs) {
        time_to_return = millis() + timeout_secs * 1000;
    }
}
void MODE::setTimeout(uint16_t t) {
    timeout_secs = t;
}

//---------------------- The Main working mode -----------------------------------
void MAIN::init(BATTERY b[2]) {
    pCore->dspl.clear();
    encoder_pos = 0;
    pCore->encoder.reset(encoder_pos, 0, 1, 1, 1, true);
    reset_display = false;
    change_mode = millis() + 3000;
    update_screen = 0;
    resetDisplay();
}

MODE* MAIN::loop(BATTERY b[2]) {
    DSPL *pD = &pCore->dspl;

	uint8_t bs = pCore->encoder.buttonCheck();	
	if (bs > 0) {                                           // short or long button press
        if (mode_next) return mode_next;
	}

    uint8_t e = pCore->encoder.read();
    if (encoder_pos != e) {
        encoder_pos = e;
        resetDisplay();
        update_screen = 0;
    }
	
    if (millis() < update_screen) return this;
    update_screen = millis() + period;
    if (pCore->dspl.isBacklight() && now() > turn_off_display)
        pCore->dspl.backlight(false);

    if (reset_display) {
        pCore->dspl.clear();
        reset_display = false;
    }
    
    uint32_t mode_period = 10000;
    for (uint8_t i = 0; i < 2; ++i) {                       // Two batteries loop
        tPhase  phase   = b[i].phaseID();
        uint16_t mV     = b[i].averageVoltage();
        uint16_t mA     = b[i].averageCurrent();
        uint16_t temp   = pCore->temperature(i);
        // dspl_mode is display information mode (what info to show this time)
        // 0 - About and controller temperature
        // 1 - Phase name
        // 2 - Charging info
        // 3 - Temperature info
        // 4 - Charge elapced / remaining time 
        uint8_t d_mode = dspl_mode;
        if (phase == PH_CHECK) {
            if (d_mode > 2) d_mode = 2;                     // Show voltage and current
            if (d_mode > 0 && mV < BATT_DETECT_VOLTAGE) {   // No battery detected in the channel
                pD->slotStatus(i, b[i].capacity(), b[i].schedule(), b[i].noDischarge());
                continue;
            }
            uint16_t real = pCore->mV(i);                   // We are setting up the average voltage value
            if (real > mV) mV = real;                       // Average voltage has not been setup yet
        }
        uint16_t charged = 0;
        if (phase == PH_DISCHARGE) {
            charged = pCore->discharged(i);
            if (d_mode > 3) d_mode = 3;
        } else {
            charged = pCore->charged(i);
        }
        switch (d_mode) {
            case 0:                                         // About + self temperature
                if (i == 0) {                               // Only show global info on first charger
                    pD->aboutInfo(pCore->temperature(2));
                }
                mode_period = 3000;
                break;
            case 1:                                         // Phase name
                if (phase == 7) {                           // Charging complete
                    pD->complete(i, b[i].finishReason(), b[i].capacity(), b[i].schedule());
                } else {
                    pD->phaseName(phase, i, b[i].capacity(), b[i].schedule());
                }
                mode_period = 20000;
                break;
            case 2:                                         // Voltage, current
                pD->chargingInfo(phase, i, mV, mA, charged, temp);
                mode_period = 20000;
                break;
            case 3:                                         // (Dis)charged mAh, Temperature
                pD->tempInfo(phase, i, charged, temp, mV, mA);
                break;
            case 4:                                         // Charging times
                pD->timeInfo(phase, i, b[i].elapsed(), b[i].remains(), mV, mA);
                break;
            default:
                break;
        }
        if (d_mode == 0)                                    // Do not display second slot when about screen
            break;
    }                                                       // End of battery loop
    if (millis() > change_mode) {
        change_mode = millis() + mode_period;
        if (++dspl_mode > 4) {
            dspl_mode = 0;
            reset_display = true;
        }
    }
    return this;
}

void MAIN::resetDisplay(void) {
    turn_off_display = now() + turn_off_period;
    if (pCore->dspl.updateBrightness() < 50) {
        pCore->dspl.setBrightness(50);
    }
    pCore->dspl.backlight(true);
}

//---------------------- The Setup mode ------------------------------------------
void SETUP::init(BATTERY b[2]) {
    pCore->cfg.readConfig(cfg);                             // Load the configuration data
    pCore->dspl.setBrightness(255);
    pCore->dspl.backlight(true);
    pCore->dspl.clear();
    slot    = 2;                                            // The battery slot should be selected first
    pCore->encoder.reset(0, 0, 1, 1, 1, true);              // Encoder interval [0; 1] - select battery slot
    update_screen   = 0;
    setTimeout(30);                                         // Automatically return in 30 seconds timeout
}

MODE* SETUP::loop(BATTERY b[2]) {
    static uint16_t old_encoder = 10000;
    DSPL *pD = &pCore->dspl;
    RENC *pE = &pCore->encoder;

    uint8_t bs = pCore->encoder.buttonCheck();  
    if (bs == 1) {                                          // short button press
        if (slot == 2) {                                    // The batery slot just selected
            slot = pCore->encoder.read();                   // Setup slot number to edit
            entity = 0;
            old_encoder = 1;
            pE->reset(1, 1, 5, 1, 1, true);                 // Prepare to select edit entity
        } else {
            if (entity == 0) {                              // The entity index to be edited just selected
                entity = pE->read();                        // Setup entity to edit
                switch (entity) {                           // Prepare encoder depending on entity
                    case 1:                                 // Prepare to edit battery capacity
                        old_encoder = cfg.capacity[slot];
                        // Make sure the battery capacity is multiplied by 100
                        old_encoder -= old_encoder % 100;
                        old_encoder = constrain(old_encoder, 100, 5000); 
                        pE->reset(old_encoder, 100, 5000, 100, 500, false);
                        break;
                    case 2:                                 // Prepare to edit charging type
                        old_encoder = cfg.type[slot];
                        pE->reset(old_encoder, 0, 2, 1, 1, true);
                        break;
                    case 3:                                 // Prepare to edit charging loops number
                        old_encoder = cfg.loops[slot];
                        pE->reset(old_encoder, 0, 10, 1, 1, false);
                        break;
                    case 4:                                 // Toggle battery discharge phase
                        cfg.bit_flag[slot] ^= bf_nodischarge;
                        entity = 0;                         // Edit in place
                        break;
                    default:                                // Return to battery select menu
                        old_encoder = 0;
                        pCore->encoder.reset(0, 0, 1, 1, 1, true);
                        slot = 2;
                        break;
                }
            } else {                                        // Some entity has been changed
                switch (entity) {
                    case 1:                                 // battery capacity
                       cfg.capacity[slot] = pE->read();
                       cfg.capacity[slot] -= cfg.capacity[slot] % 100;
                       cfg.capacity[slot] = constrain(cfg.capacity[slot], 100, 5000);
                       break;
                    case 2:                                 // charging type
                        cfg.type[slot]  = pE->read();
                        break;
                    case 3:                                 // charging loops
                        cfg.loops[slot] = pE->read();
                        break;
                    default:
                        break;
                }
                pE->reset(1, 1, 5, 1, 1, true);             // Prepare to select edit entity
                entity = 0;
            }
        }
        pD->clear();
        update_screen   = 0;
        resetTimeout();
    } else if (bs == 2) {                                   // long button press
        for (uint8_t i = 0; i < 2; ++i) {
            if (cfg.bit_flag[i] & bf_nodischarge)           // When nodischarge bit set, disable charging loops
                cfg.loops[i] = 0;
        }
        pCore->cfg.saveConfig(cfg);                         // Save configuration data to the EEPROM
        for (uint8_t i = 0; i < 2; ++i) {
            // Cannot change battery parameters for already charging battery
            if (b[i].phaseIndex() == (uint8_t)PH_CHECK) {
                bool no_discharge = cfg.bit_flag[i] & bf_nodischarge;
                b[i].init(cfg.capacity[i], cfg.type[i], cfg.loops[i], no_discharge);
            }
        }
        if (mode_next) return mode_next;
        return this;
    }

    uint16_t e = pE->read();
    if (e != old_encoder) {
        old_encoder     = e;
        update_screen   = 0;
        resetTimeout();
    }
    if (millis() < update_screen) return this;
    update_screen = millis() + period;

    if (slot == 2) {                                        // Select battery
        pD->slotMenu(e);
    } else {
        uint16_t v = 0;
        if (entity == 0) {                                  // Selecting entity to be edited
            switch (e) {
                case 1:                                     // Battery capacity
                    v = cfg.capacity[slot];
                    break;
                case 2:                                     // Charging type
                    v = cfg.type[slot];
                    break;
                case 3:                                     // Charging loops
                    v = cfg.loops[slot];
                    break;
                case 4:                                     // nodischarge bit
                    v = cfg.bit_flag[slot];
                    break;
                default:
                    break;
            }
            pD->setupMode(slot, e, v);
        } else {                                            // Changing entity value
            if (entity == 1) {                              // The battery capacity
                e -= e % 100;
                e = constrain(e, 100, 5000);
            }
            pD->setupMode(slot, entity, e);
        }
    }
    return returnToMain();
}
