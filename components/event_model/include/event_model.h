#ifndef EVENT_MODEL_H
#define EVENT_MODEL_H

#include <stdint.h>

typedef enum {
    MEDICATION_TAKEN,
    MEDICATION_MISSED,
} MedicationStatus;

typedef struct {
    uint8_t slot_id;
    MedicationStatus status;
    int64_t timestamp; // Unix epoch time in milliseconds, or 0 if time is not synced
} MedicationEvent;

#endif // EVENT_MODEL_H
