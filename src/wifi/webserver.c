/**
 * Web Server Implementation for Remote Control
 * Provides REST API for camera control and file access
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_http_client.h"
#include "esp_mac.h"
#include <sys/time.h>
#include <time.h>
#include "cJSON.h"
#include "webserver.h"
#include "wifi.h"
#include "camera.h"
#include "sdcard.h"
#include "timelapse.h"
#include "power.h"

static const char *TAG = "webserver";

static httpd_handle_t server = NULL;
static SemaphoreHandle_t api_mutex = NULL;
static int server_port = 80;

/**
 * Get current status as JSON
 */
static esp_err_t get_status_handler(httpd_req_t *req)
{
    timelapse_status_t status;
    timelapse_get_status(&status);

    battery_status_t battery;
    power_get_battery_status(&battery);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddNumberToObject(root, "state", status.state);
    cJSON_AddNumberToObject(root, "current_shot", status.current_shot);
    cJSON_AddNumberToObject(root, "total_shots", status.total_shots);
    cJSON_AddNumberToObject(root, "next_shot_sec", status.next_shot_sec);
    cJSON_AddNumberToObject(root, "elapsed_sec", status.elapsed_sec);
    cJSON_AddNumberToObject(root, "saved_count", status.saved_count);
    cJSON_AddNumberToObject(root, "saved_bytes", status.saved_bytes);
    cJSON_AddNumberToObject(root, "free_bytes", status.free_bytes);
    cJSON_AddNumberToObject(root, "battery_voltage", battery.voltage);
    cJSON_AddNumberToObject(root, "battery_percent", battery.percentage);
    cJSON_AddBoolToObject(root, "usb_connected", battery.usb_connected);
    cJSON_AddStringToObject(root, "ip", wifi_get_ip_address());
    cJSON_AddNumberToObject(root, "start_time_sec", status.start_time_sec);
    cJSON_AddNumberToObject(root, "end_time_sec", status.end_time_sec);

    char *json = cJSON_Print(root);
    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    free(json);

    return ESP_OK;
}

/**
 * Start timelapse
 */
static esp_err_t post_start_handler(httpd_req_t *req)
{
    timelapse_start();
    httpd_resp_send(req, "{\"status\":\"started\"}", 20);
    return ESP_OK;
}

/**
 * Stop timelapse
 */
static esp_err_t post_stop_handler(httpd_req_t *req)
{
    timelapse_stop();
    httpd_resp_send(req, "{\"status\":\"stopped\"}", 20);
    return ESP_OK;
}

/**
 * Take single photo
 */
static esp_err_t post_capture_handler(httpd_req_t *req)
{
    esp_err_t ret = timelapse_take_photo();
    if (ret == ESP_OK) {
        httpd_resp_send(req, "{\"status\":\"captured\"}", 22);
    } else {
        httpd_resp_send(req, "{\"status\":\"failed\"}", 19);
    }
    return ESP_OK;
}

/**
 * Get configuration
 */
static esp_err_t get_config_handler(httpd_req_t *req)
{
    timelapse_config_t *config = timelapse_get_config();

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "interval_sec", config->interval_sec);
    cJSON_AddNumberToObject(root, "total_shots", config->total_shots);
    cJSON_AddNumberToObject(root, "resolution", config->resolution);
    cJSON_AddNumberToObject(root, "quality", config->quality);
    cJSON_AddStringToObject(root, "filename_prefix", config->filename_prefix);

    char *json = cJSON_Print(root);
    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    free(json);

    return ESP_OK;
}

/**
 * Update configuration
 */
static esp_err_t post_config_handler(httpd_req_t *req)
{
    int content_len = httpd_req_get_url_query_len(req) + 1;
    if (content_len > 1) {
        char *query = malloc(content_len);
        if (httpd_req_get_url_query_str(req, query, content_len) == ESP_OK) {
            timelapse_config_t config = *timelapse_get_config();

            char param[32];
            if (httpd_query_key_value(query, "interval", param, sizeof(param)) == ESP_OK) {
                config.interval_sec = atoi(param);
            }
            if (httpd_query_key_value(query, "shots", param, sizeof(param)) == ESP_OK) {
                config.total_shots = atoi(param);
            }
            if (httpd_query_key_value(query, "quality", param, sizeof(param)) == ESP_OK) {
                config.quality = atoi(param);
            }
            if (httpd_query_key_value(query, "resolution", param, sizeof(param)) == ESP_OK) {
                config.resolution = atoi(param);
            }

            timelapse_set_config(&config);
            timelapse_save_config();
        }
        free(query);
    }

    httpd_resp_send(req, "{\"status\":\"updated\"}", 20);
    return ESP_OK;
}

/**
 * Set device time (epoch seconds)
 */
static esp_err_t post_time_handler(httpd_req_t *req)
{
    int content_len = httpd_req_get_url_query_len(req) + 1;
    if (content_len > 1) {
        char *query = malloc(content_len);
        if (query && httpd_req_get_url_query_str(req, query, content_len) == ESP_OK) {
            char param[32];
            if (httpd_query_key_value(query, "epoch", param, sizeof(param)) == ESP_OK) {
                struct timeval tv = {0};
                tv.tv_sec = strtoll(param, NULL, 10);
                settimeofday(&tv, NULL);
                // Set timezone to UTC+8 (e.g., China Standard Time) so localtime() matches wall clock
                setenv("TZ", "CST-8", 1);
                tzset();
            }
        }
        free(query);
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"time_set\"}", strlen("{\"status\":\"time_set\"}"));
    return ESP_OK;
}

/**
 * Get preview image
 */
static esp_err_t get_preview_handler(httpd_req_t *req)
{
    camera_fb_t *fb = camera_get_preview();
    if (fb == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_send(req, (const char *)fb->buf, fb->len);
    camera_free_fb(fb);

    return ESP_OK;
}

/**
 * List files on SD card
 */
static esp_err_t get_files_handler(httpd_req_t *req)
{
    static char buffer[4096];
    int count = sdcard_list_files(NULL, buffer, sizeof(buffer));

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "count", count);
    cJSON_AddStringToObject(root, "files", buffer);

    char *json = cJSON_Print(root);
    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    free(json);

    return ESP_OK;
}

/**
 * Download file
 */
static esp_err_t get_file_handler(httpd_req_t *req)
{
    char filename[64];
    if (httpd_req_get_url_query_str(req, filename, sizeof(filename)) != ESP_OK ||
        httpd_query_key_value(filename, "name", filename, sizeof(filename)) != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Read file from SD card
    int64_t size = sdcard_get_file_size(filename);
    if (size < 0) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    uint8_t *buffer = (uint8_t *)malloc(size);
    if (buffer == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    size_t read_size = size;
    if (sdcard_read_file(filename, buffer, (size_t *)&read_size) != ESP_OK) {
        free(buffer);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_send(req, (const char *)buffer, read_size);
    free(buffer);

    return ESP_OK;
}

/**
 * Serve main HTML page
 */
static const char *html_template =
    "<!DOCTYPE html>"
    "<html><head><title>Timelapse Controller</title>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no\">"
    "<style>"
    "*{box-sizing:border-box}"
    "body{font-family:-apple-system,BlinkMacSystemFont,sans-serif;margin:0;background:#f2f2f7;padding:10px;color:#1c1c1e}"
    ".container{max-width:600px;margin:0 auto;background:white;border-radius:16px;overflow:hidden;box-shadow:0 4px 20px rgba(0,0,0,0.08)}"
    "header{padding:15px;text-align:center;border-bottom:1px solid #ebedf0;background:#fff}"
    "h1{margin:0;font-size:18px;font-weight:600}"
    ".status-grid{padding:15px;background:#fbfbfd;display:grid;grid-template-columns:repeat(3,1fr);gap:12px;text-align:center;border-bottom:1px solid #ebedf0}"
    ".stat-item{font-size:11px;color:#8e8e93;text-transform:uppercase;letter-spacing:0.5px}"
    ".stat-val{font-size:15px;font-weight:600;color:#1c1c1e;margin-top:4px}"
    ".preview{background:#000;display:flex;justify-content:center;align-items:center;min-height:240px;position:relative}"
    ".preview img{max-width:100%;max-height:55vh;object-fit:contain;display:none}"
    ".controls{padding:15px;display:grid;grid-template-columns:1fr 1fr 1fr;gap:12px}"
    "button{border:none;border-radius:12px;padding:16px;font-size:15px;font-weight:600;cursor:pointer;transition:opacity 0.2s}"
    "button:active{opacity:0.7}"
    ".btn-start{background:#34c759;color:#fff}"
    ".btn-stop{background:#ff3b30;color:#fff}"
    ".btn-capture{background:#007aff;color:#fff}"
    ".config-panel{padding:0 20px 20px}"
    ".config-header{font-size:13px;font-weight:600;color:#8e8e93;margin:15px 0 10px;text-transform:uppercase}"
    ".input-row{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px;background:#f2f2f7;padding:8px 12px;border-radius:10px}"
    ".input-label{font-size:15px}"
    "input,select{width:140px;border:none;background:transparent;text-align:right;font-size:15px;font-weight:600;color:#007aff;outline:none}"
    ".btn-update{width:100%;background:#5856d6;color:#fff;margin-top:5px}"
    ".footer{text-align:center;padding:20px;color:#c7c7cc;font-size:12px}"
    ".footer a{color:#8e8e93;text-decoration:none;border-bottom:1px dotted}"
    "</style></head>"
    "<body><div class=\"container\">"
    "<header><h1>Timelapse Controller</h1></header>"
    "<div class=\"status-grid\" id=\"status\"></div>"
    "<div class=\"preview\">"
    "<img id=\"preview\" src=\"/preview\" onload=\"this.style.display='block'\" onerror=\"this.style.display='none'\">"
    "</div>"
    "<div class=\"controls\">"
    "<button class=\"btn-start\" onclick=\"api('start')\">Start</button>"
    "<button class=\"btn-stop\" onclick=\"api('stop')\">Stop</button>"
    "<button class=\"btn-capture\" onclick=\"api('capture')\">Snap</button>"
    "</div>"
    "<div class=\"config-panel\">"
    "<div class=\"config-header\">Configuration</div>"
    "<div class=\"input-row\"><span class=\"input-label\">Interval (sec)</span><input type=\"number\" id=\"interval\" inputmode=\"numeric\"></div>"
    "<div class=\"input-row\"><span class=\"input-label\">Target Shots</span><input type=\"number\" id=\"shots\" inputmode=\"numeric\"></div>"
    "<div class=\"input-row\"><span class=\"input-label\">Capture Resolution</span><select id=\"resolution\"></select></div>"
    "<div class=\"input-row\"><span class=\"input-label\">JPEG Quality</span><select id=\"quality\"></select></div>"
    "<div class=\"input-row\"><span class=\"input-label\">Time</span><button class=\"btn-capture\" style=\"width:100%\" onclick=\"syncTime()\">Sync from Device</button></div>"
    "<button class=\"btn-update\" onclick=\"updateConfig()\">Apply Settings</button>"
    "</div>"
    "<div class=\"footer\">%s &bull; <a href=\"/files\">Gallery</a></div>"
    "</div>"
    "<script>"
    "const $=id=>document.getElementById(id);"
    "const resOpts=[{v:5,t:'UXGA 1600x1200'},{v:4,t:'SXGA 1280x1024'},{v:3,t:'XGA 1024x768'},{v:2,t:'SVGA 800x600'},{v:1,t:'VGA 640x480'},{v:0,t:'CIF 352x288'},{v:7,t:'HD 1280x720'},{v:8,t:'FHD 1920x1080'}];"
    "const qOpts=[10,30,50,63];"
    "function hydrateSelects(){"
    "$('resolution').innerHTML=resOpts.map(o=>`<option value=\"${o.v}\">${o.t}</option>`).join('');"
    "$('quality').innerHTML=qOpts.map(v=>`<option value=\"${v}\">${v}</option>`).join('');"
    "}"
    "function renderStatus(d){"
    "const s=['Idle','Running','Paused','Done','Error'];"
    "const h=`"
    "<div class='stat-item'>State<div class='stat-val'>${s[d.state]}</div></div>"
    "<div class='stat-item'>Progress<div class='stat-val'>${d.current_shot}/${d.total_shots}</div></div>"
    "<div class='stat-item'>Next<div class='stat-val'>${d.next_shot_sec}s</div></div>"
    "<div class='stat-item'>Power<div class='stat-val'>${d.battery_percent.toFixed(0)}%</div></div>"
    "<div class='stat-item'>Storage<div class='stat-val'>${(d.free_bytes/1048576).toFixed(0)}MB</div></div>"
    "<div class='stat-item'>Saved<div class='stat-val'>${d.saved_count}</div></div>"
    "<div class='stat-item'>Started<div class='stat-val'>${d.start_time_sec?new Date(d.start_time_sec*1000).toLocaleTimeString():\"-\"}</div></div>"
    "<div class='stat-item'>Ended<div class='stat-val'>${d.end_time_sec?new Date(d.end_time_sec*1000).toLocaleTimeString():\"-\"}</div></div>"
    "<div class='stat-item'>Run<div class='stat-val'>${d.start_time_sec?Math.round(((d.end_time_sec||Date.now()/1000)-d.start_time_sec)/60):0} min</div></div>"
    "`;"
    "$('status').innerHTML=h;"
    "}"
    "function update(){fetch('/status').then(r=>r.json()).then(renderStatus).catch(console.error);setTimeout(update,2000);}" 
    "function api(act){fetch('/'+act,{method:'POST'}).then(r=>r.json()).then(d=>{alert(d.status||'OK');update();});}"
    "function updateConfig(){"
    "const i=$('interval').value,s=$('shots').value,r=$('resolution').value,q=$('quality').value;"
    "fetch(`/config?interval=${i}&shots=${s}&resolution=${r}&quality=${q}`,{method:'POST'}).then(r=>r.json()).then(()=>{alert('Config Updated');update();});"
    "}"
    "function syncTime(){const epoch=Math.floor(Date.now()/1000);fetch(`/time?epoch=${epoch}`,{method:'POST'}).then(r=>r.json()).then(()=>{alert('Time synced');update();});}"
    "function hydrateConfig(){fetch('/config').then(r=>r.json()).then(d=>{$('interval').value=d.interval_sec;$('shots').value=d.total_shots;$('resolution').value=d.resolution;$('quality').value=d.quality;});}"
    "hydrateSelects();"
    "hydrateConfig();"
    "update();"
    "</script></body></html>";

static esp_err_t get_index_handler(httpd_req_t *req)
{
    size_t len = strlen(html_template) + 64; // add slack for IP substitution
    char *html = malloc(len);
    if (html == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    snprintf(html, len, html_template, wifi_get_ip_address());
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    free(html);
    return ESP_OK;
}

/**
 * URI handlers
 */
static const httpd_uri_t uris[] = {
    {"/", HTTP_GET, get_index_handler, NULL},
    {"/index.html", HTTP_GET, get_index_handler, NULL},
    {"/status", HTTP_GET, get_status_handler, NULL},
    {"/start", HTTP_POST, post_start_handler, NULL},
    {"/stop", HTTP_POST, post_stop_handler, NULL},
    {"/capture", HTTP_POST, post_capture_handler, NULL},
    {"/config", HTTP_GET, get_config_handler, NULL},
    {"/config", HTTP_POST, post_config_handler, NULL},
    {"/time", HTTP_POST, post_time_handler, NULL},
    {"/preview", HTTP_GET, get_preview_handler, NULL},
    {"/files", HTTP_GET, get_files_handler, NULL},
    {"/download", HTTP_GET, get_file_handler, NULL}
};

/**
 * Initialize web server
 */
esp_err_t webserver_init(int port)
{
    if (server != NULL) {
        return ESP_OK;
    }

    server_port = port;
    api_mutex = xSemaphoreCreateMutex();

    ESP_LOGI(TAG, "Web server initialized on port %d", port);
    return ESP_OK;
}

/**
 * Deinitialize web server
 */
void webserver_deinit(void)
{
    if (server != NULL) {
        httpd_stop(server);
        server = NULL;
    }
    if (api_mutex != NULL) {
        vSemaphoreDelete(api_mutex);
        api_mutex = NULL;
    }
}

/**
 * Start web server
 */
esp_err_t webserver_start(void)
{
    if (server != NULL) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = server_port;
    config.max_uri_handlers = 16;
    config.stack_size = 16384;
    config.lru_purge_enable = true;

    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register URI handlers
    for (int i = 0; i < sizeof(uris) / sizeof(httpd_uri_t); i++) {
        httpd_register_uri_handler(server, &uris[i]);
    }

    ESP_LOGI(TAG, "Web server started");
    return ESP_OK;
}

/**
 * Stop web server
 */
esp_err_t webserver_stop(void)
{
    if (server == NULL) {
        return ESP_OK;
    }

    httpd_stop(server);
    server = NULL;
    ESP_LOGI(TAG, "Web server stopped");
    return ESP_OK;
}

/**
 * Check if web server is running
 */
bool webserver_is_running(void)
{
    return server != NULL;
}
