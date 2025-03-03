#include <stdio.h>
#include "driver/adc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_websocket_client.h"
#include "esp_tls.h"

#define GPIO_4_ADC_CHANNEL ADC1_CHANNEL_0 // GPIO 4 is ADC1 channel 0
#define ADC_WIDTH ADC_WIDTH_BIT_12        // 12-bit resolution
#define ADC_ATTEN ADC_ATTEN_DB_12         // 11 dB attenuation (full range: 0V to 3.3V)
static const char *TAG = "GPIO4_ADC";

#define WIFI_SSID "Ramtin"
#define WIFI_PASS "4120728013"
#define WEBSOCKET_URI "ws://192.168.1.7:8080"

// WebSocket client handle
static esp_websocket_client_handle_t client;

// Tag for logging
static const char *TAGG = "wifi_station";

// Event handler for WebSocket events
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WebSocket Connected");
        // Send a message after connection
        if (esp_websocket_client_send_text(client, "Hello from ESP32", strlen("Hello from ESP32"), portMAX_DELAY) == -1)
        {
            printf("couldn't send the message\n");
        }
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WebSocket Disconnected");
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "WebSocket DATA: %.*s", data->data_len, (char *)data->data_ptr);
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WebSocket ERROR");
        break;
    }
}

// Function to initialize WebSocket client
void websocket_app_start(void)
{
    esp_websocket_client_config_t websocket_cfg = {
        .uri = WEBSOCKET_URI,
        .disable_auto_reconnect = false,
        .cert_pem = NULL, // Set to NULL to skip certificate validation
    };

    // Initialize the WebSocket client
    client = esp_websocket_client_init(&websocket_cfg);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize WebSocket client");
        return;
    }

    // Register event handlers
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);

    // Start the WebSocket client
    esp_err_t ret = esp_websocket_client_start(client);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start WebSocket client: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "WebSocket client started");
}

// Event handler for Wi-Fi events
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
        ESP_LOGI(TAGG, "Retry connecting to the AP");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAGG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        // Initialize WebSocket client
        websocket_app_start();
    }
}

// Function to initialize Wi-Fi
void wifi_init_sta(void)
{
    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create the default Wi-Fi station interface
    esp_netif_create_default_wifi_sta();

    // Initialize Wi-Fi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));

    // Configure Wi-Fi station mode
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

void app_main(void)
{
    // Initialize ADC
    adc1_config_width(ADC_WIDTH);                             // Set ADC resolution to 12 bits
    adc1_config_channel_atten(GPIO_4_ADC_CHANNEL, ADC_ATTEN); // Set attenuation

    // Initialize NVS (Non-Volatile Storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Wi-Fi
    wifi_init_sta();
    int64_t time = 1;
    char send_data[1024];
    // struct timeval tv_now;
    while (1)
    {
        time++;
        // gettimeofday(&tv_now, NULL);
        // int64_t time = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;

        // Read the raw ADC value (0 to 4095 for 12-bit resolution)
        int raw_value = adc1_get_raw(GPIO_4_ADC_CHANNEL);

        // Convert the raw value to voltage (in mV)
        int voltage_mV = raw_value * 3300 / 4095; // 3300 mV = 3.3V

        sprintf(send_data, "{\"time\":\"%lld\", \"raw_value\":\"%d\", \"voltage_mV\":\"%d\"}", time, raw_value, voltage_mV);

        esp_websocket_client_send_text(client, send_data, strlen(send_data), portMAX_DELAY);

        // vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}