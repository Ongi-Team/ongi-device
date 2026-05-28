#ifndef EVENT_SENDER_H
#define EVENT_SENDER_H

#include "esp_err.h"
#include "event_model.h"

esp_err_t event_sender_send(const MedicationEvent *event);

#endif // EVENT_SENDER_H
