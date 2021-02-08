#include "twin_charger.h"

void PID::init(uint8_t denominator_p) {                     // PID parameters are initialized from EEPROM by  call
    resetPID();
}

void PID::resetPID(void) {
    power   = previous_power = 1300000;                     // Experementally setup
    i_summ  = 0;
    curr_h0 = 0;                                            // Start PID process again
    curr_h1 = 0;
    reached = false;
}

int32_t PID::reqPower(int16_t current_set, int16_t actual_current) {
    if (curr_h0 < 0) {                                     // Use direct formulae because do not know previous temperature
        power       = 0;
        i_summ      = 0;
        i_summ += current_set - actual_current;
        power = Kp*(current_set - actual_current) + Ki * i_summ;
    } else {
        if (!reached && actual_current > current_set) {     // Too much power
            i_summ  = 0;
            power   = previous_power;
            reached = true;
        }
        int32_t kp = Kp * (curr_h1  - actual_current);
        int32_t ki = Ki * (current_set - actual_current);
        int32_t delta_p = kp + ki;
        power += delta_p;                                   // Power is stored multiplied by denominator!
        previous_power  = power;
    }
    curr_h0 = curr_h1;
    curr_h1 = actual_current;
    int32_t pwr = power + (1 << (denominator_p-1));         // prepare the power to divide by denominator, round the result
    pwr >>= denominator_p;                                  // divide by the denominator
    return pwr;
}

int PID::changePID(uint8_t p, int k) {
  switch(p) {
    case 1:
      if (k >= 0) Kp = k;
      return Kp;
    case 2:
      if (k >= 0) Ki = k;
      return Ki;
    default:
      break;
  }
  return 0;
}

void LongPWM::init(void) {
    pinMode(9,  OUTPUT);                                    // Use D9 pin for charger power A
    pinMode(10, OUTPUT);                                    // Use D10 pin for charger power B
    PORTB &= ~0b00000110;                                   // Switch-off both chargers, D9 & D10
    noInterrupts();
    TCNT1   = 0;
    TCCR1B  = _BV(WGM13);                                   // Set mode as phase and frequency correct pwm, stop the timer
    TCCR1A  = 0;
    ICR1    = 8000;                                         // max PWM value is 7999
    TCCR1B  = _BV(WGM13)  | _BV(CS10);                      // Top value = ICR1, prescale = 1; 500 Hz
    TCCR1A |= _BV(COM1A1) | _BV(COM1B1);                    // XOR D9 on OC1A and D10 on OC1B
    OCR1A   = 0;                                            // Switch-off the signal on pin D19
    OCR1B   = 0;                                            // Switch-off the signal on pin D10
    TIMSK1  = _BV(TOIE1);                                   // Enable overflow interrupts
    interrupts();
}

void LongPWM::duty(uint8_t index, uint16_t d) {
    if (index > 2) return;
    if (d > 8191) d = 8191;
    if (index)
        OCR1B = d;
    else
        OCR1A = d;
}

TWCHARGER::TWCHARGER(uint8_t one_wire_pin, uint8_t fan_pin) : ds(one_wire_pin){
    enable_pin[0]       = TWCH_ENABL_A;
    enable_pin[1]       = TWCH_ENABL_B;
    discharge_pin[0]    = TWCH_DISCH_A;
    discharge_pin[1]    = TWCH_DISCH_B;
    voltage_pin[0]      = TWCH_VOLTG_A;
    voltage_pin[1]      = TWCH_VOLTG_B;
    current_pin[0]      = TWCH_CURRT_A;
    current_pin[1]      = TWCH_CURRT_B;
    this->fan_pin       = fan_pin;
    fan_on              = false;
    clearSensors();
}

void TWCHARGER::init(void) {
    pwm.init();
    pinMode(enable_pin[0],      OUTPUT);
    pinMode(enable_pin[1],      OUTPUT);
    pinMode(discharge_pin[0],   OUTPUT);
    pinMode(discharge_pin[1],   OUTPUT);
    pinMode(voltage_pin[0],     INPUT);
    pinMode(voltage_pin[1],     INPUT);
    pinMode(current_pin[0],     INPUT);
    pinMode(current_pin[1],     INPUT);
    pinMode(fan_pin,            OUTPUT);
    digitalWrite(enable_pin[0],     LOW);
    digitalWrite(enable_pin[1],     LOW);
    digitalWrite(discharge_pin[0],  LOW);
    digitalWrite(discharge_pin[1],  LOW);
    digitalWrite(fan_pin,           LOW);
    mode[0]     = MODE_STOP;
    mode[1]     = MODE_STOP;
    current[0]  = 0;
    current[1]  = 0;
    ch_pid[0].init();
    ch_pid[1].init();
    voltage_update  = 0;
    temp_update     = 0;
}

bool TWCHARGER::setSensorAddress(uint8_t index, const uint8_t addr[8]) {
    if (index < 3 && OneWire::crc8(addr, 7) == addr[7]) {
        for (uint8_t i = 0; i < 8; ++i) {
            ds1820b_addr[index][i] = addr[i];
        }
        return true;
    }
    return false;
}

uint8_t TWCHARGER::discoverSensors(void) {
    clearSensors();
    uint8_t addr[8] = {0};
    uint8_t i = 0;
    for ( ; i < 3; ++i) {
        if (!ds.search(addr)) {
            break;
        }
        if (OneWire::crc8(addr, 7) == addr[7]) {
            setSensorAddress(i, addr);
        }
    }
    ds.reset_search();
    delay(100);
    return i; 
}

bool TWCHARGER::getSensorAddress(uint8_t index, uint8_t addr[8]) {
    if (index < 3 && OneWire::crc8(ds1820b_addr[index], 7) == ds1820b_addr[index][7]) {
        for (uint8_t i = 0; i < 8; ++i) {
            addr[i] = ds1820b_addr[index][i];
        }
        return true;
    }
    return false;
}

// The battery sensor temperature, 1/100 of Celsius
int16_t TWCHARGER::temperature(uint8_t index) {
    if (index < 3 && OneWire::crc8(ds1820b_addr[index], 7) == ds1820b_addr[index][7]) {
        if (temp[index] == 0 || no_expiration || now() >= temp_update) {
            temp_update = now() + temp_expiration;
            uint8_t type_s = 0;
            switch (ds1820b_addr[index][0]) {
                case 0x10:
                    type_s = 1;
                    break;
                case 0x28:
                case 0x22:
                    break;
                default:
                    temp[index] = 0;
                    return 0;
            }        
            ds.reset();
            ds.select(ds1820b_addr[index]);
            ds.write(0x44, 1);                              // start conversion, with parasite power on at the end
            delay(1000);                                    // wait for conversion finished                    
            if (ds.reset()) {                               // return 1 if the device found on the bus
                ds.select(ds1820b_addr[index]);    
                ds.write(0xBE);                             // Read Scratchpad
                uint8_t data[12];
                for (uint8_t j = 0; j < 9; j++) {           // we need 9 bytes
                    data[j] = ds.read();
                }
                int16_t raw = (data[1] << 8) | data[0];     // Convert the data to actual temperature
                if (type_s) {
                    raw = raw << 3;                         // 9 bit resolution default
                    if (data[7] == 0x10) {
                        raw = (raw & 0xFFF0) + 12 - data[6];// "count remain" gives full 12 bit resolution
                    }
                } else {
                    uint8_t cfg = (data[4] & 0x60);         // at lower res, the low bits are undefined, so let's zero them
                    if (cfg == 0x00) raw = raw & ~7;        // 9 bit resolution, 93.75 ms
                    else if (cfg == 0x20) raw = raw & ~3;   // 10 bit res, 187.5 ms
                    else if (cfg == 0x40) raw = raw & ~1;   // 11 bit res, 375 ms
                }
                raw *= 5;                                   // celsius = float(raw). We return celsuis*10 (raw*5/8)
                raw >>= 3;                                  // divide by 8
                temp[index] = raw;
                return raw;        
            }
        }
        return temp[index];
    }
    return 0;
}

/*
 * Stop Any charge/discharge activity to check voltage of both batteries
 * save measured data for expiration_period
 */
uint16_t TWCHARGER::mV(uint8_t index) {
    if (index < 2) {
        if (no_expiration || now() >= voltage_update) {     // Switch off all chargings and perform the voltage checking                         
            for (uint8_t i = 0; i < 2; ++i) {
                if (mode[i] == MODE_CHARGE) {
                    mode[i] = MODE_WAS_CHARGE;              // Change mode to inform keepCurrent() procedure
                    digitalWrite(enable_pin[i], LOW);       // Stop charging
                }
                digitalWrite(discharge_pin[i], LOW);        // Stop discharging
            }
            delay(50);
            voltage[0] = milliVolts(voltage_pin[0]);
            voltage[1] = milliVolts(voltage_pin[1]);
            for (uint8_t i = 0; i < 2; ++i) {
                if (mode[i] == MODE_WAS_CHARGE) {
                    mode[i] = MODE_CHARGE;
                    digitalWrite(enable_pin[i], HIGH);      // Restore charging
                } else if (mode[i] == MODE_DISCHARGE) {
                    digitalWrite(discharge_pin[i], HIGH);   // Restore discharging
                }
            }
            voltage_update = now() + voltage_expiration;
        }
        return voltage[index];
    }
    return 0;
}

uint16_t TWCHARGER::mA(uint8_t index) {
    if (index < 2) {
        uint32_t v      = 0;
        uint32_t res    = 1;
        if (mode[index] == MODE_DISCHARGE) {
            v   = milliVolts(voltage_pin[index]);
            res = (index==0)?TWCH_DISCH_RES_A:TWCH_DISCH_RES_B;
        } else {
            v   = milliVolts(current_pin[index]);
            res = (index==0)?TWCH_CHARGE_RES_A:TWCH_CHARGE_RES_B;
        }
        v *= 10;                                            // Because thge resistance is in 1/10 ohm
        v += res/2;                                         // Round the result
        return v / res;
    }
    return 0;
}

void TWCHARGER::discharge(uint8_t index, bool on) {
    if (index < 2) {
        if (on) {                                           // Switch off charging voltage
            mode[index] = MODE_DISCHARGE;
        } else {
            mode[index] = MODE_STOP;
        }
        digitalWrite(enable_pin[index], LOW);               // Switch charging power off
        pwm.duty(index, 0);                                 // No voltage to LM317
        current[index] = 0;
        digitalWrite(discharge_pin[index], on);
    }
}

void TWCHARGER::pulseDischarge(uint8_t index, uint16_t ms) {
    if (mode[index] == MODE_CHARGE) {
        mode[index] = MODE_DISCHARGE;
        digitalWrite(enable_pin[index], LOW);
        digitalWrite(discharge_pin[index], HIGH);
        delay(ms);
        digitalWrite(discharge_pin[index], LOW);;
        digitalWrite(enable_pin[index], HIGH);
        mode[index] = MODE_CHARGE;
    }
}

void TWCHARGER::setChargeCurrent(uint8_t index, uint16_t mA) {
    if (index > 2) return;
    if (mA >0) {                                            // Start charging
        mode[index] = MODE_CHARGE;
        digitalWrite(discharge_pin[index], LOW);            // Make sure stop discharging
        digitalWrite(enable_pin[index], HIGH);              // Switch charging power on
        current[index] = mA;
        ch_pid[index].init();
    } else {
        mode[index] = MODE_STOP;
        digitalWrite(discharge_pin[index], LOW);            // Make sure stop discharging
        digitalWrite(enable_pin[index], LOW);               // Switch charging power off
    }
    pwm.duty(index, 0);                                     // No voltage to LM317 yet
}

void TWCHARGER::pauseCharging(uint8_t index, bool on) {
    if (index >= 2) return;
    if (!on) {
        if (mode[index] == MODE_PAUSE) {
            mode[index] = MODE_CHARGE;
            digitalWrite(enable_pin[index], HIGH);
        }
    } else {
        if (mode[index] == MODE_CHARGE) {
            mode[index] = MODE_PAUSE;
            digitalWrite(enable_pin[index], LOW);
        }
    }
}

void TWCHARGER::keepCurrent(uint8_t index) {
    if (mode[index] == MODE_CHARGE) {
        int16_t actual_current = mA(index);
        charge_ctr[index] += actual_current;                // Count applied current
        if (charge_ctr[index] >= power_mAh) {               // Charged at least by 1 mAh
            ++charge_mAh[index];
            charge_ctr[index] -= power_mAh;
        }
        int32_t pwr = ch_pid[index].reqPower(current[index], actual_current);
        pwr = constrain(pwr, 0, 7000);
        pwm.duty(index, pwr);                               // Apply voltage to LM317
    } else if (mode[index] == MODE_DISCHARGE) {
        dische_ctr[index] += mA(index);                     // Count discharge current
        if (dische_ctr[index] >= power_mAh) {               // Charged at least by 1 mAh
            ++dische_mAh[index];
            dische_ctr[index] -= power_mAh;
        }
    }
}

bool TWCHARGER::isBatteryConnected(uint8_t index, uint8_t iteration) {
    bool status = false;
    if (index < 2 && iteration < 20 && mode[index] == MODE_STOP) {
        digitalWrite(discharge_pin[index], LOW);            // Make sure stop discharging
        digitalWrite(enable_pin[index], HIGH);              // Switch charging power on
        uint16_t power = map(iteration, 0, 19, BATT_DETECT_POWER, MAX_BATT_DETECT_POWER);
        pwm.duty(index, power);
        delay(100);
        uint16_t mV = milliVolts(current_pin[index]);
        pwm.off();
        status = (mV > BATT_DETECT_CURRENT);
        digitalWrite(enable_pin[index], LOW);
    }
    return status;
}

void TWCHARGER::clearSensors(void) {
    for (uint8_t i = 0; i < 8; ++i) {
        ds1820b_addr[0][i] = ds1820b_addr[1][i] = 0;
    }
}

uint32_t TWCHARGER::milliVolts(uint8_t pin) {
    uint32_t v = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        v += analogRead(pin);
        delay(20);
    }
    v += 4;
    v >>= 3;
    v *= AREF_MV;
    v += 1023/2;                                            // Round the result
    v /= 1023;
    return v;
}

// change two sensors address
void TWCHARGER::changeSensors(uint8_t x, uint8_t y) {
    if (x == y || x > 2 || y > 2) return;
    for (uint8_t i =0; i < 8; ++i) {
        uint8_t t           = ds1820b_addr[x][i];
        ds1820b_addr[x][i]  = ds1820b_addr[y][i];
        ds1820b_addr[y][i]  = t;
    }
}

void TWCHARGER::orderSensors(tSensorOrder order) {
    switch (order) {
        case SO_AHB:
            changeSensors(1, 2);
            break;
        case SO_BAH:
            changeSensors(0, 1);
            break;
        case SO_BHA:
            changeSensors(0, 1);
            changeSensors(0, 2);
            break;
        case SO_HAB:
            changeSensors(0, 1);
            changeSensors(1, 2);
            break;
        case SO_HBA:
            changeSensors(0, 2);
            break;
        case SO_ABH:                                        // Already in order
        default:
            break;
    }
    // Initialize FAN turn-on temperature                      
    hs_hot_temp = temperature(2) + HS_ON_TEMP;              // The ambient temperature plus predefined difference
    if (hs_hot_temp > HS_HOT_TEMP) {                        // The ambient temperature is high
        hs_hot_temp = HS_HOT_TEMP;
    }
}

bool TWCHARGER::manageFan(void) {
    int16_t hs_temp = temperature(2);                       // Ordered sensor list. 2 - is a heat sink sensor
    if (fan_on && hs_temp < hs_hot_temp - HS_DIFF_TEMP) {
        fan_on = false;
        digitalWrite(fan_pin, LOW);
    } else if (!fan_on && hs_temp >= hs_hot_temp) {
        fan_on = true;
        digitalWrite(fan_pin, HIGH);
    }
}

void TWCHARGER::fan(bool fan_on) {
    digitalWrite(fan_pin, fan_on?HIGH:LOW);
    this->fan_on = fan_on;
}
