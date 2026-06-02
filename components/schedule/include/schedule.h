#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>
#include <stdbool.h>

#define SCHEDULE_SLOT_COUNT 6
#define SCHEDULE_TASK_STACK_SIZE 4096
#define SCHEDULE_TASK_PRIORITY 5

typedef struct {
    uint8_t slot_id;
    uint8_t hour;
    uint8_t minute;
    bool triggered;
} SlotEntry;

void schedule_task(void *arg);

#endif  // SCHEDULE_H
