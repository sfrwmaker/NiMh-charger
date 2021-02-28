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

class HISTORY {
    public:
        HISTORY(uint8_t h_length);
        virtual         ~HISTORY(void)                  { if (h_length > 0) delete [] queue; }
        uint8_t         length(void)                    { return len; }
        void            reset(void)                     { len = 0; index = 0; }
        uint16_t        read(void);                     // Calculate the average value
        void            update(uint16_t item);          // Add new entry to the history
        uint16_t        average(uint16_t item);         // Add new value and calculate the average value
        float           dispersion(void);               // Calculate the math dispersion
        int32_t         gradient(void);                 // approximating the history with the line (y = ax+b). Return parameter a * 1000
        void            dump(void);                     // Dump history data to the serial port
    private:
        uint16_t        h_length;
        uint16_t        *queue;
        uint8_t         len;                            // The number of elements in the queue
        uint8_t         index;                          // The current element position, use ring buffer
};

#endif
