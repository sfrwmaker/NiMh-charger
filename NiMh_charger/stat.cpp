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

HISTORY::HISTORY(uint8_t h_length) {
    len = 0;
    this->h_length = h_length;
    if (h_length > 0)
        queue = new uint16_t[h_length];
}

void HISTORY::update(uint16_t item) {
    if (len < h_length) {
        queue[len++] = item;
    } else {
        queue[index] = item;
        if (++index >= h_length)                        // Use ring buffer
            index = 0;
    }
}

uint16_t HISTORY::read(void) {
    uint32_t sum = 0;
    if (len == 0) return 0;
    if (len == 1) return queue[0];
    for (uint8_t i = 0; i < len; ++i) sum += queue[i];
    sum += len >> 1;                                    // round the average
    sum /= len;
    return uint16_t(sum);
}

uint16_t HISTORY::average(uint16_t item) {
    update(item);
    return read();
}

float HISTORY::dispersion(void) {
    if (len < 3) return 1000;
    long sum = 0;
    long avg = read();
    for (uint8_t i = 0; i < len; ++i) {
        long q = queue[i];
        q -= avg;
        q *= q;
        sum += q;
    }
    sum += len << 1;
    float d = (float)sum / (float)len;
    return d;
}

/*
 * Calculate gradient using linear approximation by Ordinary Least Squares method
 * Y = a * X + b, where a and b are double coefficients
 * a = (N * sum(Xi*Yi) - sum(Xi) * sum(Yi)) / ( N * sum(Xi^2) - (sum(Xi))^2)
 * b = 1/N * (sum(Yi) - a * sum(Xi))
 * gradient is an 'a' parameter
 * 
 */
int32_t HISTORY::gradient(void) {
    if (len < 3) return 0;                              // The queue is almost empty
    int32_t sx, sxx, sxy, sy;
    sx = sxx = sxy = sy = 0;
    uint8_t item = index;                               // The first item in the queue (ring queue)
    uint16_t avg = read();                              // Average value
    for (uint8_t i = 1; i <= len; ++i) {
        int32_t value = (int32_t)queue[item] - (int32_t)avg; // To increase accuracy, calculate difference between queue data and average value
        sx    += i;
        sxx += i*i;
        sxy   += i*value;
        sy    += value;
        if (++item >= h_length) item = 0;
    }
    int32_t numerator   = len * sxy - sx * sy;
    int32_t denominator = len * sxx - sx * sx;
    return (numerator*100/denominator);
}

void HISTORY::dump(void) {
    if (len < h_length) {                               // Partially loaded queue
        for (uint8_t i = 0; i < len; ++i) {
            Serial.print(queue[i]);
            Serial.print(", ");
        }
    } else {                                            // Completely loaded queue
        uint8_t item = index;                           // The first item in the queue (ring queue)
        for (uint8_t i = 1; i <= len; ++i) {
            Serial.print(queue[item]);
            Serial.print(", ");
            if (++item >= h_length) item = 0;
        }
    }
    Serial.print(F("Gradient = "));
    Serial.println(gradient());
}
