#include "config.h"
#include "encoder.h"
#include "twin_charger.h"

#define MENU_LEN    (6)

static char *menu[MENU_LEN] = { 
    "channel",
    "charge current",
    "discharge",
    "charge",
    "heat sink fan",
    "temperature"        
};
static uint8_t  item        = 0;
static bool     edit        = false;
static uint8_t  channel     = 0;
static uint16_t current[2]  = {50, 50};
static bool     fan         = false;
static bool     show_temp   = false;

const uint32_t  passive_period  = 30000;
const uint32_t  active_period   =  1000;
const uint32_t  temp_period     =  5000;

RENC        enc(RENC_M_PIN, RENC_S_PIN, RENC_B_PIN);
TWCHARGER   tchrgr(TWCH_ONE_WIRE, TWCH_FAN_PIN);

// Encoder interrupt handler
static void rotEncChange(void) {
    enc.encoderIntr();
}

void printItem(uint16_t value) {
    if (edit) {
        Serial.print("set ");
        Serial.print(menu[item]);
        Serial.print(" ");
        if (item == 1) {                        // Charge current
            Serial.print("[");
            Serial.print(channel);
            Serial.print(F("] = "));
        }
        Serial.println(value);
    } else {
        Serial.print(menu[item]);
        switch (item) {
            case 0:
                Serial.print(" (");
                Serial.print(channel);
                Serial.println(")");
                break;
            case 1:
                Serial.print("([");
                Serial.print(channel);
                Serial.print(F("] = "));
                Serial.print(current[channel]);
                Serial.println(")");
                break;
            case 4:
            case 5:
                Serial.println("");
                break;
            default:
                Serial.print("[");
                Serial.print(channel);
                Serial.println(F("]"));
                break;
        }
    }
}

void setup() {
    analogReference(EXTERNAL);
    Serial.begin(115200);
    tchrgr.init();
    tchrgr.discoverSensors();
    attachInterrupt(digitalPinToInterrupt(RENC_M_PIN), rotEncChange, CHANGE);
    enc.init();
    enc.reset(item, 0, MENU_LEN-1, 1, 1, true);
    printItem(0);
}

void loop() {
    static uint32_t show_battery_ms = 0;

    if (enc.buttonCheck() > 0) {                // Button pressed
        if (edit) {                             // Exit from edit mode
            edit = false;
            enc.reset(item, 0, MENU_LEN-1, 1, 1, true);
            printItem(item);
        } else {                                // Button pressed on menu item
            switch (item) {
                case 0:                         // Prepare to edit channel
                    edit = true;
                    enc.reset(channel, 0, 1, 1, 1, true);
                    printItem(channel);
                    break;
                case 1:                         // Prepare to edit current
                    edit = true;
                    enc.reset(current[channel], 10, 500, 10, 50, false);
                    printItem(current[channel]);
                    break;
                case 2:                         // dischange
                    if (tchrgr.getMode(channel) == MODE_STOP) {
                        tchrgr.discharge(channel, true);
                        Serial.print(F("disch. channel "));
                        Serial.println(channel);
                    } else if (tchrgr.getMode(channel) == MODE_DISCHARGE) {
                        tchrgr.discharge(channel, false);
                        Serial.print(F("stop disch. channel "));
                        Serial.println(channel);
                    }
                    show_temp = false;
                    show_battery_ms = 0;
                    break;
                case 3:                         // charge
                    if (tchrgr.getMode(channel) == MODE_STOP) {
                        tchrgr.setChargeCurrent(channel, current[channel]);
                        Serial.print(F("charge channel "));
                        Serial.print(channel);
                        Serial.print(F(" by "));
                        Serial.print(current[channel]);
                        Serial.println(F("mA"));
                    } else if (tchrgr.getMode(channel) == MODE_CHARGE) {
                        tchrgr.setChargeCurrent(channel, 0);
                        Serial.print(F("stop charging channel "));
                        Serial.println(channel);
                    }
                    show_temp = false;
                    show_battery_ms = 0;
                    break;
                case 4:                         // toggle fan
                    fan = !fan;
                    tchrgr.fan(fan);
                    Serial.print("Fan ");
                    Serial.println(fan?"ON":"OFF");
                    break;
                case 5:                         // Show temperatures
                    show_temp = !show_temp;
                    if (show_temp) {
                        tchrgr.setChargeCurrent(0, 0);
                        tchrgr.setChargeCurrent(1, 0);
                        tchrgr.discharge(0, false);
                        tchrgr.discharge(1, false);
                    }
                    tchrgr.debugMode(show_temp);
                    break;
               default:
                    break;
            }
        }
    }
    if (edit) {
        uint16_t value = enc.read();
        bool changed = false;
        switch (item) {
            case 0:
                if (value != channel) {
                    channel = value;
                    changed = true;
                }
                break;
            case 1:
                if (value != current[channel]) {
                    current[channel] = value;
                    changed = true;
                }
                break;
            default:
                break;
        }
        if (changed) {
            printItem(value);
        }
    } else {                                    // Main menu
        uint8_t m_item = enc.read();
        if (m_item != item) {                   // Next menu item
            item = m_item;
            printItem(item);
        }
    }

    uint32_t n = millis();
    if (n >= show_battery_ms) {
        show_battery_ms = n + passive_period;
        if (show_temp) {
            Serial.print(F("temp: "));
            for (uint8_t i = 0; i < 3; ++i) {
                uint16_t t = tchrgr.temperature(i);
                Serial.print(t/10);
                Serial.print(".");
                Serial.print(t%10);
                if (i < 2)
                    Serial.print(", ");
            }
            Serial.println("");
            show_battery_ms = n + temp_period;
        } else {
            for (uint8_t i = 0; i < 2; ++i) {
                Serial.print(i);
                Serial.print(": ");
                Serial.print(tchrgr.mV(i));
                Serial.print(F(" mV, temp. "));
                uint16_t t = tchrgr.temperature(i);
                Serial.print(t/10);
                Serial.print(".");
                Serial.print(t%10);
                Serial.print(" ");
                switch (tchrgr.getMode(i)) {
                    case MODE_DISCHARGE:
                    case MODE_CHARGE:
                        show_battery_ms = n + active_period;
                        Serial.print(tchrgr.mA(i));
                        Serial.print(" mA, ");
                        break;
                    default:
                        break;
                }
            }
            Serial.print(F("Ambient: "));
            uint16_t t = tchrgr.temperature(2);
            Serial.print(t/10);
            Serial.print(".");
            Serial.println(t%10);
        }
    }
    delay(10);
}

/*
 * The TIM1 overflow interrupt handler.
 */
static volatile uint16_t counter = 0;
ISR(TIMER1_OVF_vect) {
    if (++counter > 500/4) {                                // 4 times per second.
        counter = 0;
        TIMSK1 &= ~_BV(TOIE1);                              // disable the overflow interrupts
        tchrgr.keepCurrent(0);                              // Manage the power of cnannel "A"
        TIMSK1 |= _BV(TOIE1);                               // enable the the overflow interrupts
    } else if (counter == 500/8) {
        TIMSK1 &= ~_BV(TOIE1);                              // disable the overflow interrupts
        tchrgr.keepCurrent(1);                              // Manage the channel "B"
        TIMSK1 |= _BV(TOIE1);                               // enable the the overflow interrupts
    }
}
