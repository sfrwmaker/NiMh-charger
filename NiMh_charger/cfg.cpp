#include <Arduino.h>
#include <EEPROM.h>
#include "cfg.h"
#include "config.h"
#include "types.h"

bool CONFIG::readConfig(tCfg &rec) {
    uint8_t *buff = (uint8_t *)&rec;
    
    for (uint8_t i = 0; i < sizeof(struct record); ++i) {
        buff[i] = EEPROM.read(i);
    }
    if (crc(buff)) {                                            // CRC of the record is OK
        return true;
    }
    // Create default config
    rec.capacity[0] = rec.capacity[1]   = BATT_CAPACITY;
    rec.type[0]     = rec.type[1]       = CH_SLOW;
    rec.loops[0]    = rec.loops[1]      = 0;
    rec.bit_flag[0] = 0;
    return false;
}

void  CONFIG::saveConfig(tCfg &rec) {
    uint8_t *buff = (uint8_t *)&rec;
    crc(buff, true);
    for (uint8_t i = 0; i < sizeof(struct record); ++i) {
        EEPROM.write(i, buff[i]);
    }
}

bool CONFIG::crc(uint8_t *buff, bool write) {
    uint32_t summ = 151;
    for (uint8_t i = 4; i < sizeof(struct record); ++i) {
        summ <<= 2;
        summ += buff[i];
    }
    if (write) {
        for (int8_t i = 3; i >= 0; --i) {
            buff[i] = summ & 0xff;
            summ >>= 8;
        }
        return true;
    }
    uint32_t s = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        s <<= 8;
        s |= buff[i];
    }
    return (s == summ);
}
