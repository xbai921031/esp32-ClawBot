/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * Asset handlers: embedded files and simple single-page config UI.
 */
#include "http_server_priv.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#define LV_USE_QRCODE 1
#define QRCODEGEN_TEST
#include "lvgl/src/libs/qrcode/qrcodegen.h"
#include "lvgl/src/libs/qrcode/qrcodegen.c"

extern const uint8_t index_html_gz_start[] asm("_binary_index_html_gz_start");
extern const uint8_t index_html_gz_end[]   asm("_binary_index_html_gz_end");

extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[]   asm("_binary_favicon_ico_end");

static esp_err_t favicon_handler(httpd_req_t *req)
{
    return http_server_send_embedded_file(req,
                                          favicon_ico_start,
                                          favicon_ico_end,
                                          "image/x-icon");
}

/* ── Simple single-page config UI (served at root "/") ──────────── */

static const char SIMPLE_PAGE_HTML[] =
"<!DOCTYPE html>\n"
"<html lang='zh-CN'>\n"
"<head>\n"
"<meta charset='UTF-8'>\n"
"<meta name='viewport' content='width=device-width,initial-scale=1'>\n"
"<title>ESP-ClawBot</title>\n"
"<style>\n"
"*{margin:0;padding:0;box-sizing:border-box}\n"
"body{background:#0e1014;color:#e0e0e0;font-family:sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;padding:16px}\n"
".box{background:#1a1c23;border:1px solid #2a2d38;border-radius:12px;max-width:400px;width:100%;padding:24px}\n"
"h1{text-align:center;font-size:18px;color:#e4533c;margin-bottom:4px}\n"
".sub{text-align:center;font-size:12px;color:#888;margin-bottom:16px}\n"
".dots{display:flex;gap:6px;justify-content:center;margin-bottom:16px}\n"
".dots i{width:10px;height:10px;border-radius:50%;background:#2a2d38}\n"
".dots i.on{background:#e4533c}\n"
"label{display:block;font-size:12px;color:#aaa;margin-bottom:4px}\n"
"input{width:100%;padding:10px;border-radius:8px;border:1px solid #2a2d38;background:#13151b;color:#e0e0e0;font-size:14px;outline:none;margin-bottom:12px}\n"
"input:focus{border-color:#e4533c}\n"
".pg{display:none}\n"
".pg.on{display:block}\n"
".btns{display:flex;gap:8px;margin-top:14px}\n"
".btns button{flex:1;padding:10px;border-radius:8px;border:1px solid #2a2d38;background:#13151b;color:#ccc;font-size:14px;cursor:pointer}\n"
".btns button.red{background:#e4533c;color:#fff;border-color:#e4533c}\n"
".btns button:disabled{opacity:.3;cursor:default}\n"
".msg{margin-top:10px;padding:8px 12px;border-radius:8px;font-size:13px;display:none}\n"
".msg.s{display:block;background:rgba(80,200,120,.15);border:1px solid #50c878;color:#7ddfa0}\n"
".msg.e{display:block;background:rgba(228,83,60,.15);border:1px solid #e4533c;color:#ff9a8a}\n"
".msg.w{display:block;background:rgba(50,120,220,.15);border:1px solid #3278dc;color:#8ab8ff}\n"
".wx-ct{text-align:center;padding:8px 0}\n"
".wx-ct canvas{background:#fff;padding:6px;border-radius:10px;max-width:180px;margin:0 auto 6px;display:none}\n"
".wx-ct p{font-size:12px;color:#888;margin:4px 0}\n"
".wx-ct .wxb{padding:7px 16px;border-radius:6px;border:1px solid #2a2d38;background:#13151b;color:#ccc;font-size:12px;cursor:pointer}\n"
".wx-ct .wxb:disabled{opacity:.4}\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class='box'>\n"
"<div class='dots'><i id='d1' class='on'></i><i id='d2'></i><i id='d3'></i></div>\n"
"<h1 id='title'>WiFi 配置</h1>\n"
"<p class='sub' id='subtitle'>第1步：配置WiFi网络</p>\n"

"<div id='p1' class='pg on'>\n"
"<label>WiFi 名称 (SSID)</label>\n"
"<input id='wifi_ssid' placeholder='输入WiFi名称'>\n"
"<label>WiFi 密码</label>\n"
"<input id='wifi_password' type='password' placeholder='留空即无密码'>\n"
"</div>\n"

"<div id='p2' class='pg'>\n"
"<label>API Key</label>\n"
"<input id='llm_api_key' type='password' placeholder='sk-...'>\n"
"<label>模型名称</label>\n"
"<input id='llm_model' placeholder='deepseek-v4-pro'>\n"
"<label>接口地址</label>\n"
"<input id='llm_base_url' placeholder='https://api.deepseek.com'>\n"
"<label>Backend</label>\n"
"<input id='llm_backend_type' placeholder='openai_compatible'>\n"
"</div>\n"

"<div id='p3' class='pg'>\n"
"<div class='wx-ct'>\n"
"<canvas id='qr'></canvas>\n"
"<p id='wstat'>未绑定</p>\n"
"<button class='wxb' id='wbb'>生成二维码</button>\n"
"<button class='wxb' id='wbc' style='display:none'>取消</button>\n"
"</div>\n"
"</div>\n"

"<div class='btns'>\n"
"<button id='prev'>上一步</button>\n"
"<button id='next'>下一步</button>\n"
"</div>\n"
"<div class='btns'>\n"
"<button id='save' class='red'>保存并重启</button>\n"
"</div>\n"
"<div id='msg' class='msg'></div>\n"

"<input id='wechat_token' type='hidden'>\n"
"<input id='wechat_account_id' type='hidden'>\n"
"<input id='wechat_base_url' type='hidden'>\n"
"<input id='wechat_cdn_base_url' type='hidden'>\n"

"<script>\n"
"var pg=1\n"
"var titles=['WiFi 配置','大模型配置','微信绑定']\n"
"var subs=['第1步：配置WiFi网络','第2步：配置大模型','第3步：微信扫码绑定']\n"
"var wxPoll=null,wxOn=false\n"

"function $(id){return document.getElementById(id)}\n"
"function msg(c,t){var e=$('msg');e.className='msg '+c;e.textContent=t}\n"
"function gv(a){var o={};for(var i=0;i<a.length;i++){var k=a[i];o[k]=$(k).value.trim()||''};return o}\n"

"function show(){\n"
"for(var i=1;i<=3;i++){$('p'+i).className=i==pg?'pg on':'pg'}\n"
"for(var i=1;i<=3;i++){$('d'+i).className=i==pg?'on':''}\n"
"$('title').textContent=titles[pg-1]\n"
"$('subtitle').textContent=subs[pg-1]\n"
"$('prev').disabled=pg==1\n"
"$('next').textContent=pg<3?'下一步':'完成'\n"
"}\n"

"$('next').addEventListener('click',function(){if(pg<3){pg++;show()}})\n"
"$('prev').addEventListener('click',function(){if(pg>1){pg--;show()}})\n"

"$('save').addEventListener('click',function(){\n"
"var btn=this;btn.disabled=true\n"
"var body\n"
"if(pg==1)body=gv(['wifi_ssid','wifi_password'])\n"
"else if(pg==2)body=gv(['llm_api_key','llm_model','llm_base_url','llm_backend_type'])\n"
"else body=gv(['wechat_token','wechat_account_id','wechat_base_url','wechat_cdn_base_url'])\n"
"msg('w','正在保存...')\n"
"var x=new XMLHttpRequest()\n"
"x.open('POST','/api/config',true)\n"
"x.setRequestHeader('Content-Type','application/json')\n"
"x.onload=function(){\n"
"if(x.status>=400){msg('e','保存失败: HTTP '+x.status);btn.disabled=false;return}\n"
"try{var j=JSON.parse(x.responseText);if(!j.ok)throw new Error(j.message||'未知错误')\n"
"msg('s','已保存，正在重启...')\n"
"var r=new XMLHttpRequest();r.open('POST','/api/restart',true);r.send()\n"
"}catch(e){msg('e','失败: '+e.message);btn.disabled=false}\n"
"}\n"
"x.onerror=function(){msg('e','网络错误');btn.disabled=false}\n"
"x.send(JSON.stringify(body))\n"
"})\n"

"$('wbb').addEventListener('click',function(){\n"
"var btn=this;btn.disabled=true;btn.textContent='请求中...'\n"
"msg('w','连接微信服务...')\n"
"var x=new XMLHttpRequest()\n"
"x.open('POST','/api/wechat/login/start',true)\n"
"x.setRequestHeader('Content-Type','application/json')\n"
"x.onload=function(){\n"
"if(x.status>=400){var d;try{d=JSON.parse(x.responseText)}catch(e){};msg('e','启动失败: '+(d&&d.error||'HTTP '+x.status));btn.disabled=false;btn.textContent='生成二维码';return}\n"
"btn.textContent='刷新二维码';btn.disabled=false\n"
"$('wbc').style.display='inline-block';wxOn=true\n"
"wxPollLoop()\n"
"}\n"
"x.onerror=function(){msg('e','网络错误');btn.disabled=false;btn.textContent='生成二维码'}\n"
"x.send('{\"account_id\":\"default\",\"force\":true}')\n"
"})\n"

"function doQr(t){\n"
"var c=$('qr');c.style.display='none';var old=document.getElementById('qrimg');if(old)old.remove()\n"
"if(!t)return\n"
"var img=document.createElement('img');img.id='qrimg'\n"
"img.src='/api/qr?data='+encodeURIComponent(t)\n"
"img.style.cssText='max-width:180px;display:block;margin:0 auto;border-radius:6px;background:#fff;padding:6px'\n"
"c.parentNode.insertBefore(img,c)\n"
"}\n"

"function wxPollLoop(){\n"
"if(!wxOn)return\n"
"var x=new XMLHttpRequest()\n"
"x.open('GET','/api/wechat/login/status',true)\n"
"x.onload=function(){\n"
"if(x.status>=400){if(wxOn)wxPoll=setTimeout(wxPollLoop,3000);return}\n"
"try{var d=JSON.parse(x.responseText)\n"
"if(d.qr_data_url)doQr(d.qr_data_url)\n"
"if(d.status)$('wstat').textContent=d.status\n"
"if(d.completed&&d.token){\n"
"$('wechat_token').value=d.token||''\n"
"$('wechat_account_id').value=d.account_id||'default'\n"
"if(d.base_url)$('wechat_base_url').value=d.base_url\n"
"$('wstat').textContent='已绑定'\n"
"msg('s','微信绑定成功！')\n"
"wxOn=false;$('wbc').style.display='none'\n"
"return\n"
"}\n"
"if(wxOn)wxPoll=setTimeout(wxPollLoop,1500)\n"
"}catch(e){if(wxOn)wxPoll=setTimeout(wxPollLoop,3000)}\n"
"}\n"
"x.onerror=function(){if(wxOn)wxPoll=setTimeout(wxPollLoop,3000)}\n"
"x.send()\n"
"}\n"

"$('wbc').addEventListener('click',function(){\n"
"wxOn=false;if(wxPoll)clearTimeout(wxPoll)\n"
"var x=new XMLHttpRequest();x.open('POST','/api/wechat/login/cancel',true);x.send()\n"
"doQr('')\n"
"this.style.display='none'\n"
"$('wbb').textContent='生成二维码';$('wstat').textContent='已取消'\n"
"})\n"

"show()\n"
"</script>\n"
"</div>\n"
"</body>\n"
"</html>\n";

/* ── QR code image endpoint (/api/qr?data=URL) ──────────── */

static esp_err_t qr_image_handler(httpd_req_t *req)
{
    size_t query_len = httpd_req_get_url_query_len(req) + 1;
    if (query_len > 1) {
        char *query = malloc(query_len);
        if (!query) {
            httpd_resp_send_500(req);
            return ESP_ERR_NO_MEM;
        }
        httpd_req_get_url_query_str(req, query, query_len);

        char param[512];
        if (httpd_query_key_value(query, "data", param, sizeof(param)) == ESP_OK) {
            free(query);
            /* URL-decode the parameter */
            size_t len = strlen(param);
            char *decoded = malloc(len + 1);
            if (!decoded) {
                httpd_resp_send_500(req);
                return ESP_ERR_NO_MEM;
            }
            size_t dlen = 0;
            for (size_t i = 0; i < len; i++) {
                if (param[i] == '%' && i + 2 < len) {
                    char hex[3] = {param[i+1], param[i+2], '\0'};
                    decoded[dlen++] = (char)strtol(hex, NULL, 16);
                    i += 2;
                } else if (param[i] == '+') {
                    decoded[dlen++] = ' ';
                } else {
                    decoded[dlen++] = param[i];
                }
            }
            decoded[dlen] = '\0';

            /* Generate QR code (buffers on heap to avoid stack overflow) */
            uint8_t *temp = malloc(qrcodegen_BUFFER_LEN_MAX);
            uint8_t *qr = malloc(qrcodegen_BUFFER_LEN_MAX);
            if (!temp || !qr) {
                free(temp); free(qr);
                free(decoded);
                httpd_resp_send_500(req);
                return ESP_ERR_NO_MEM;
            }
            if (qrcodegen_encodeText(decoded, temp, qr, qrcodegen_Ecc_MEDIUM,
                                     1, 40, qrcodegen_Mask_AUTO, true)) {
                int qr_size = qrcodegen_getSize(qr);
                int scale = 4;
                int w = qr_size * scale;
                int h = qr_size * scale;
                int row_bytes = ((w * 3 + 3) / 4) * 4;
                uint32_t image_size = row_bytes * h;
                uint32_t file_size = 54 + image_size;

                uint8_t *bmp = malloc(file_size);
                if (!bmp) {
                    free(temp); free(qr);
                    free(decoded);
                    httpd_resp_send_500(req);
                    return ESP_ERR_NO_MEM;
                }

                /* BMP header */
                memset(bmp, 0, 54);
                bmp[0] = 'B'; bmp[1] = 'M';                    /* bfType */
                bmp[2] = file_size & 0xFF;                      /* bfSize */
                bmp[3] = (file_size >> 8) & 0xFF;
                bmp[4] = (file_size >> 16) & 0xFF;
                bmp[5] = (file_size >> 24) & 0xFF;
                bmp[10] = 54;                                    /* bfOffBits */
                bmp[14] = 40;                                    /* biSize */
                bmp[18] = w & 0xFF;                              /* biWidth */
                bmp[19] = (w >> 8) & 0xFF;
                bmp[22] = h & 0xFF;                              /* biHeight */
                bmp[23] = (h >> 8) & 0xFF;
                bmp[26] = 1;                                     /* biPlanes */
                bmp[28] = 24;                                    /* biBitCount */
                bmp[34] = image_size & 0xFF;                    /* biSizeImage */
                bmp[35] = (image_size >> 8) & 0xFF;
                bmp[36] = (image_size >> 16) & 0xFF;
                bmp[37] = (image_size >> 24) & 0xFF;
                bmp[38] = 0x13; bmp[39] = 0x0B;                /* 2835 pixels/m = 72 DPI */
                bmp[42] = 0x13; bmp[43] = 0x0B;

                /* Pixel data (BGR, bottom-up) */
                for (int y = 0; y < h; y++) {
                    int qy = (h - 1 - y) / scale;
                    for (int x = 0; x < w; x++) {
                        int qx = x / scale;
                        bool black = qrcodegen_getModule(qr, qx, qy);
                        uint8_t val = black ? 0x00 : 0xFF;
                        int offset = 54 + y * row_bytes + x * 3;
                        bmp[offset]     = val;  /* B */
                        bmp[offset + 1] = val;  /* G */
                        bmp[offset + 2] = val;  /* R */
                    }
                }

                free(temp); free(qr);
                free(decoded);
                httpd_resp_set_type(req, "image/bmp");
                httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
                esp_err_t err = httpd_resp_send(req, (const char *)bmp, file_size);
                free(bmp);
                return err;
            }

            free(temp); free(qr);
            free(decoded);
        } else {
            free(query);
        }
    }
    httpd_resp_send_500(req);
    return ESP_FAIL;
}

static void ensure_config_html(void)
{
    const char *path = "/fatfs/config.html";
    struct stat st;
    if (stat(path, &st) == 0) return;

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return;
    write(fd, SIMPLE_PAGE_HTML, sizeof(SIMPLE_PAGE_HTML) - 1);
    close(fd);
}

static esp_err_t simple_config_handler(httpd_req_t *req)
{
    ensure_config_html();

    int fd = open("/fatfs/config.html", O_RDONLY);
    if (fd < 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    struct stat st;
    fstat(fd, &st);

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");

    char *buf = malloc(st.st_size + 1);
    if (!buf) {
        close(fd);
        httpd_resp_send_500(req);
        return ESP_ERR_NO_MEM;
    }

    ssize_t total = 0;
    while (total < st.st_size) {
        ssize_t n = read(fd, buf + total, st.st_size - total);
        if (n <= 0) break;
        total += n;
    }
    close(fd);
    buf[total] = '\0';

    esp_err_t err = httpd_resp_send(req, buf, total);
    free(buf);
    return err;
}

/* ── Original frontend (served at /index.html for advanced users) ── */

static esp_err_t frontend_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Vary", "Accept-Encoding");
    return http_server_send_embedded_file(req,
                                          index_html_gz_start,
                                          index_html_gz_end,
                                          "text/html; charset=utf-8");
}

esp_err_t http_server_register_assets_routes(httpd_handle_t server)
{
    const httpd_uri_t handlers[] = {
        { .uri = "/",           .method = HTTP_GET, .handler = simple_config_handler },
        { .uri = "/api/qr",     .method = HTTP_GET, .handler = qr_image_handler },
        { .uri = "/index.html", .method = HTTP_GET, .handler = frontend_handler },
        { .uri = "/favicon.ico", .method = HTTP_GET, .handler = favicon_handler },
    };

    for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); ++i) {
        esp_err_t err = httpd_register_uri_handler(server, &handlers[i]);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;
}
