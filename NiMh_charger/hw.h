#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include "config.h"
#include "display.h"
#include "twin_charger.h"
#include "phase.h"
#include "stat.h"
#include "encoder.h"
#include "cfg.h"

class HW : public TWCHARGER {
    public:
        HW(void) : dspl(LCD_I2C_ADDR, LCD_BR_PIN),
            TWCHARGER(TWCH_ONE_WIRE, TWCH_FAN_PIN), encoder(RENC_M_PIN, RENC_S_PIN, RENC_B_PIN)   { }
        void        init(tSensorOrder order);
        DSPL        dspl;
        RENC        encoder;
        CONFIG      cfg;
};

#endif
