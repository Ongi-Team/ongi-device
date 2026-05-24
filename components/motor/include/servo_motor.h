#ifndef SERVO_MOTOR_H
#define SERVO_MOTOR_H

#include "esp_err.h"

esp_err_t servo_motor_open(int slot_id);
esp_err_t servo_motor_close(int slot_id);

#endif // SERVO_MOTOR_H