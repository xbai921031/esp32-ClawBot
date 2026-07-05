/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * ESP-Claw with official http_server + WeChat QR login
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_config.h"
#include "app_claw.h"
#include "app_fs.h"
#include "http_server.h"
#include "claw_paths.h"

static const char *TAG = "app";

static app_claw_config_t *s_claw_config;

static esp_err_t app_allocate_runtime_state(void)
{
    if (!s_claw_config) s_claw_config = calloc(1, sizeof(*s_claw_config));
    ESP_RETURN_ON_FALSE(s_claw_config, ESP_ERR_NO_MEM, TAG, "alloc failed");
    return ESP_OK;
}

static void app_free_runtime_state(void) { free(s_claw_config); s_claw_config = NULL; }

static esp_err_t load_app_config(app_claw_config_t *config)
{
    app_config_t app_cfg = {0};
    esp_err_t ret = app_config_load(&app_cfg);
    if (ret != ESP_OK) return ESP_OK;
    app_config_to_claw(&app_cfg, config);
    return ESP_OK;
}

static void on_got_ip(void *arg, esp_event_base_t b, int32_t id, void *data)
{
    char ip[16] = {0};
    esp_ip4addr_ntoa(&((ip_event_got_ip_t *)data)->ip_info.ip, ip, sizeof(ip));
    ESP_LOGI(TAG, "[WiFi] Connected OK! IP: %s", ip);
    ESP_LOGI(TAG, "[WiFi] Open http://%s in browser", ip);
    (void)arg; (void)b; (void)id;
}

static void on_disconnected(void *arg, esp_event_base_t b, int32_t id, void *data)
{
    wifi_event_sta_disconnected_t *evt = (wifi_event_sta_disconnected_t *)data;
    ESP_LOGW(TAG, "[WiFi] Disconnected! reason=%d, retrying...", evt ? evt->reason : -1);
    esp_wifi_connect();
    (void)arg; (void)b; (void)id;
}

static void init_wifi(void)
{
    esp_netif_create_default_wifi_sta();
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_disconnected, NULL);

    /* Check saved credentials */
    nvs_handle_t h;
    char ssid[64]={0}, pass[128]={0}; size_t len;
    if (nvs_open("app", NVS_READONLY, &h) == ESP_OK) {
        len=sizeof(ssid); nvs_get_str(h, "wifi_ssid", ssid, &len);
        len=sizeof(pass); nvs_get_str(h, "wifi_password", pass, &len);
        nvs_close(h);
    }

    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));

    if (ssid[0]) {
        wifi_config_t sta_cfg = { .sta = { .threshold.authmode = WIFI_AUTH_WPA2_PSK } };
        strlcpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid));
        strlcpy((char *)sta_cfg.sta.password, pass, sizeof(sta_cfg.sta.password));

        wifi_config_t ap_cfg = {
            .ap = { .ssid = "ESP-ClawBot", .ssid_len = 11,
                    .channel = 0, .max_connection = 4, .authmode = WIFI_AUTH_OPEN },
        };

        esp_netif_create_default_wifi_ap();
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
        ESP_ERROR_CHECK(esp_wifi_start());
        esp_wifi_connect();
        ESP_LOGI(TAG, "Connecting to %s (AP+STA mode)...", ssid);
    } else {
        /* No WiFi saved — start SoftAP only */
        esp_netif_create_default_wifi_ap();
        wifi_config_t ap_cfg = {
            .ap = { .ssid = "ESP-ClawBot", .ssid_len = 11,
                    .channel = 6, .max_connection = 4, .authmode = WIFI_AUTH_OPEN },
        };
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_LOGI(TAG, "No WiFi credentials — SoftAP: ESP-ClawBot, http://192.168.4.1");
    }
}

static esp_err_t main_save_claw_config(const app_claw_config_t *config, void *user_ctx)
{
    (void)user_ctx;
    app_config_t app_cfg;
    app_config_load_defaults(&app_cfg);
    app_config_to_claw(&app_cfg, (app_claw_config_t *)config);
    return app_config_save(&app_cfg);
}

#include "http_server_adapter.h"

void app_main(void)
{
    ESP_LOGI(TAG, "ESP-Claw Starting...");

    /* NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Init config storage */
    app_config_init();

    esp_netif_init();
    esp_event_loop_create_default();

    /* WiFi (STA + SoftAP depending on credentials) */
    init_wifi();

    /* Wait for WiFi to connect if credentials exist */
    vTaskDelay(pdMS_TO_TICKS(6000));

    /* Init filesystem (non-fatal) */
    if (app_fs_init() != ESP_OK) {
        ESP_LOGW(TAG, "Filesystem init failed, continuing");
    }
    claw_paths_set(CLAW_PATH_DATA,   app_fs_storage_base_path());
    claw_paths_set(CLAW_PATH_SYSTEM, app_fs_system_base_path());

    /* Start official http_server — serves frontend + REST APIs */
    http_server_config_t http_cfg = {
        .storage_base_path = app_fs_storage_base_path(),
        .services = http_server_adapter_get_services(),
    };
    ESP_ERROR_CHECK(http_server_init(&http_cfg));
    ESP_LOGI(TAG, "Starting HTTP server...");
    esp_err_t http_err = http_server_start();
    if (http_err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP server start failed: %s", esp_err_to_name(http_err));
    }

    /* Agent config */
    ESP_ERROR_CHECK(app_allocate_runtime_state());
    load_app_config(s_claw_config);

    if (s_claw_config && s_claw_config->llm_base_url[0] == '\0') {
        strlcpy(s_claw_config->llm_backend_type, "openai", sizeof(s_claw_config->llm_backend_type));
        strlcpy(s_claw_config->llm_base_url,  "https://api.openai.com/v1", sizeof(s_claw_config->llm_base_url));
        strlcpy(s_claw_config->llm_model,     "gpt-4o-mini",              sizeof(s_claw_config->llm_model));
    }
    if (s_claw_config) {
        strlcpy(s_claw_config->enabled_cap_groups, "cap_im_wechat", sizeof(s_claw_config->enabled_cap_groups));
    }

    ESP_LOGI(TAG, "Starting agent...");
    app_claw_set_save_config_callback(main_save_claw_config, NULL);
    esp_err_t agent_err = app_claw_start(s_claw_config);
    if (agent_err != ESP_OK) {
        ESP_LOGE(TAG, "Agent start failed: %s", esp_err_to_name(agent_err));
    }

    app_free_runtime_state();
    ESP_LOGI(TAG, "ESP-Claw started");
}
