#include "ledc_servo_driver.h"
#include "slot_map.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "ledc_servo";

#define SERVO_TIMER           LEDC_TIMER_0
#define SERVO_MODE            LEDC_LOW_SPEED_MODE
#define SERVO_FREQ_HZ         50
#define SERVO_RESOLUTION      LEDC_TIMER_14_BIT
#define SERVO_PERIOD_US       20000
#define SERVO_MIN_PULSE_US    500
#define SERVO_MAX_PULSE_US    2500

#define SERVO_OPEN_ANGLE_DEG  90
#define SERVO_CLOSE_ANGLE_DEG 0

static ledc_channel_t s_channels[SLOT_COUNT];

// Convert angle in degree to duty cycle value for LEDC
static uint32_t angle_to_duty(uint8_t angle_deg)
{
    uint32_t pulse_us = SERVO_MIN_PULSE_US +
        (uint32_t)angle_deg * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) / 180;
    return (pulse_us * (1u << SERVO_RESOLUTION)) / SERVO_PERIOD_US;
}

esp_err_t ledc_servo_driver_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = SERVO_MODE,
        .timer_num       = SERVO_TIMER,
        .duty_resolution = SERVO_RESOLUTION,
        .freq_hz         = SERVO_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer init failed: %s", esp_err_to_name(err));
        return err;
    }

    uint32_t close_duty = angle_to_duty(SERVO_CLOSE_ANGLE_DEG);

    // Initialize LEDC channels for all slots
    for (uint8_t slot_id = 1; slot_id <= SLOT_COUNT; slot_id++) {
        const SlotHwConfig *cfg = NULL;
        err = slot_map_get(slot_id, &cfg);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "slot_map_get failed: slot=%d", slot_id);
            return err;
        }

        ledc_channel_t ch = (ledc_channel_t)(slot_id - 1);
        s_channels[slot_id - 1] = ch;

        ledc_channel_config_t ch_cfg = {
            .gpio_num   = cfg->servo_gpio,
            .speed_mode = SERVO_MODE,
            .channel    = ch,
            .intr_type  = LEDC_INTR_DISABLE,
            .timer_sel  = SERVO_TIMER,
            .duty       = close_duty,
            .hpoint     = 0,
        };
        err = ledc_channel_config(&ch_cfg);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "LEDC channel init failed: slot=%d gpio=%d", slot_id, cfg->servo_gpio);
            return err;
        }
        ESP_LOGI(TAG, "servo init: slot=%d gpio=%d ch=%d", slot_id, cfg->servo_gpio, ch);
    }

    ESP_LOGI(TAG, "all %d servo channels initialized", SLOT_COUNT);
    return ESP_OK;
}

static esp_err_t ledc_open(uint8_t slot_id)
{
    if (slot_id < 1 || slot_id > SLOT_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    ledc_channel_t ch = s_channels[slot_id - 1];
    uint32_t duty = angle_to_duty(SERVO_OPEN_ANGLE_DEG);
    esp_err_t err = ledc_set_duty(SERVO_MODE, ch, duty);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "servo open set_duty failed: slot=%d", slot_id);
        return err;
    }
    err = ledc_update_duty(SERVO_MODE, ch);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "servo open update_duty failed: slot=%d", slot_id);
        return err;
    }
    ESP_LOGI(TAG, "servo open: slot=%d ch=%d duty=%u", slot_id, ch, (unsigned)duty);
    return ESP_OK;
}

static esp_err_t ledc_close(uint8_t slot_id)
{
    if (slot_id < 1 || slot_id > SLOT_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    ledc_channel_t ch = s_channels[slot_id - 1];
    uint32_t duty = angle_to_duty(SERVO_CLOSE_ANGLE_DEG);
    esp_err_t err = ledc_set_duty(SERVO_MODE, ch, duty);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "servo close set_duty failed: slot=%d", slot_id);
        return err;
    }
    err = ledc_update_duty(SERVO_MODE, ch);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "servo close update_duty failed: slot=%d", slot_id);
        return err;
    }
    ESP_LOGI(TAG, "servo close: slot=%d ch=%d duty=%u", slot_id, ch, (unsigned)duty);
    return ESP_OK;
}

const ServoDriver LEDC_SERVO_DRIVER = {
    .open  = ledc_open,
    .close = ledc_close,
};
