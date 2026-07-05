set(app_claw_srcs
    "app_claw.c"
    "app_capabilities.c"
)

if(CONFIG_APP_CLAW_ENABLE_CLI)
    list(APPEND app_claw_srcs "app_claw_cli.c")
endif()

idf_component_register(
    SRCS
        ${app_claw_srcs}
    INCLUDE_DIRS
        "include"
    REQUIRES
        cap_llm_config
        claw_cap
        claw_core
        claw_event_router
        claw_manager
        claw_memory
        claw_skill
        cap_im_platform
        freertos
    PRIV_REQUIRES
        esp_timer
)
