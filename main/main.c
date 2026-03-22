/*
 *    Autor: xbai
 *    Date: 2026/01/17
 */

#include "Os.h"
#include "lcd.h"
#include "lvgl.h"
#include "wifi.h"
#include "webserver.h"

void app_main(void)
{
    lv_init();
    lcd_init();
    wifi_init();
    start_webserver();
    OS_Init();
    OS_Start();
}