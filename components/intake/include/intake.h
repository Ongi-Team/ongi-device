#ifndef INTAKE_H
#define INTAKE_H

#include <stdint.h>
#include "esp_err.h"

esp_err_t intake_init(void);
esp_err_t intake_read_slot_raw(uint8_t slot_id, int16_t *out_raw);

#endif // INTAKE_H
