#include "ads1115.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "ads1115";

#define ADS1115_I2C_PORT    I2C_NUM_0
#define ADS1115_I2C_SPEED   100000

#define ADS1115_REG_CONVERSION  0x00
#define ADS1115_REG_CONFIG      0x01

// 128 SPS = 7.8 ms per conversion; 15 ms gives enough margin.
#define ADS1115_CONV_WAIT_MS    15

// Config high byte: OS=1, MUX=single-ended, PGA=+/-2.048 V, MODE=single-shot.
// Single-ended MUX values are ch0=100, ch1=101, ch2=110, ch3=111.
#define ADS1115_CONFIG_HI_BASE  0xC5
// Config low byte: DR=128 SPS, comparator disabled.
#define ADS1115_CONFIG_LO       0x83

static i2c_master_bus_handle_t s_bus = NULL;
// Index 0 maps to 0x48, index 1 maps to 0x49.
static i2c_master_dev_handle_t s_devs[2] = {NULL, NULL};

static void cleanup_bus(void)
{
    for (int i = 0; i < 2; i++) {
        if (s_devs[i] != NULL) {
            esp_err_t err = i2c_master_bus_rm_device(s_devs[i]);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "remove device %d failed: %s", i, esp_err_to_name(err));
                continue;
            }
            s_devs[i] = NULL;
        }
    }

    if (s_bus != NULL) {
        esp_err_t err = i2c_del_master_bus(s_bus);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "delete bus failed: %s", esp_err_to_name(err));
            return;
        }
        s_bus = NULL;
    }
}

static int addr_to_idx(uint8_t addr)
{
    if (addr == ADS1115_ADDR_LO) return 0;
    if (addr == ADS1115_ADDR_HI) return 1;
    return -1;
}

esp_err_t ads1115_bus_init(int sda_gpio, int scl_gpio)
{
    if (s_bus != NULL) {
        ESP_LOGW(TAG, "bus already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_master_bus_config_t bus_cfg = {
        .clk_source             = I2C_CLK_SRC_DEFAULT,
        .i2c_port               = ADS1115_I2C_PORT,
        .scl_io_num             = scl_gpio,
        .sda_io_num             = sda_gpio,
        .glitch_ignore_cnt      = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bus init failed: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t addrs[2] = {ADS1115_ADDR_LO, ADS1115_ADDR_HI};
    for (int i = 0; i < 2; i++) {
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address  = addrs[i],
            .scl_speed_hz    = ADS1115_I2C_SPEED,
        };
        err = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_devs[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "add device 0x%02x failed: %s", addrs[i], esp_err_to_name(err));
            cleanup_bus();
            return err;
        }
    }

    ESP_LOGI(TAG, "I2C bus ready: sda=%d scl=%d", sda_gpio, scl_gpio);
    return ESP_OK;
}

esp_err_t ads1115_detect(uint8_t addr, bool *found)
{
    if (found == NULL) return ESP_ERR_INVALID_ARG;
    if (addr_to_idx(addr) < 0) return ESP_ERR_INVALID_ARG;
    if (s_bus == NULL) return ESP_ERR_INVALID_STATE;

    esp_err_t err = i2c_master_probe(s_bus, addr, 50);
    *found = (err == ESP_OK);
    ESP_LOGI(TAG, "detect 0x%02x: %s", addr, *found ? "found" : "not found");
    return ESP_OK;
}

esp_err_t ads1115_read_channel(uint8_t addr, uint8_t channel, int16_t *out)
{
    if (out == NULL || channel >= ADS1115_CHANNEL_COUNT) return ESP_ERR_INVALID_ARG;
    int idx = addr_to_idx(addr);
    if (idx < 0) return ESP_ERR_INVALID_ARG;
    if (s_devs[idx] == NULL) return ESP_ERR_INVALID_STATE;

    uint8_t config_hi = (uint8_t)(ADS1115_CONFIG_HI_BASE | (channel << 4));
    uint8_t write_buf[3] = {ADS1115_REG_CONFIG, config_hi, ADS1115_CONFIG_LO};

    esp_err_t err = i2c_master_transmit(s_devs[idx], write_buf, sizeof(write_buf), 50);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "config write failed: addr=0x%02x ch=%d: %s", addr, channel, esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(ADS1115_CONV_WAIT_MS));

    uint8_t reg = ADS1115_REG_CONVERSION;
    uint8_t read_buf[2] = {0};
    err = i2c_master_transmit_receive(s_devs[idx], &reg, 1, read_buf, 2, 50);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "conversion read failed: addr=0x%02x ch=%d: %s", addr, channel, esp_err_to_name(err));
        return err;
    }

    *out = (int16_t)((read_buf[0] << 8) | read_buf[1]);
    return ESP_OK;
}
