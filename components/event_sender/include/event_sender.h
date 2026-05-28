#ifndef EVENT_SENDER_H
#define EVENT_SENDER_H

#include "esp_err.h"
#include "event_model.h"
#include <stdbool.h>

esp_err_t event_sender_send(const MedicationEvent *event);
bool event_sender_is_transient_error(esp_err_t err);

#endif // EVENT_SENDER_H
