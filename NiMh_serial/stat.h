#ifndef _STAT_H_
#define _STAT_H_
#include <Arduino.h>

// Exponential average
class EMP_AVERAGE {
    public:
        EMP_AVERAGE(uint8_t h_length = 8)               { emp_k = h_length; emp_data = 0; }
        void            length(uint8_t h_length)        { emp_k = h_length; emp_data = 0; }
        void            reset(void)                     { emp_data = 0; }
        int32_t         average(int32_t value);
        void            update(int32_t value);
        int32_t         read(void);
    private:
        volatile    uint8_t     emp_k       = 8;
        volatile    uint32_t    emp_data    = 0;
};

#endif
