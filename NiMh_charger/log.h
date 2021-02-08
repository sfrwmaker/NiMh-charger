#ifndef _LOG_H_
#define _LOG_H_

#include <Arduino.h>
#include "battery.h"
#include "hw.h"

void logBegin(void);
void logMessage(__FlashStringHelper *msg);
void logPhase(uint8_t index, uint8_t phase, bool lf = true);
void logBatteryStatus(uint8_t index, BATTERY *b, HW *core, int16_t temp);
void logComplete(uint8_t index, tFinish code);

#endif
