#include "hw.h"

static RENC * re = 0;

// Encoder interrupt handler
static void rotEncChange(void) {
    re->encoderIntr();
}

void HW::init(tSensorOrder order) {
    TWCHARGER::init();
    dspl.begin();
    uint8_t sensors = TWCHARGER::discoverSensors();
    if (sensors == 3) TWCHARGER::orderSensors(order);
    re = &encoder;
    attachInterrupt(digitalPinToInterrupt(RENC_M_PIN), rotEncChange, CHANGE);
	encoder.init();
}
