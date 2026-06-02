#include "servo_driver_factory.h"

#if defined(CI_BUILD) || defined(TEST_BUILD)
#include "dummy_servo_driver.h"

// Get the default servo driver instance, which is a dummy driver in CI/test builds.
const ServoDriver *get_default_servo_driver(void) {
    return &DUMMY_SERVO_DRIVER;
}

#else
#include "ledc_servo_driver.h"
#include "esp_log.h"

static const char *TAG = "servo_factory";

// Get the default servo driver instance, which is initialized on first call
const ServoDriver *get_default_servo_driver(void) {
    static bool s_initialized = false;
    if (!s_initialized) {
        esp_err_t err = ledc_servo_driver_init();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ledc_servo_driver_init failed: %s", esp_err_to_name(err));
            return NULL;
        }
        s_initialized = true;
    }
    return &LEDC_SERVO_DRIVER;
}

#endif
