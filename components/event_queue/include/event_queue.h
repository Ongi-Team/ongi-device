#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define EVENT_QUEUE_LENGTH 10
#define EVENTQUEUE_TASK_STACK_SIZE 4096
#define EVENTQUEUE_TASK_PRIORITY 5

typedef enum {
    MEDICATION_TAKEN,
    MEDICATION_MISSED,
} MedicationStatus;

typedef struct {
    uint8_t slot_id;
    MedicationStatus status;
    int64_t timestamp;
} MedicationEvent;

esp_err_t event_queue_init(void);
esp_err_t event_queue_push(const MedicationEvent* event);
QueueHandle_t get_event_queue(void);

#endif // EVENT_QUEUE_H