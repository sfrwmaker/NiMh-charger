#ifndef _TYPES_H_
#define _TYPES_H_

typedef enum {
    CH_SLOW = 0, CH_RESTORE, CH_FAST 
} tChargeType;

typedef enum {
    PH_CHECK = 0, PH_DISCHARGE, PH_PRECHARGE, PH_CHARGE,
    PH_POSTCHARGE, PH_KEEP
} tPhase;

typedef enum {
    CODE_UNKNOWN = 0, CODE_OK, CODE_OVERHEAT, CODE_MAX_VOLTAGE, CODE_HS_OVERHEAT 
} tFinish;

// The themperature sensors order on the oneWire bus.
// A - channel "A" (0-th channel)
// B - channel "B" (1-st channel)
// H - heat sink sensor (2-nd channel)
typedef enum {
    SO_ABH = 0, SO_AHB, SO_BAH, SO_BHA, SO_HAB, SO_HBA
} tSensorOrder;

#endif
