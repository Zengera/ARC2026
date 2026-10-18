#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "motor_driver.h"
#include "ps4_controller.h"

static const char *TAG = "MAIN";

// 設定用BT MACアドレス（仮設定: 0A:0A:0A:0A -> 00:0A:0A:0A:0A:0A として解釈）
#define BT_MAC_ADDRESS "00:0A:0A:0A:0A:0A"

void app_main(void) {
    ESP_LOGI(TAG, "=== ESP32 PS4 Robot Control Starting ===");

    // NVSの初期化 (Bluetoothスタックに必須)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Hブリッジモータドライバの初期化
    motor_driver_init(NULL);

    // PS4コントローラ通信の初期化
    ESP_LOGI(TAG, "Target PS4 Controller MAC: %s", BT_MAC_ADDRESS);
    ret = ps4_controller_init(BT_MAC_ADDRESS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PS4 controller, error: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Robot control loop ready. Waiting for PS4 Controller connection...");

    ps4_controller_state_t state;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        if (ps4_controller_is_connected()) {
            ps4_controller_get_state(&state);
            ESP_LOGI(TAG, "[Status: Connected] LX:%d, LY:%d, RX:%d, RY:%d",
                     state.left_stick_x, state.left_stick_y,
                     state.right_stick_x, state.right_stick_y);
        } else {
            ESP_LOGI(TAG, "[Status: Waiting for Connection...]");
        }
    }
}
