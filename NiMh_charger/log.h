#ifndef _LOG_H_
#define _LOG_H_

#include <Arduino.h>
#include "battery.h"
#include "hw.h"

void logBegin(void);
void logTimestamp(void);
void logMessage(__FlashStringHelper *msg);
void logPhase(uint8_t index, uint8_t phase, bool lf = true);
void logBatteryStatus(uint8_t index, BATTERY *b, HW *core, int16_t temp);
void logFan(int16_t hs_temp, bool on);
void logComplete(uint8_t index, __FlashStringHelper *msg);
void logComplete(uint8_t index, const char *msg);

#endif
