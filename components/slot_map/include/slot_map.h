#ifndef SLOT_MAP_H
#define SLOT_MAP_H

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"

#define SLOT_COUNT 8

typedef struct {
    gpio_num_t servo_gpio;
    uint8_t    ads1115_addr;
    uint8_t    ads1115_channel;
} SlotHwConfig;

/**
 * Returns the hardware config for the given slot_id (1-based, 1..SLOT_COUNT).
 * Returns ESP_ERR_INVALID_ARG if slot_id is out of range.
 */
esp_err_t slot_map_get(uint8_t slot_id, const SlotHwConfig **out);

#endif // SLOT_MAP_H
