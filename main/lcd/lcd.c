/*
 * Author: xbai
 * Date  : 2026/01/19
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "lcd.h"

#define LCD_SPI_HOST   SPI2_HOST

#define PIN_NUM_MOSI   41
#define PIN_NUM_CLK    40
#define PIN_NUM_CS     39
#define PIN_NUM_DC     38
#define PIN_NUM_RST    42
#define PIN_NUM_BK     20

#define LCD_WIDTH  240
#define LCD_HEIGHT 240
#define BLOCK_HEIGHT 16

esp_lcd_panel_handle_t panel_handle = NULL;

void lcd_init(void)
{
    /* SPI init */
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * BLOCK_HEIGHT * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    /* IO panel */
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_SPI_HOST, &io_config, &io_handle));

    /* ST7789 panel config */
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    /* Panel init */
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 0));

    /* Backlight init */
    gpio_set_direction(PIN_NUM_BK, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_BK, 1);
}

#if 0
void lcd_fill_color(uint16_t color)
{
    uint16_t *buf = heap_caps_malloc(LCD_WIDTH * BLOCK_HEIGHT * sizeof(uint16_t), MALLOC_CAP_DMA);
    if(!buf)
    {
        return;
    }

    for(int i = 0; i < LCD_WIDTH * BLOCK_HEIGHT; i++)
    {
        buf[i] = color;
    }

    for(int y = 0; y < LCD_HEIGHT; y += BLOCK_HEIGHT)
    {
        int h = (y + BLOCK_HEIGHT <= LCD_HEIGHT) ? BLOCK_HEIGHT : (LCD_HEIGHT - y);
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, y, LCD_WIDTH, y + h, buf));
    }

    free(buf);
}
#endif