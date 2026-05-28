#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include "esp_err.h"
#include "event_model.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define EVENT_QUEUE_LENGTH 10
#define EVENT_QUEUE_SEND_TIMEOUT_MS 100
#define EVENTQUEUE_TASK_STACK_SIZE 8192
#define EVENTQUEUE_TASK_PRIORITY 5

esp_err_t event_queue_init(void);
esp_err_t event_queue_push(const MedicationEvent* event);
QueueHandle_t get_event_queue(void);

#endif // EVENT_QUEUE_H
