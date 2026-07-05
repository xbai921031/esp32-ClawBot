/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *app_fs_storage_base_path(void);
const char *app_fs_system_base_path(void);
esp_err_t app_fs_init(void);
esp_err_t app_fs_create_default_router_rules(void);

#ifdef __cplusplus
}
#endif
