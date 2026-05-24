#ifndef DISPENSE_H
#define DISPENSE_H

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define DISPENSE_QUEUE_LENGTH 10
#define DISPENSE_TASK_STACK_SIZE 4096
#define DISPENSE_TASK_PRIORITY 5

typedef struct {
    uint8_t slot_id;
} DispenseEvent;

esp_err_t dispense_init(void);
esp_err_t dispense_enqueue(uint8_t slot_id);

QueueHandle_t get_dispense_queue(void);

#endif // DISPENSE_H