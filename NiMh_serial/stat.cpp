#include "stat.h"

int32_t EMP_AVERAGE::average(int32_t value) {
    uint8_t round_v = emp_k >> 1;
    update(value);
    return (emp_data + round_v) / emp_k;
}

void EMP_AVERAGE::update(int32_t value) {
    uint8_t round_v = emp_k >> 1;
    emp_data += value - (emp_data + round_v) / emp_k;
}

int32_t EMP_AVERAGE::read(void) {
    uint8_t round_v = emp_k >> 1;
    return (emp_data + round_v) / emp_k;
}
