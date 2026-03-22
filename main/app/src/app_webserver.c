/*
 * Author: xbai
 * Date  : 2026/03/22
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"

#define WS_PORT     81
#define WS_URI      "/ws"

static const char *TAG = "WS_ECHO";
static httpd_handle_t server = NULL;

static esp_err_t ws_handler(httpd_req_t *req)
{
    if(req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "WebSocket handshake done");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if(ret != ESP_OK)
    {
        return ret;
    }

    if(ws_pkt.len)
    {
        buf = malloc(ws_pkt.len + 1);
        if(!buf)
        {
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if(ret != ESP_OK)
        {
            free(buf);
            return ret;
        }
        buf[ws_pkt.len] = '\0';
        ESP_LOGI(TAG, "Received: %s", buf);

        ws_pkt.payload = (uint8_t *)buf;
        ret = httpd_ws_send_frame(req, &ws_pkt);
        if(ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to echo");
        }
        free(buf);
    }
    return ESP_OK;
}

void start_webserver(void)
{
    static const httpd_uri_t ws_uri = {
        .uri        = WS_URI,
        .method     = HTTP_GET,
        .handler    = ws_handler,
        .user_ctx   = NULL,
        .is_websocket = true
    };
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WS_PORT;
    config.stack_size = 4096;
    config.lru_purge_enable = true;

    if(httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &ws_uri);
        ESP_LOGI(TAG, "WebSocket server started on port %d", WS_PORT);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to start server");
    }
}

