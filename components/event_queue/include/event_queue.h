#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <stdint.h>

typedef enum {
    MEDICATION_TAKEN,
    MEDICATION_MISSED,
} MedicationStatus;

typedef struct {
    uint8_t slot_id;
    MedicationStatus status;
    int64_t timestamp;
} MedicationEvent;

#endif // EVENT_QUEUE_H