#include "schedule_fetcher.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"

#if defined(CI_BUILD) || defined(TEST_BUILD)
#include "http_config_example.h"
#include "wifi_config_example.h"
#else
#include "http_config.h"
#include "wifi_config.h"
#endif

static const char *TAG = "schedule_fetcher";

#define SCHEDULE_FETCH_URL BASE_URL "/api/device/schedules"
#define SCHEDULE_FETCH_TIMEOUT_MS 5000
#define SCHEDULE_RESPONSE_BUFFER_SIZE 1024

typedef struct {
    char *buffer;
    size_t capacity;
    size_t length;
    bool overflowed;
} ScheduleResponseBuffer;

static bool is_retryable_http_status(int status_code)
{
    return status_code == 408 || status_code == 429 || status_code >= 500;
}

static esp_err_t append_response_data(ScheduleResponseBuffer *response, const char *data, int data_len)
{
    if (response == NULL || data == NULL || data_len < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (data_len == 0) {
        return ESP_OK;
    }

    size_t incoming_len = (size_t)data_len;
    if (incoming_len >= response->capacity - response->length) {
        response->overflowed = true;
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(response->buffer + response->length, data, incoming_len);
    response->length += incoming_len;
    response->buffer[response->length] = '\0';
    return ESP_OK;
}

static esp_err_t schedule_http_event_handler(esp_http_client_event_t *evt)
{
    if (evt == NULL || evt->user_data == NULL) {
        return ESP_OK;
    }

    if (evt->event_id != HTTP_EVENT_ON_DATA) {
        return ESP_OK;
    }

    ScheduleResponseBuffer *response = (ScheduleResponseBuffer *)evt->user_data;
    esp_err_t err = append_response_data(response, evt->data, evt->data_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Schedule response buffer append failed: %s", esp_err_to_name(err));
    }

    return err;
}

static esp_err_t parse_scheduled_time(const char *scheduled_time, uint8_t *hour, uint8_t *minute)
{
    if (scheduled_time == NULL || hour == NULL || minute == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int parsed_hour = -1;
    int parsed_minute = -1;
    int parsed_second = -1;
    char tail = '\0';
    int parsed = sscanf(scheduled_time, "%d:%d:%d%c", &parsed_hour, &parsed_minute, &parsed_second, &tail);
    if (parsed != 3 ||
        parsed_hour < 0 || parsed_hour > 23 ||
        parsed_minute < 0 || parsed_minute > 59 ||
        parsed_second < 0 || parsed_second > 59) {
        return ESP_ERR_INVALID_ARG;
    }

    *hour = (uint8_t)parsed_hour;
    *minute = (uint8_t)parsed_minute;
    return ESP_OK;
}

static const char *find_token_in_range(const char *begin, const char *end, const char *token)
{
    if (begin == NULL || end == NULL || token == NULL || begin > end) {
        return NULL;
    }

    size_t token_len = strlen(token);
    for (const char *cursor = begin; cursor + token_len <= end; cursor++) {
        if (memcmp(cursor, token, token_len) == 0) {
            return cursor;
        }
    }

    return NULL;
}

static const char *skip_json_space(const char *cursor, const char *end)
{
    while (cursor < end && (*cursor == ' ' || *cursor == '\n' || *cursor == '\r' || *cursor == '\t')) {
        cursor++;
    }

    return cursor;
}

static esp_err_t parse_json_int_field(const char *begin, const char *end, const char *key, int *out_value)
{
    if (begin == NULL || end == NULL || key == NULL || out_value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const char *key_pos = find_token_in_range(begin, end, key);
    if (key_pos == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    const char *colon = find_token_in_range(key_pos + strlen(key), end, ":");
    if (colon == NULL) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    const char *cursor = skip_json_space(colon + 1, end);
    int value = 0;
    bool has_digit = false;
    while (cursor < end && *cursor >= '0' && *cursor <= '9') {
        has_digit = true;
        value = (value * 10) + (*cursor - '0');
        cursor++;
    }

    if (!has_digit) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    *out_value = value;
    return ESP_OK;
}

static esp_err_t parse_json_string_field(
    const char *begin,
    const char *end,
    const char *key,
    char *out_value,
    size_t out_value_size)
{
    if (begin == NULL || end == NULL || key == NULL || out_value == NULL || out_value_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const char *key_pos = find_token_in_range(begin, end, key);
    if (key_pos == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    const char *colon = find_token_in_range(key_pos + strlen(key), end, ":");
    if (colon == NULL) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    const char *cursor = skip_json_space(colon + 1, end);
    if (cursor >= end || *cursor != '"') {
        return ESP_ERR_INVALID_RESPONSE;
    }
    cursor++;

    const char *value_begin = cursor;
    while (cursor < end && *cursor != '"') {
        cursor++;
    }
    if (cursor >= end) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    size_t value_len = (size_t)(cursor - value_begin);
    if (value_len >= out_value_size) {
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(out_value, value_begin, value_len);
    out_value[value_len] = '\0';
    return ESP_OK;
}

static esp_err_t parse_schedule_response(const char *json, SlotEntry *slots, size_t max_count, size_t *out_count)
{
    if (json == NULL || slots == NULL || out_count == NULL || max_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const char *json_end = json + strlen(json);
    const char *data_key = find_token_in_range(json, json_end, "\"data\"");
    if (data_key == NULL) {
        ESP_LOGE(TAG, "Schedule response missing data array");
        return ESP_ERR_INVALID_RESPONSE;
    }

    const char *array_cursor = find_token_in_range(data_key, json_end, "[");
    if (array_cursor == NULL) {
        ESP_LOGE(TAG, "Schedule response data is not an array");
        return ESP_ERR_INVALID_RESPONSE;
    }
    array_cursor++;

    size_t count = 0;
    while (true) {
        array_cursor = skip_json_space(array_cursor, json_end);
        if (array_cursor >= json_end) {
            return ESP_ERR_INVALID_RESPONSE;
        }
        if (*array_cursor == ']') {
            break;
        }
        if (*array_cursor == ',') {
            array_cursor++;
            continue;
        }
        if (*array_cursor != '{') {
            ESP_LOGE(TAG, "Schedule response item is not an object");
            return ESP_ERR_INVALID_RESPONSE;
        }

        const char *item_begin = array_cursor;
        const char *item_end = find_token_in_range(item_begin, json_end, "}");
        if (item_end == NULL) {
            return ESP_ERR_INVALID_RESPONSE;
        }

        if (count >= max_count) {
            ESP_LOGE(TAG, "Schedule response has too many slots");
            return ESP_ERR_INVALID_SIZE;
        }

        int slot_number = 0;
        char scheduled_time[9] = { 0 };
        esp_err_t err = parse_json_int_field(item_begin, item_end, "\"slotNumber\"", &slot_number);
        if (err == ESP_OK) {
            err = parse_json_string_field(
                item_begin,
                item_end,
                "\"scheduledTime\"",
                scheduled_time,
                sizeof(scheduled_time)
            );
        }
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Schedule response item has invalid fields");
            return ESP_ERR_INVALID_RESPONSE;
        }

        if (slot_number <= 0 || slot_number > SCHEDULE_SLOT_COUNT) {
            ESP_LOGE(TAG, "Schedule response slot is out of range: slot=%d", slot_number);
            return ESP_ERR_INVALID_ARG;
        }

        uint8_t hour = 0;
        uint8_t minute = 0;
        err = parse_scheduled_time(scheduled_time, &hour, &minute);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Schedule response time is invalid: slot=%d", slot_number);
            return err;
        }

        // Convert the server schedule item into the local slot entry contract.
        slots[count] = (SlotEntry) {
            .slot_id = (uint8_t)slot_number,
            .hour = hour,
            .minute = minute,
            .triggered = false,
        };
        count++;
        array_cursor = item_end + 1;
    }

    *out_count = count;
    return ESP_OK;
}

static esp_err_t prepare_schedule_fetch_request(esp_http_client_handle_t client)
{
    if (client == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = esp_http_client_set_header(client, "Device-Token", CONFIG_DEVICE_TOKEN);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Device-Token header: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t schedule_fetcher_fetch(SlotEntry *slots, size_t max_count, size_t *out_count)
{
    if (slots == NULL || out_count == NULL || max_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    *out_count = 0;
    char response_storage[SCHEDULE_RESPONSE_BUFFER_SIZE] = { 0 };
    ScheduleResponseBuffer response = {
        .buffer = response_storage,
        .capacity = sizeof(response_storage),
        .length = 0,
        .overflowed = false,
    };

    esp_http_client_config_t config = {
        .url = SCHEDULE_FETCH_URL,
        .method = HTTP_METHOD_GET,
        .event_handler = schedule_http_event_handler,
        .user_data = &response,
        .timeout_ms = SCHEDULE_FETCH_TIMEOUT_MS,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize schedule fetch HTTP client");
        return ESP_FAIL;
    }

    esp_err_t err = prepare_schedule_fetch_request(client);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        return err;
    }

    ESP_LOGI(TAG, "Fetching schedule snapshot");
    err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Schedule fetch failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    if (response.overflowed) {
        ESP_LOGE(TAG, "Schedule response exceeded buffer");
        esp_http_client_cleanup(client);
        return ESP_ERR_INVALID_SIZE;
    }

    int status_code = esp_http_client_get_status_code(client);
    if (status_code < 200 || status_code >= 300) {
        ESP_LOGE(TAG, "Schedule fetch HTTP error: http_status=%d", status_code);
        esp_http_client_cleanup(client);
        return is_retryable_http_status(status_code) ? ESP_ERR_TIMEOUT : ESP_ERR_INVALID_RESPONSE;
    }

    err = parse_schedule_response(response.buffer, slots, max_count, out_count);
    esp_http_client_cleanup(client);
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "Schedule fetch parsed: count=%u", (unsigned int)*out_count);
    return ESP_OK;
}
