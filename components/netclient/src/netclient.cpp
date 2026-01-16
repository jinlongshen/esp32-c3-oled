#include "netclient.h"

#include <array>
#include <cstring>

#include <esp_crt_bundle.h>
#include <esp_err.h>
#include <esp_http_client.h>
#include <esp_log.h>

namespace muc::net
{

static const char* TAG = "NET_CLIENT";

static constexpr std::size_t MAX_BODY_SIZE = 4096;
static std::array<char, MAX_BODY_SIZE> s_body_buffer{};
static std::size_t s_body_len = 0;

static esp_err_t http_event_handler(esp_http_client_event_t* evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data_len > 0)
    {
        std::size_t max_copy = MAX_BODY_SIZE - 1 - s_body_len;
        if (max_copy == 0)
        {
            return ESP_OK;
        }

        std::size_t copy_len = evt->data_len;
        if (copy_len > max_copy)
        {
            copy_len = max_copy;
        }

        std::memcpy(s_body_buffer.data() + s_body_len, evt->data, copy_len);
        s_body_len += copy_len;
    }

    return ESP_OK;
}

HttpResponse https_get(const char* url)
{
    HttpResponse resp{};
    resp.body = ""; // safe default
    resp.length = 0;

    s_body_len = 0;
    s_body_buffer[0] = '\0';

    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_GET;
    config.event_handler = http_event_handler;
    config.user_data = nullptr;
    config.timeout_ms = 5000;
    config.crt_bundle_attach = esp_crt_bundle_attach; // TLS verification

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Required for Stooq to return a body
    esp_http_client_set_header(client, "User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "HTTPS GET failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        resp.status = -1;
        return resp;
    }

    resp.status = esp_http_client_get_status_code(client);

    s_body_buffer[s_body_len] = '\0';
    resp.body = s_body_buffer.data();
    resp.length = s_body_len;

    esp_http_client_cleanup(client);
    return resp;
}

} // namespace muc::net
