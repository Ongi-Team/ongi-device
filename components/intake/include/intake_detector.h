#ifndef INTAKE_DETECTOR_H
#define INTAKE_DETECTOR_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

typedef enum {
    INTAKE_PENDING = 0,
    INTAKE_DETECTED,
    INTAKE_TIMEOUT
} IntakeResult;

typedef struct {
    esp_err_t (*init)(void);
    IntakeResult (*wait)(uint8_t slot_id, TickType_t timeout_ticks);
} IntakeDetector;

#endif // INTAKE_DETECTOR_H