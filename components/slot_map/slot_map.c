#include "slot_map.h"

static const SlotHwConfig s_slot_map[SLOT_COUNT] = {
    {GPIO_NUM_13, 0x48, 0}, // slot_id=1
    {GPIO_NUM_14, 0x48, 1}, // slot_id=2
    {GPIO_NUM_16, 0x48, 2}, // slot_id=3
    {GPIO_NUM_17, 0x48, 3}, // slot_id=4
    {GPIO_NUM_18, 0x49, 0}, // slot_id=5
    {GPIO_NUM_19, 0x49, 1}, // slot_id=6
};

esp_err_t slot_map_get(uint8_t slot_id, const SlotHwConfig **out)
{
    if (slot_id < 1 || slot_id > SLOT_COUNT || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *out = &s_slot_map[slot_id - 1];
    return ESP_OK;
}
