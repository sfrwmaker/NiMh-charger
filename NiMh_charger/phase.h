#ifndef _PHASE_H_
#define _PHASE_H_
#include <Time.h>
#include <TimeLib.h>
#include "types.h"
#include "twin_charger.h"
#include "battery.h"
#include "stat.h"

class PHASE {
    public:
        PHASE(void)                                         { }
        virtual time_t      init(uint8_t index, TWCHARGER *pCharger, BATTERY *b)    { return 0; }
        virtual uint32_t    run(uint8_t index, TWCHARGER *pCharger, BATTERY *b) = 0; // Returns time of next loop
};

class CHECK: public PHASE {
    public:
        CHECK(void)                                         { }
        virtual uint32_t    run(uint8_t index, TWCHARGER *pCharger, BATTERY *b);
    private:
        const uint32_t period = 5000;
};

class DISCHARGE: public PHASE {
    public:
        DISCHARGE(void)                                     { }
        virtual time_t      init(uint8_t index, TWCHARGER *pCharger, BATTERY *b);
        virtual uint32_t    run(uint8_t index, TWCHARGER *pCharger, BATTERY *b);
    private:
        uint32_t        pause_period    = 1000;
        const uint32_t  disch_period    = 10000;
};

class PRECHARGE: public PHASE {
    public:
        PRECHARGE(void)                                     { }
        virtual time_t      init(uint8_t index, TWCHARGER *pCharger, BATTERY *b);
        virtual uint32_t    run(uint8_t index, TWCHARGER *pCharger, BATTERY *b);
    private:
        const uint32_t  charge_period   = 300;
        const uint32_t  pause_period    = 700;
        const uint32_t  test_period     = 30000;            // Wait for 30 seconds before finish phase
};

class CHARGE: public PHASE {
    public:
        CHARGE(void)                                        { }
        virtual time_t      init(uint8_t index, TWCHARGER *pCharger, BATTERY *b);
        virtual uint32_t    run(uint8_t index, TWCHARGER *pCharger, BATTERY *b);
    private:
        const uint32_t  period      = 60000;                 // 60 seconds charging period
};

class POSTCHARGE: public PHASE {
    public:
        POSTCHARGE(void)                                    { }
        virtual time_t      init(uint8_t index, TWCHARGER *pCharger, BATTERY *b);
        virtual uint32_t    run(uint8_t index, TWCHARGER *pCharger, BATTERY *b);
    private:
        const uint32_t  period          = 60000;            // 60 seconds charging period
        const uint32_t  pause_period    = 10000;
};

class KEEPCHARGE: public PHASE {
    public:
        KEEPCHARGE(void)                                    { }
        virtual time_t      init(uint8_t index, TWCHARGER *pCharger, BATTERY *b);
        virtual uint32_t    run(uint8_t index, TWCHARGER *pCharger, BATTERY *b);
    private:
        const uint32_t  charge_period   = 300;
        const uint32_t  pause_period    = 10000;
};

#endif
