#ifndef MOTOR_TASK_H
#define MOTOR_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"

#define MOTOR_TASK_STACK_SIZE 4096
#define MOTOR_TASK_PRIORITY 5
#define MOTOR_COMMAND_QUEUE_LENGTH 4
#define MOTOR_COMMAND_SEND_TIMEOUT_MS 100
#define MOTOR_OPEN_ALL_AUTO_CLOSE_TIMEOUT_MS 600000

typedef enum {
    MOTOR_COMMAND_OPEN_ALL = 0,
    MOTOR_COMMAND_CLOSE_ALL,
} MotorCommandType;

typedef struct {
    MotorCommandType type;
} MotorCommand;

esp_err_t motor_init(void);
esp_err_t motor_command_enqueue(MotorCommand command);
QueueHandle_t get_motor_command_queue(void);
void motor_task(void *arg);
BaseType_t create_motor_task(void);

#endif // MOTOR_TASK_H
