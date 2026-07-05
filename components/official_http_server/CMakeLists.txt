set(http_server_srcs
    "http_server_core.c"
    "http_server_assets.c"
    "http_server_capabilities_api.c"
    "http_server_config_api.c"
    "http_server_status_api.c"
    "http_server_files_api.c"
    "http_server_wechat_api.c"
    "http_server_webim_api.c"
    "http_server_json.c"
    "http_server_utils.c"
)

set(http_server_requires
    app_config
    app_claw
    cap_im_local
    cap_im_platform
    claw_cap
    esp_http_server
    heap
    lvgl
)

idf_component_register(
    SRCS
        ${http_server_srcs}
    INCLUDE_DIRS
        "include"
    EMBED_FILES
        "frontend_source/dist/index.html.gz"
        "frontend_source/public/favicon.ico"
    REQUIRES
        ${http_server_requires}
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
