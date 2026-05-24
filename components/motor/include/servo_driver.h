#ifndef SERVO_DRIVER_H
#define SERVO_DRIVER_H

#include "esp_err.h"

typedef struct {
    esp_err_t (*open)(uint8_t slot_id);
    esp_err_t (*close)(uint8_t slot_id);
} ServoDriver;

#endif // SERVO_DRIVER_H