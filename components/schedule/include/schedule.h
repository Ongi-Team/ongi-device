#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>
#include <stdbool.h>

#define SCHEDULE_SLOT_COUNT 8

typedef struct {
    uint8_t slot_id;
    uint8_t hour;
    uint8_t minute;
    bool triggered;
} SlotEntry;

typedef struct {
    uint8_t slot_id;
} DispenseEvent;

// SlotEntry Test
void init_test_slots(SlotEntry *slots, size_t count);

#endif  // SCHEDULE_H