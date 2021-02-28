#include "display.h"
#include "config.h"
/*
static const char M0[11] PROGMEM = "No battery";
static const char M1[10] PROGMEM = "Discharge";
static const char M2[10] PROGMEM = "Precharge";
static const char M3[5]  PROGMEM = "Slow";
static const char M4[5]  PROGMEM = "Fast";
static const char M5[7]  PROGMEM = "Fast B";
static const char M6[5]  PROGMEM = "Post";
static const char M7[5]  PROGMEM = "Keep";
static const char *phase_name[8] = {M0, M1, M2, M3, M4, M5, M6, M7};
*/
/*
 * LCD custom characters. Should be in order with phase type (see types.h)
 * PH_CHECK,   PH_DISCHARGE, PH_PRECHARGE,  PH_SLOW,
 * PH_RESTORE, PH_FAST,      PH_POSTCHARGE, PH_KEEP
 * 
 * 0-th index, channel "A" shown in the bottom line, 1-st index, channel "B" shown in the upper line
 */
static const uint8_t M[8][8] PROGMEM = {
    {   0b00000000,                                             // No battery
        0b00001110,
        0b00011011,
        0b00010001,
        0b00010001,
        0b00010001,
        0b00010001,
        0b00011111
    },
    {   0b00000000,                                             // Discharge
        0b00001110,
        0b00001110,
        0b00001110,
        0b00001110,
        0b00011111,
        0b00001110,
        0b00000100
    },
    {   0b00000000,                                             // Precharge
        0b00001110,
        0b00011011,
        0b00010101,
        0b00011111,
        0b00010101,
        0b00010001,
        0b00011111
    },
    {   0b00000000,                                             // Slow charge
        0b00000001,
        0b00000010,
        0b00000111,
        0b00000010,
        0b00000100,
        0b00000000,
        0b00000000
    },
    {   0b00000000,                                             // Restore
        0b00000100,
        0b00000100,
        0b00001110,
        0b00001110,
        0b00011111,
        0b00011111,
        0b00001110
    },
    {   0b00000011,                                             // Fast Charge
        0b00000110,
        0b00001100,
        0b00011111,
        0b00000011,
        0b00000110,
        0b00001100,
        0b00011000
    },
    {   0b00000100,                                             // Post charge
        0b00001110,
        0b00011111,
        0b00001110,
        0b00001110,
        0b00001110,
        0b00001110,
        0b00001110
    },
    {   0b00000000,                                             // Keep
        0b00001110,
        0b00011111,
        0b00011111,
        0b00011111,
        0b00011111,
        0b00011111,
        0b00011111
    }
};

static const char M_name[8][12] PROGMEM = {
    "Checking   ",
    "Discharge  ",
    "Precharge  ",
    "Slow charge",
    "Restore    ",
    "Fast charge",
    "Post charge",
    "Complete   "
};

static const char types[3][7] PROGMEM = {
    "Slow  ",
    "Rest. ",
    "Fast  "
};

static const char F_code[5][12] PROGMEM = {
    "Aborted    ",
    "Charged    ",
    "Overheat   ",
    "Max voltage",
    "High temer."
};

#ifdef TWIST_DISPLAY
static const char slot_name[2][6] PROGMEM = {
    "Lower",
    "Upper"
};
#else
static const char slot_name[2][6] PROGMEM = {
    "Upper",
    "Lower"
};
#endif

void DSPL::begin(void) {
    LiquidCrystal_I2C::begin();
    if (br_pin < 255) {
        pinMode(br_pin, OUTPUT);
        analogWrite(br_pin, lcd_brightness);
    }
    LiquidCrystal_I2C::clear();
    customCharacters();
}

uint8_t DSPL::updateBrightness(void) {
    if (millis() < update_br) return;
    update_br = millis() + 100;
    uint8_t br = back_light?255:0;
    if (br != lcd_brightness) {
        if (br > lcd_brightness) {
            ++lcd_brightness;
        } else {
            --lcd_brightness;
        }
        analogWrite(br_pin, lcd_brightness);
    }
    return lcd_brightness;
}

void DSPL::setBrightness(uint8_t br) {
    lcd_brightness = br;
    analogWrite(br_pin, br);
    back_light = (br >= 128);
}

void DSPL::slotStatus(uint8_t index, uint16_t cap, tChargeType type, bool no_discharge) {
    setCursor(0, index*ROWS/2);
    drawSlotInfo(cap, type);
    if (no_discharge) {
        setCursor(15, index*ROWS/2);
        write('!');
    }
    if (ROWS > 2) {
        setCursor(0, index*2+1);
        fillRow(0);
    }
}

void DSPL::phaseName(tPhase phase, uint8_t index, uint16_t cap, tChargeType type) {
    if (ROWS > 2) {
        setCursor(0, index*ROWS/2+1);
        drawSlotInfo(cap, type);
        setCursor(0, index*ROWS/2);
    } else {
        setCursor(0, index);
    }
    write(phase);
    write(' ');
    uint8_t c = 0;
    for ( ; c < 16; ++c) {
        char sym = pgm_read_byte(&M_name[(uint8_t)phase][c]);
        if (sym ==0) break;
        write(sym);
    }
    fillRow(c+2);
}

void DSPL::complete(uint8_t index, tFinish code, uint16_t cap, tChargeType type) {
    if (ROWS > 2) {
        setCursor(0, index*ROWS/2+1);
        drawSlotInfo(cap, type);
        setCursor(0, index*ROWS/2);
    } else {
        setCursor(0, index);
    }
    write(PH_KEEP);
    write(' ');
    uint8_t c = 0;
    if (code <= 4) {
        for ( ; c < 16; ++c) {
            char sym = pgm_read_byte(&F_code[code][c]);
            if (sym ==0) break;
            write(sym);
        }
    }
    print("("); print((uint8_t)code); print(")");
    fillRow(c+5);
}

void DSPL::chargingInfo(tPhase phase, uint8_t index, uint16_t mV, uint16_t mA, uint16_t charged, uint16_t temp) {
    if (ROWS > 2) {
        setCursor(0, index*ROWS/2+1);
        write(' ');                                             // First symbol in the line shoulc be phase code
        drawTempInfo(charged, temp);
        setCursor(0, index*ROWS/2);
    } else {
        setCursor(0, index);
    }
    write(phase);
    drawChargeInfo(mV, mA);
}

void DSPL::tempInfo(tPhase phase, uint8_t index, uint16_t charged, uint16_t temp, uint16_t mV, uint16_t mA) {
    if (ROWS > 2) {
        setCursor(0, index*ROWS/2+1);
        write(' ');
        drawChargeInfo(mV, mA);
        setCursor(0, index*ROWS/2);
    } else {
        setCursor(0, index);
    }
    print((char)phase);
    drawTempInfo(charged, temp);
}

void DSPL::timeInfo(tPhase phase, uint8_t index, time_t elapsed, time_t remains, uint16_t mV, uint16_t mA) {
    if (ROWS > 2) {
        setCursor(0, index*ROWS/2+1);
        write(' ');
        drawChargeInfo(mV, mA);
        setCursor(0, index*ROWS/2);
    } else {
        setCursor(0, index);
    }
    printTime(elapsed);
    print(F("  ("));
    printTime(remains);
    print(F(")"));
}

void  DSPL::aboutInfo(uint16_t temp) {
    setCursor(0, 0);
    print(F("NiMh chrgr "));
    print(VERSION);
    setCursor(COLS-6, ROWS-1);
    sprintf(buff, "%2d.%1d%cC", temp/10, temp%10, DEGREE_CODE);
    print(buff);
}

void DSPL::printTime(time_t sec) {
    if (sec > 86400) {
        uint8_t d = sec/86400;
        sec %= 86400;
        uint8_t h = sec/3600;
        sprintf(buff, "%2dd%02dh", d, h);
    } else if (sec >= 3600) {
        uint8_t h = sec/3600;
        sec %= 3600;
        uint8_t m = sec/60;
        sprintf(buff, "%2dh%02dm", h, m);
    } else {
        sprintf(buff, "%2dm%02ds", sec/60, sec%60);
    }
    print(buff);
}

void DSPL::slotMenu(uint8_t slot) {
    setCursor(0, 0);
    print(F("Select battery"));
    setCursor(5, 1);
    print((char)(slot+'A'));
    print(F(" ("));
    for (uint8_t c = 0 ; c < 16; ++c) {
        char sym = pgm_read_byte(&slot_name[slot][c]);
        if (sym ==0) break;
        write(sym);
    }
    print(")");
    fillRow(16);
}

void DSPL::setupMode(uint8_t index, uint8_t menu, uint16_t value) {
    clear();
    print(F("Battery ")); print((char)(index+'A')); print(F(" setup"));
    setCursor(1, 1);
    switch (menu) {
        case 1:
            print(F("Capacity "));
            print(value);
            break;
        case 2:
            print(F("Charge "));
            if (value < 3) {
                for (uint8_t c = 0 ; c < 6; ++c) {
                    char sym = pgm_read_byte(&types[value][c]);
                    write(sym);
                }
                write(value+3);
            }
            break;
        case 3:
            print(F("Loops "));
            print(value);
            break;
        case 4:
            if (value & bf_nodischarge)
                print(F("NO "));
            print(F("discharge"));
            break;
        case 5:
            print(F("Back"));
        default:
            break;
    }
}

void DSPL::fanState(bool on) {
    setCursor(0, 0);
    print(F("Fan Test"));
    setCursor(0, 1);
    if (on) print(F("ON")); else print(F("OFF"));
}

void DSPL::sensorState(uint16_t s_data[3]) {
    setCursor(0, 0);
    print(F("Sensor data"));
    setCursor(0, 1);
    for (uint8_t i = 0; i < 3; ++i) {
        sprintf(buff, "%2d.%1d ", s_data[i]/10, s_data[i]%10);
        print(buff);
    }
}

void DSPL::customCharacters(void) {
    uint8_t custom[8];
    for (uint8_t i = 0; i < 8; ++i) {
        for (uint8_t c = 0 ; c < 8; ++c) {
            custom[c] = pgm_read_byte(&M[i][c]);
        }
        createChar(i, custom);
    }
}

void  DSPL::fillRow(uint16_t pos) {
    for (uint8_t i = pos; i < COLS; ++i) write (' ');
}

void DSPL::drawSlotInfo(uint16_t cap, tChargeType type) {
    sprintf(buff, "%4d mAh  ", cap);
    print(buff);
    for (uint8_t c = 0 ; c < 16; ++c) {
        char sym = pgm_read_byte(&types[(uint8_t)type][c]);
        if (sym == 0) break;
        write(sym);
   }
   fillRow(16);
}

void DSPL::drawTempInfo(uint16_t charged, uint16_t temp) {
    sprintf(buff, " %3d mAh %2d.%1d%cC ", charged, temp/10, temp%10, DEGREE_CODE);
    buff[15] = '\0';
    print(buff);
}

void DSPL::drawChargeInfo(uint16_t mV, uint16_t mA) {
    uint16_t decV = mV % 1000;
    decV /= 10;
    sprintf(buff, " %1d.%02d V %4d mA", mV/1000, decV, mA);
    print(buff);
    fillRow(16);
}
