#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "driver/gpio.h"

#define TAG "BLE_SERVER"

// BLE Service UUID
#define SERVICE_UUID   "da1a59b9-1125-4bd4-b72b-d15dc8057c53"
#define CHAR_TEMP_UUID "c10aa881-8c74-4146-bc26-4834ffb2ce5f"
#define CHAR_BATT_UUID "01234567-0123-4567-89ab-0123456789ef"
#define CHAR_LED_UUID  "28ecdcde-5541-4db2-982c-91bde176da5d"

// LED Pin
#define LED_PIN GPIO_NUM_10  

// Global variables
static float tempC = 25.0;
static float vBatt = 5.0;
static bool LedState = false;

// BLE Variables
static esp_gatt_char_prop_t char_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static uint16_t ble_handle_table[3];  // Stores handles for temp, battery, LED
static esp_gatt_if_t server_if;

// BLE Callback Functions
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            ESP_LOGI(TAG, "Registering service...");
            esp_ble_gatts_create_attr_tab(ble_handle_table, gatts_if, 3, 0);
            break;

        case ESP_GATTS_WRITE_EVT:
            if (param->write.handle == ble_handle_table[2]) {  // LED Control
                if (param->write.value[0] == '0') {
                    LedState = true;
                    gpio_set_level(LED_PIN, 0);
                    ESP_LOGI(TAG, "LED ON");
                } else if (param->write.value[0] == '1') {
                    LedState = false;
                    gpio_set_level(LED_PIN, 1);
                    ESP_LOGI(TAG, "LED OFF");
                }
            }
            break;

        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "Client Connected!");
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "Client Disconnected!");
            esp_ble_gap_start_advertising();
            break;

        default:
            break;
    }
}

// BLE Initialization
void ble_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    esp_ble_gatts_register_callback(gatts_profile_event_handler);
    esp_ble_gatts_app_register(0);
}

// Temperature & Battery Simulation
void sensor_task(void *pvParameter) {
    while (1) {
        tempC += (rand() % 2 == 0) ? 0.1 : -0.1;
        vBatt = (vBatt > 1.0) ? vBatt - 0.01 : 5.0;
        
        ESP_LOGI(TAG, "Temperature: %.2f°C, Battery: %.2fV", tempC, vBatt);
        vTaskDelay(pdMS_TO_TICKS(15000));  // Update every 15 sec
    }
}

void app_main() {
    ESP_LOGI(TAG, "Starting BLE Server...");

    // Initialize BLE
    ble_init();

    // Configure LED GPIO
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 1);  // LED off

    // Start Sensor Task
    xTaskCreate(&sensor_task, "sensor_task", 2048, NULL, 5, NULL);
}
