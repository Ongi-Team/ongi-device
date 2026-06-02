#ifndef LEDC_SERVO_DRIVER_H
#define LEDC_SERVO_DRIVER_H

#include "servo_driver.h"
#include "esp_err.h"

esp_err_t ledc_servo_driver_init(void);

extern const ServoDriver LEDC_SERVO_DRIVER;

#endif // LEDC_SERVO_DRIVER_H
