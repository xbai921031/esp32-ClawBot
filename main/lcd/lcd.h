/*
 *    Autor: xbai
 *    Date: 2026/01/19
 */

#pragma once

#include <stdint.h>
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_interface.h"

extern esp_lcd_panel_handle_t panel_handle;

void lcd_init(void);
void lcd_fill_color(uint16_t color);

