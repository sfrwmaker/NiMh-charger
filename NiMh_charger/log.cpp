#include <Time.h>
#include <TimeLib.h>
#include "log.h"

static const char phase_name[5][12] PROGMEM = {
    "discharge  ",
    "precharge  ",
    "charge     ",
    "postcharge ",
    "complete   "
};

void logBegin(void) {
#ifdef LOG_ENABLE
    Serial.begin(115200);
    logMessage(F("Started"));
#endif
}

void logTimestamp(void) {
    time_t n = now();
    if (n > 86400) {                                        // Longer that one day
        Serial.print(n/86400);
        Serial.print("d");
        n %= 86400;
    }
    char buff[12];
    uint8_t h = n / 3600;
    n %= 3600;
    uint8_t m = n / 60;
    uint8_t s = n % 60;
    sprintf(buff, "%02dh%02dm%02ds: ", h, m, s);
    Serial.print(buff);
}

void logMessage(__FlashStringHelper *msg) {
   logTimestamp();
   Serial.println(msg); 
}

void logPhase(uint8_t index, uint8_t phase, bool lf) {
#ifdef LOG_ENABLE
    if (phase >= 5) return;
    index &= 1;                                             // Ensure index is in interval 0..1
    logTimestamp();
    Serial.print((const __FlashStringHelper*)&phase_name[phase]);
    Serial.print(" ");
    Serial.print((char)(index+'A'));
    if (lf)
        Serial.println("");
#endif
}

void logBatteryStatus(uint8_t index, BATTERY *b, HW *core, int16_t temp) {
#ifdef LOG_ENABLE
    uint8_t phase_index = b->phaseIndex();
    if (phase_index == 0) return;

    if (phase_index == 0 && phase_index >= 5) {
        Serial.print(F("unknown "));
        Serial.print(index);
    } else {
        logPhase(index, phase_index-1, false);
    }
    Serial.print(" ");
    Serial.print(b->capacity());
    Serial.print(F("mAh. "));
    Serial.print(b->averageVoltage());
    Serial.print(F(" mV, "));
    Serial.print(b->averageCurrent());
    Serial.print(F(" mA, dis-charged "));
    Serial.print(core->discharged(index));
    Serial.print(F(" mAh, charged "));
    Serial.print(core->charged(index));
    Serial.print(F(" mAh, temp. "));
    Serial.print(temp/10);
    Serial.print(".");
    Serial.println(temp%10);
#endif
}

void logFan(int16_t hs_temp, bool on) {
#ifdef LOG_ENABLE
    logTimestamp();
    Serial.print(F(" Heat sink temp. "));
    Serial.print(hs_temp/10);
    Serial.print(".");
    Serial.print(hs_temp%10);
    Serial.print(F(", fan o"));
    if (on) {
        Serial.println(F("n"));
    } else {
        Serial.println(F("ff"));
    }
#endif
}

void logComplete(uint8_t index, __FlashStringHelper *msg) {
#ifdef LOG_ENABLE
    index &= 1;                                             // Ensure index is in interval 0..1
    logTimestamp();
    Serial.print((const __FlashStringHelper*)&phase_name[4]);
    Serial.print(" ");
    Serial.print((char)(index+'A'));
    Serial.print(" ");
    Serial.println(msg);
#endif
}

void logComplete(uint8_t index, const char *msg) {
#ifdef LOG_ENABLE
    index &= 1;                                             // Ensure index is in interval 0..1
    logTimestamp();
    Serial.print((const __FlashStringHelper*)&phase_name[4]);
    Serial.print(" ");
    Serial.print((char)(index+'A'));
    Serial.print(" ");
    Serial.println(msg);
#endif
}
