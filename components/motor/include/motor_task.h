#ifndef MOTOR_TASK_H
#define MOTOR_TASK_H

#include "freertos/FreeRTOS.h"
#include "esp_err.h"

#define MOTOR_TASK_STACK_SIZE 4096
#define MOTOR_TASK_PRIORITY 5

esp_err_t motor_init(void);
void motor_task(void *arg);
BaseType_t create_motor_task(void);

#endif // MOTOR_TASK_H