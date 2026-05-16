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

extern QueueHandle_t dispense_event_queue;

esp_err_t dispense_init(void);
esp_err_t dispense_enqueue(uint8_t slot_id);
void dispense_task(void *arg);

#endif // DISPENSE_H