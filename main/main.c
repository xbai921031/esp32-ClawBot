/*
 *    Autor: xbai
 *    Date: 2026/01/17
 */

#include "Os.h"
#include "bsp_lcd.h"
#include "lvgl.h"
#include "app_wifi.h"
#include "app_webserver.h"

void app_main(void)
{
    lv_init();
    bsp_lcd_init();
    wifi_init();
    start_webserver();
    OS_Init();
    OS_Start();
}