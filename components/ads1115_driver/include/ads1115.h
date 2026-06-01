#ifndef ADS1115_H
#define ADS1115_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define ADS1115_ADDR_LO       0x48
#define ADS1115_ADDR_HI       0x49
#define ADS1115_CHANNEL_COUNT 4

/**
 * Initialize the I2C master bus. Must be called once before ads1115_detect or
 * ads1115_read_channel.
 */
esp_err_t ads1115_bus_init(int sda_gpio, int scl_gpio);

/**
 * Probe the bus to check whether an ADS1115 device is present at addr.
 * addr must be ADS1115_ADDR_LO or ADS1115_ADDR_HI.
 */
esp_err_t ads1115_detect(uint8_t addr, bool *found);

/**
 * Trigger a single-shot conversion on channel (0..3) and return the raw
 * signed 16-bit result. Returns ESP_ERR_INVALID_ARG for out-of-range inputs.
 */
esp_err_t ads1115_read_channel(uint8_t addr, uint8_t channel, int16_t *out);

#endif // ADS1115_H
