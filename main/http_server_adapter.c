/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * Service callbacks for the official http_server component
 */
#include "http_server.h"
#include "app_config.h"
#include "cap_im_wechat.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "adapter";

static esp_err_t adapter_load_config(app_config_t *config)
{
    return app_config_load(config);
}

static esp_err_t adapter_save_config(const app_config_t *config)
{
    return app_config_save(config);
}

static char s_ip_str[16], s_ap_ip_str[16], s_ap_ssid_str[33];

static esp_err_t adapter_get_wifi_status(http_server_wifi_status_t *status)
{
    if (!status) return ESP_ERR_INVALID_ARG;
    memset(status, 0, sizeof(*status));
    esp_netif_t *n = esp_netif_get_handle_from_ifkey("STA_DEF");
    if (n) {
        esp_netif_ip_info_t ip;
        if (esp_netif_get_ip_info(n, &ip) == ESP_OK && ip.ip.addr != 0) {
            status->wifi_connected = true;
            esp_ip4addr_ntoa(&ip.ip, s_ip_str, sizeof(s_ip_str));
            status->ip = s_ip_str;
        }
    }
    esp_netif_t *ap = esp_netif_get_handle_from_ifkey("AP_DEF");
    if (ap) {
        esp_netif_ip_info_t ip;
        if (esp_netif_get_ip_info(ap, &ip) == ESP_OK) {
            status->ap_active = true;
            esp_ip4addr_ntoa(&ip.ip, s_ap_ip_str, sizeof(s_ap_ip_str));
            status->ap_ip = s_ap_ip_str;
            wifi_config_t cfg;
            memset(&cfg, 0, sizeof(cfg));
            if (esp_wifi_get_config(WIFI_IF_AP, &cfg) == ESP_OK) {
                memcpy(s_ap_ssid_str, cfg.ap.ssid, sizeof(s_ap_ssid_str));
                s_ap_ssid_str[32] = 0;
                status->ap_ssid = s_ap_ssid_str;
            }
        }
    }
    status->wifi_mode = "sta+ap";
    return ESP_OK;
}

static void restart_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(1000));  /* 1s delay so HTTP response can be sent */
    esp_restart();
    vTaskDelete(NULL);
}

static esp_err_t adapter_restart_device(void)
{
    xTaskCreate(restart_task, "restart", 2048, NULL, 1, NULL);
    return ESP_OK;
}

static esp_err_t adapter_wechat_login_start(const char *account_id, bool force)
{
    esp_err_t err = cap_im_wechat_qr_login_start(account_id ? account_id : "default", force);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "[WeChat] QR login started (account=%s)", account_id ? account_id : "default");
    } else {
        ESP_LOGE(TAG, "[WeChat] QR login start failed: %s", esp_err_to_name(err));
    }
    return err;
}

static esp_err_t adapter_wechat_login_get_status(http_server_wechat_login_status_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    cap_im_wechat_qr_login_status_t raw = {0};
    esp_err_t err = cap_im_wechat_qr_login_get_status(&raw);
    if (err != ESP_OK) return err;

    memset(out, 0, sizeof(*out));
    out->active     = raw.active;
    out->configured = raw.configured;
    out->completed  = raw.completed;
    out->persisted  = raw.persisted;
    strlcpy(out->session_key, raw.session_key, sizeof(out->session_key));
    strlcpy(out->status,      raw.status,      sizeof(out->status));
    strlcpy(out->message,     raw.message,     sizeof(out->message));
    strlcpy(out->qr_data_url, raw.qr_data_url, sizeof(out->qr_data_url));
    strlcpy(out->account_id,  raw.account_id,  sizeof(out->account_id));
    strlcpy(out->user_id,     raw.user_id,     sizeof(out->user_id));
    strlcpy(out->token,       raw.token,       sizeof(out->token));
    strlcpy(out->base_url,    raw.base_url,    sizeof(out->base_url));

    if (raw.completed && raw.token[0]) {
        ESP_LOGI(TAG, "[WeChat] Login completed! account=%s user=%s token=%.8s...",
                 raw.account_id, raw.user_id, raw.token);
    }
    return ESP_OK;
}

static esp_err_t adapter_wechat_login_cancel(void)
{
    ESP_LOGW(TAG, "[WeChat] QR login cancelled by user");
    return cap_im_wechat_qr_login_cancel();
}

static esp_err_t adapter_wechat_login_mark_persisted(void)
{
    return cap_im_wechat_qr_login_mark_persisted();
}

void http_server_adapter_init(void)
{
    /* All other services will be initialized by http_server_init */
}

http_server_services_t http_server_adapter_get_services(void)
{
    http_server_services_t svc = {0};
    svc.load_config              = adapter_load_config;
    svc.save_config              = adapter_save_config;
    svc.get_wifi_status          = adapter_get_wifi_status;
    svc.restart_device           = adapter_restart_device;
    svc.wechat_login_start       = adapter_wechat_login_start;
    svc.wechat_login_get_status  = adapter_wechat_login_get_status;
    svc.wechat_login_cancel      = adapter_wechat_login_cancel;
    svc.wechat_login_mark_persisted = adapter_wechat_login_mark_persisted;
    return svc;
}
