idf_component_register(
    SRCS
        "main.c"
        "os/os_alarm.c"
        "os/os.c"
        "bsp/bsp_lcd/bsp_lcd.c"
    INCLUDE_DIRS
        "."
        "os"
        "bsp/bsp_lcd"
    REQUIRES
        lvgl
        esp_wifi
        esp_netif
        nvs_flash
)

add_compile_definitions(LV_CONF_INCLUDE_SIMPLE)
