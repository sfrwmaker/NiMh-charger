#ifndef _MODE_H_
#define _MODE_H_

#include "hw.h"

class MODE {
    public:
        MODE(HW *pCore)                                     { this->pCore = pCore;  }
        void            setup(MODE* next_mode)              { mode_next = next_mode; }
        virtual void    init(BATTERY b[2])                  { }
        virtual MODE*   loop(BATTERY b[2])                  { return 0; }
        virtual         ~MODE(void)                         { }
        MODE*           returnToMain(void);
    protected:
        void            resetTimeout(void);
        void            setTimeout(uint16_t t);
		bool			isBattDetected(uint8_t index);
        HW*             pCore           = 0;
        uint16_t        timeout_secs    = 0;                // Timeout to return to main mode, seconds
        uint32_t        time_to_return  = 0;                // Time in ms when to return to the main mode
        uint32_t        update_screen   = 0;                // Time in ms when the screen should be updated
        MODE*           mode_next       = 0;                // Next working mode
};

//---------------------- The Main working mode -----------------------------------
class MAIN : public MODE {
    public:
        MAIN(HW *pCore) : MODE(pCore)                      	{ }
        virtual void    init(BATTERY b[2]);
        virtual MODE*   loop(BATTERY b[2]);
    private:
        void            resetDisplay(void);
        uint8_t         dspl_mode   = 0;                    // Display mode: what info to show this time
        uint32_t        change_mode = 0;                    // Time to change display mode
        time_t          turn_off_display    = 0;            // Time when turn lcd baclkight off
        bool            reset_display       = false;
        uint8_t         encoder_pos         = 0;
        const uint32_t  period              = 1000;
        const uint8_t   turn_off_period     = 180;          // Seconds
};
/*
 *  dspl_mode:
 *  0   - phase name
 *  1   - voltage, current
 *  2   - charging or discharging current, temperature
 *  3   - charging times: elapced, remain
 */

//---------------------- The Setup mode ------------------------------------------
class SETUP : public MODE {
    public:
        SETUP(HW *pCore) : MODE(pCore)                      { }
        virtual void    init(BATTERY b[2]);
        virtual MODE*   loop(BATTERY b[2]);
    private:
        tCfg            cfg;                                // The configuration record
        uint8_t         slot    = 2;                        // The battery index: 0, 1; 2 means select slot number
        uint8_t         entity  = 0;                        // The entity to edit 0 - unselected, 1 - capacity, 2 - type, 3 - loops, 4 - back
        const uint32_t  period  = 5000;
};

#endif
