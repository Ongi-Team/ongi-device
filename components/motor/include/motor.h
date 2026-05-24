#ifndef MOTOR_H
#define MOTOR_H

#include "freertos/FreeRTOS.h"

#define MOTOR_TASK_STACK_SIZE 1024
#define MOTOR_TASK_PRIORITY 5

void motor_task(void *arg);
BaseType_t create_motor_task(void);

#endif // MOTOR_H