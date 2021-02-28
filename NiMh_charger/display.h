#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Time.h>
#include <TimeLib.h>
#include "types.h"

#define COLS    (20)
#define ROWS    (4)

//----------------------------------------- class lcd display for NiMh charger -------------------------------
class DSPL : public LiquidCrystal_I2C {
    public:
        DSPL(uint8_t i2c_addr, uint8_t br_pin = 255) : LiquidCrystal_I2C(i2c_addr, COLS, ROWS) { this->br_pin = br_pin; }
        void        begin(void);
        void        slotStatus(uint8_t index, uint16_t cap, tChargeType type, bool no_discharge);
        void        phaseName(tPhase phase, uint8_t index, uint16_t cap, tChargeType type);
        void        complete(uint8_t index, tFinish code, uint16_t cap, tChargeType type);
        void        chargingInfo(tPhase phase, uint8_t index, uint16_t mV, uint16_t mA, uint16_t charged, uint16_t temp);
        void        tempInfo(tPhase phase, uint8_t index, uint16_t charged, uint16_t temp, uint16_t mV, uint16_t mA);
        void        timeInfo(tPhase phase, uint8_t index, time_t elapsed, time_t remains, uint16_t mV, uint16_t mA);
        void        aboutInfo(uint16_t temp);
        void        slotMenu(uint8_t slot);
		void		setupMode(uint8_t index, uint8_t mode, uint16_t value);
        void        fanState(bool on);
        void        sensorState(uint16_t s_data[3]);
        uint8_t     updateBrightness(void);
        void        setBrightness(uint8_t br);
        void        backlight(bool on = true)               { back_light = on; }
        bool        isBacklight(void)                       { return back_light; }
    private:
        void        printTime(time_t sec);
        void        customCharacters(void);
        void        fillRow(uint16_t pos);
        void        drawSlotInfo(uint16_t cap, tChargeType type);
        void        drawTempInfo(uint16_t charged, uint16_t temp);
        void        drawChargeInfo(uint16_t mV, uint16_t mA);
        uint8_t     br_pin;
        uint8_t     lcd_brightness  = 0;
        bool        back_light      = false;
        char        buff[COLS+2]    = {0};
        uint32_t    update_br       = 0;                    // When to update lcd brightness
};

#endif
