#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#ifdef CI_BUILD 
    #include "wifi_config_example.h"
    #include "http_config_example.h"
#else
    #include "wifi_config.h"
    #include "http_config.h"
#endif

#include "storage_nvs.h"
#include "wifi_heartbeat.h"

static const char *TAG = "ongi-wifi-heartbeat";
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static esp_err_t build_heartbeat_payload(char *json_payload, size_t json_payload_size);
static esp_err_t prepare_heartbeat_request(esp_http_client_handle_t client, const char *json_payload);
static void cleanup_wifi_event_group(void);

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    // Handle Wi-Fi and IP events
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // Connect to Wi-Fi when the station starts
        esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to connect to Wi-Fi: %s", esp_err_to_name(err));
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Log disconnection and attempt to reconnect
        ESP_LOGW(TAG, "Disconnected from WiFi, retrying...");
        esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to reconnect to Wi-Fi: %s", esp_err_to_name(err));
        }
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // Log the obtained IP address and set the connected bit
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// WiFi initialization
esp_err_t wifi_init() {
    // Create the event group to handle WiFi connection events
    wifi_event_group = xEventGroupCreate();
    // Check if event group was created successfully
    if (wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create Wi-Fi event group");
        return ESP_ERR_NO_MEM;
    }

    // Initialize the TCP/IP stack and network interface
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize network interface: %s", esp_err_to_name(err));
        cleanup_wifi_event_group();
        return err;
    }

    // Create the default event loop and Wi-Fi station network interface
    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create default event loop: %s", esp_err_to_name(err));
        cleanup_wifi_event_group();
        return err;
    }

    // Create default Wi-Fi station network interface and check for errors
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default Wi-Fi station network interface");
        cleanup_wifi_event_group();
        return ESP_ERR_NO_MEM;
    }

    // Initialize Wi-Fi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    // Initialize Wi-Fi and check for errors
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Wi-Fi: %s", esp_err_to_name(err));
        cleanup_wifi_event_group();
        return err;
    }

    // Register event handlers for Wi-Fi and IP events
    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register Wi-Fi event handler: %s", esp_err_to_name(err));
        cleanup_wifi_event_group();
        return err;
    }
    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(err));
        cleanup_wifi_event_group();
        return err;
    }

    // Configure Wi-Fi connection settings
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    // Set Wi-Fi mode to station and check for errors
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Wi-Fi mode: %s", esp_err_to_name(err));
        cleanup_wifi_event_group();
        return err;
    }

    // Set Wi-Fi configuration and check for errors
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Wi-Fi configuration: %s", esp_err_to_name(err));
        cleanup_wifi_event_group();
        return err;
    }

    // Start Wi-Fi and check for errors
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Wi-Fi: %s", esp_err_to_name(err));
        cleanup_wifi_event_group();
        return err;
    }

    ESP_LOGI(TAG, "WiFi initialization completed");
    return ESP_OK;
}

// HTTP event handler
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        ESP_LOGI(TAG, "Received response data");
    }
    return ESP_OK;
}

// Send heartbeat to server
static void send_heartbeat() {
    // Set up HTTP client configuration
    esp_http_client_config_t config = {
        .url = HEARTBEAT_URL,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .timeout_ms = 5000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    // Create HTTP client handle
    esp_http_client_handle_t client = esp_http_client_init(&config); // returns NULL if any errors
    // Check if client was created successfully
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return;
    }

    // Build the heartbeat JSON payload
        // NOTE: Buffer must be outlived until the HTTP request is performed; keep on caller's stack.
    char json_payload[256];
    // Check if payload was built successfully
    esp_err_t err = build_heartbeat_payload(json_payload, sizeof(json_payload));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to build heartbeat payload");
        esp_http_client_cleanup(client);
        return;
    }

    // Prepare the heartbeat request and set HTTP headers
    err = prepare_heartbeat_request(client, json_payload);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to prepare heartbeat request");
        esp_http_client_cleanup(client);
        return;
    }

    // Perform the HTTP request
    err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error performing HTTP request");
    } else {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP request completed with status code: %d", status_code);
    }

    // Clean up HTTP client resources
    esp_http_client_cleanup(client);
}   

// Heartbeat task
void heartbeat_task(void *pvParameters) {
    while (1) {
        // Wait for WiFi connection
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "WiFi connected, sending heartbeat...");

        send_heartbeat();
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_INTERVAL_MS));
    }
}

// Helper function to build Heartbeat Request JSON payload
static esp_err_t build_heartbeat_payload(char *json_payload, size_t json_payload_size) {
    // Get current RSSI value
    wifi_ap_record_t ap_info;
    int8_t rssi = 0;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        rssi = ap_info.rssi;
    } else {
        rssi = 0; // Default to 0 if unable to get RSSI
        ESP_LOGW(TAG, "Failed to get AP info");
    }

    // Uptime in seconds
    uint64_t uptime_sec = esp_timer_get_time() / 1000000;

    // Build JSON payload
    int written = snprintf(json_payload, json_payload_size,
       "{"
        "\"serialNumber\":\"%s\","
        "\"status\":\"ONLINE\","
        "\"uptimeSec\":%llu,"
        "\"rssi\":%d"
        "}",
        SERIAL_NUMBER, uptime_sec, rssi
    );

    // Check if JSON payload was built successfully
    if (written < 0 || (size_t)written >= json_payload_size) {
        ESP_LOGE(TAG, "Failed to build JSON payload");
        return ESP_FAIL;
    }

    return ESP_OK;
}

// Helper function to prepare the heartbeat HTTP request by setting headers and payload
static esp_err_t prepare_heartbeat_request(esp_http_client_handle_t client, const char *json_payload) {
    // Set Content-Type header to application/json 
    esp_err_t err = esp_http_client_set_header(client, "Content-Type", "application/json");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set HTTP header");
        return err;
    }

    // Set Device-Token header for authentication
    err = esp_http_client_set_header(client, "Device-Token", CONFIG_DEVICE_TOKEN);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Device-Token header");
        return err;
    }

    // Set the JSON payload for the POST request
    err = esp_http_client_set_post_field(client, json_payload, strlen(json_payload));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set POST field");
        return err;
    }

    return ESP_OK;
}

static void cleanup_wifi_event_group(void) {
    if (wifi_event_group != NULL) {
        vEventGroupDelete(wifi_event_group);
        wifi_event_group = NULL;
    }
}
