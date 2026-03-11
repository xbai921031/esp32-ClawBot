/*
 *    Autor: xbai
 *    Date: 2026/01/17
 */

#include "Os.h"
#include "bsp/bsp_lcd/bsp_lcd.h"
#include "lvgl.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"

void app_main(void)
{
    lv_init();
    bsp_lcd_init();
    nvs_flash_init();
    esp_netif_init();
    OS_Init();
    OS_Start();
}