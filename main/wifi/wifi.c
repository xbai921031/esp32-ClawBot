/*
 * Author: xbai
 * Date  : 2026/03/12
 */

#include "wifi.h"
#include "esp_rom_sys.h"
#include "clawbot_config.h"

void wifi_init(void)
{
    wifi_ap_record_t ap_info;
    // 1. NVS initialization
    nvs_flash_init();

    // 2. TCP/IP initialization
    esp_netif_init();

    // 3. Event loop
    esp_event_loop_create_default();

    // 4. Create WiFi STA
    esp_netif_create_default_wifi_sta();

    // 5. WiFi config initialization
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // 6. WiFi mode
    esp_wifi_set_mode(WIFI_MODE_STA);

    // 7. WiFi config
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

    // 8. Start WiFi
    esp_wifi_start();

    // 9. Connection
    esp_wifi_connect();

    if(esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
    {
        esp_rom_printf("Connected to: %s\n", ap_info.ssid);
    }
    else
    {
        esp_rom_printf("WiFi not connected\n");
    }
}