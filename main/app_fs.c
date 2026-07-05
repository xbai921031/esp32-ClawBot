/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * Filesystem init for 16MB flash boards (W25Q128)
 */
#include "app_fs.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_vfs_fat.h"
#include "wear_levelling.h"
#include "claw_ramfs.h"

#define APP_FS_SYSTEM_PARTITION_LABEL   "system"
#define APP_FS_STORAGE_PARTITION_LABEL  "storage"
#define APP_FS_RAMFS_MAX_FILES          (4)
#define APP_FS_RAMFS_MAX_BYTES          (32 * 1024)

static const char *TAG = "app_fs";

static const char *const s_system_base_path = "/system";
static const char *const s_storage_base_path = "/fatfs";
static const char *const s_ramfs_base_path = "/ramfs";

static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

const char *app_fs_storage_base_path(void)
{
    return s_storage_base_path;
}

const char *app_fs_system_base_path(void)
{
    return s_system_base_path;
}

static esp_err_t app_fs_init_system(void)
{
    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 4,
        .allocation_unit_size = 4096,
        .disk_status_check_enable = false,
        .use_one_fat = false,
    };
    esp_err_t ret = esp_vfs_fat_spiflash_mount_rw_wl(
        s_system_base_path, APP_FS_SYSTEM_PARTITION_LABEL,
        &mount_config, &s_wl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount system partition: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "System FATFS mounted at %s", s_system_base_path);
    return ESP_OK;
}

static esp_err_t app_fs_init_storage(void)
{
    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 8,
        .allocation_unit_size = 4096,
        .disk_status_check_enable = false,
        .use_one_fat = false,
    };
    esp_err_t ret = esp_vfs_fat_spiflash_mount_rw_wl(
        s_storage_base_path, APP_FS_STORAGE_PARTITION_LABEL,
        &mount_config, &s_wl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount storage partition: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Storage FATFS mounted at %s", s_storage_base_path);
    return ESP_OK;
}

static esp_err_t app_fs_init_ramfs(void)
{
    const claw_ramfs_config_t config = {
        .base_path = s_ramfs_base_path,
        .max_files = APP_FS_RAMFS_MAX_FILES,
        .max_bytes = APP_FS_RAMFS_MAX_BYTES,
        .caps = MALLOC_CAP_8BIT,
    };
    ESP_RETURN_ON_ERROR(claw_ramfs_register(&config), TAG,
                        "Failed to mount RAMFS at %s", s_ramfs_base_path);
    ESP_LOGI(TAG, "RAMFS mounted at %s", s_ramfs_base_path);
    return ESP_OK;
}

esp_err_t app_fs_init(void)
{
    ESP_RETURN_ON_ERROR(app_fs_init_system(), TAG, "Failed to mount system FATFS");
    ESP_RETURN_ON_ERROR(app_fs_init_storage(), TAG, "Failed to mount storage FATFS");
    esp_err_t ret = app_fs_init_ramfs();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "RAMFS init failed (%s), continuing without RAMFS", esp_err_to_name(ret));
    }
    app_fs_create_default_router_rules();
    return ESP_OK;
}

static const char APP_FS_ROUTER_RULES_CONTENT[] =
    "[\n"
    "  {\n"
    "    \"id\": \"default_agent_response\",\n"
    "    \"match\": {\n"
    "      \"event_type\": \"out_message\",\n"
    "      \"source_cap\": \"claw_core\"\n"
    "    },\n"
    "    \"actions\": [\n"
    "      {\n"
    "        \"type\": \"send_message\",\n"
    "        \"input\": {\n"
    "          \"channel\": \"{{event.source_channel}}\",\n"
    "          \"chat_id\": \"{{event.chat_id}}\",\n"
    "          \"message\": \"{{event.text}}\"\n"
    "        }\n"
    "      }\n"
    "    ]\n"
    "  }\n"
    "]\n";

esp_err_t app_fs_create_default_router_rules(void)
{
    const char *dir = "/fatfs/router_rules";
    const char *path = "/fatfs/router_rules/router_rules.json";
    struct stat st;

    if (stat(path, &st) == 0) {
        return ESP_OK;  /* already exists */
    }

    mkdir(dir, 0755);

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        ESP_LOGE(TAG, "Failed to create %s", path);
        return ESP_FAIL;
    }
    write(fd, APP_FS_ROUTER_RULES_CONTENT, sizeof(APP_FS_ROUTER_RULES_CONTENT) - 1);
    close(fd);

    ESP_LOGI(TAG, "Created default router rules: %s", path);
    return ESP_OK;
}
