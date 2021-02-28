#ifndef _CFG_H
#define _CFG_H
#include "types.h"

//------------------------------------------ Configuration record ----------------------------------------------
typedef struct record {
    uint32_t    CRC;
    uint16_t    capacity[2];                                    // The battery capacity, mAh
    tChargeType type[2];                                        // The charging type
    uint8_t     loops[2];                                       // The charging loops, usually 0: do not loop
    uint8_t     bit_flag[2];
} tCfg;

class CONFIG {
    public:
        CONFIG()    { }
        bool        readConfig(tCfg &rec);
        void        saveConfig(tCfg &rec);
    private:
        bool        crc(uint8_t *buff, bool write = false);
};


#endif
