/*
 *    Autor: xbai
 *    Date: 2026/01/17
 */

#include "Os.h"
#include "Bsp/Bsp_Lcd/Bsp_Lcd.h"
#include "App/App_Lcd/App_Lcd.h"
#include "lvgl.h"

void app_main(void)
{
    lv_init();
    bsp_lcd_init();
    stars_init();
    OS_Init();
    OS_Start();
}