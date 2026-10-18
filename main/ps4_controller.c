#include "ps4_controller.h"
#include "motor_driver.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_hidh.h"

static const char *TAG = "PS4_CONTROLLER";

#define STICK_DEADZONE       12
#define STICK_CENTER         128

static ps4_controller_state_t s_controller_state = {
    .left_stick_x = STICK_CENTER,
    .left_stick_y = STICK_CENTER,
    .right_stick_x = STICK_CENTER,
    .right_stick_y = STICK_CENTER,
    .is_connected = false
};

static uint32_t s_last_packet_time = 0;
static esp_bd_addr_t s_target_mac = {0x00, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A};

static int16_t map_stick_value(uint8_t raw_val, bool invert) {
    int diff = (int)raw_val - STICK_CENTER;
    if (invert) {
        diff = -diff;
    }

    if (diff > -STICK_DEADZONE && diff < STICK_DEADZONE) {
        return 0;
    }

    if (diff >= STICK_DEADZONE) {
        int scaled = (diff - STICK_DEADZONE) * 255 / (127 - STICK_DEADZONE);
        return (scaled > 255) ? 255 : scaled;
    } else {
        int scaled = (diff + STICK_DEADZONE) * 255 / (128 - STICK_DEADZONE);
        return (scaled < -255) ? -255 : scaled;
    }
}

static void update_motor_control(uint8_t ly, uint8_t rx) {
    int16_t forward = map_stick_value(ly, true);
    int16_t turn = map_stick_value(rx, false);

    int left_speed = forward + turn;
    int right_speed = forward - turn;

    if (left_speed > 255) left_speed = 255;
    if (left_speed < -255) left_speed = -255;
    if (right_speed > 255) right_speed = 255;
    if (right_speed < -255) right_speed = -255;

    motor_set_speed(MOTOR_LEFT, (int16_t)left_speed);
    motor_set_speed(MOTOR_RIGHT, (int16_t)right_speed);
}

static void parse_ps4_packet(const uint8_t *data, uint16_t len) {
    if (data == NULL || len < 5) return;

    uint8_t offset = 0;
    if (data[0] == 0xa1) {
        offset = 1;
    }

    if (data[offset] == 0x01) {
        s_controller_state.left_stick_x  = data[offset + 1];
        s_controller_state.left_stick_y  = data[offset + 2];
        s_controller_state.right_stick_x = data[offset + 3];
        s_controller_state.right_stick_y = data[offset + 4];

        s_last_packet_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        s_controller_state.is_connected = true;

        update_motor_control(s_controller_state.left_stick_y, s_controller_state.right_stick_x);
    }
}

static void hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data) {
    esp_hidh_event_t event = (esp_hidh_event_t)id;
    esp_hidh_event_data_t *param = (esp_hidh_event_data_t *)event_data;

    switch (event) {
        case ESP_HIDH_OPEN_EVENT:
            ESP_LOGI(TAG, "PS4 Controller Connected (HID Open)");
            s_controller_state.is_connected = true;
            s_last_packet_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            break;

        case ESP_HIDH_INPUT_EVENT:
            if (param != NULL && param->input.data != NULL && param->input.length > 0) {
                parse_ps4_packet(param->input.data, param->input.length);
            }
            break;

        case ESP_HIDH_CLOSE_EVENT:
            ESP_LOGW(TAG, "PS4 Controller Disconnected (HID Close)");
            s_controller_state.is_connected = false;
            motor_stop_all();
            break;

        default:
            break;
    }
}

static void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
        case ESP_BT_GAP_AUTH_CMPL_EVT:
            if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "Authentication success: %s", param->auth_cmpl.device_name);
            } else {
                ESP_LOGE(TAG, "Authentication failed, status:%d", param->auth_cmpl.stat);
            }
            break;
        case ESP_BT_GAP_CFM_REQ_EVT:
            esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
            break;
        default:
            break;
    }
}

static void failsafe_task(void *pvParameters) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));

        if (s_controller_state.is_connected) {
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (now - s_last_packet_time > 500) {
                ESP_LOGW(TAG, "Controller packet timeout! Stopping motors.");
                s_controller_state.is_connected = false;
                motor_stop_all();
            }
        }
    }
}

static void parse_mac_address(const char *mac_str, esp_bd_addr_t mac_addr) {
    unsigned int mac[6] = {0};
    if (sscanf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6 ||
        sscanf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6) {
        for (int i = 0; i < 6; i++) {
            mac_addr[i] = (uint8_t)mac[i];
        }
    } else if (sscanf(mac_str, "%02x:%02x:%02x:%02x",
                      &mac[2], &mac[3], &mac[4], &mac[5]) == 4 ||
               sscanf(mac_str, "%02X:%02X:%02X:%02X",
                      &mac[2], &mac[3], &mac[4], &mac[5]) == 4) {
        mac[0] = 0x00; mac[1] = 0x0A;
        for (int i = 0; i < 6; i++) {
            mac_addr[i] = (uint8_t)mac[i];
        }
    }
}

esp_err_t ps4_controller_init(const char *mac_str) {
    if (mac_str != NULL) {
        parse_mac_address(mac_str, s_target_mac);
    }

    ESP_LOGI(TAG, "Initializing PS4 Controller BT (Target MAC: %02X:%02X:%02X:%02X:%02X:%02X)",
             s_target_mac[0], s_target_mac[1], s_target_mac[2],
             s_target_mac[3], s_target_mac[4], s_target_mac[5]);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bluedroid_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bluedroid_enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_bt_gap_register_callback(gap_callback);

    esp_hidh_config_t config = {
        .callback = hidh_callback,
        .event_stack_size = 4096,
        .callback_arg = NULL,
    };
    ret = esp_hidh_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_hidh_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_bt_gap_set_device_name("PS4_Robot_Controller");
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    xTaskCreate(failsafe_task, "failsafe_task", 2048, NULL, 5, NULL);

    return ESP_OK;
}

void ps4_controller_get_state(ps4_controller_state_t *state) {
    if (state != NULL) {
        *state = s_controller_state;
    }
}

bool ps4_controller_is_connected(void) {
    return s_controller_state.is_connected;
}
