#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>

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

#endif  // SCHEDULE_H