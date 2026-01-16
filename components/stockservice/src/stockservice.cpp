#include "stockservice.h"

#include <cstdlib>
#include <string_view>

#include <esp_log.h>

#include "netclient.h"

namespace muc::stock
{

static const char* TAG = "STOCK";

static constexpr const char* URL = "https://stooq.com/q/l/?s=nvda.us&f=sd2t2ohlcv&h&e=json";

// Extracts the "close" field from Stooq JSON
static bool extract_price(std::string_view json, double* out_price)
{
    constexpr std::string_view key = "\"close\":";

    std::size_t pos = json.find(key);
    if (pos == std::string_view::npos)
    {
        return false;
    }

    pos += key.size();

    // Skip optional spaces
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
    {
        ++pos;
    }

    // Find end of number: comma or closing brace
    std::size_t end = pos;
    while (end < json.size() &&
           (json[end] == '+' || json[end] == '-' || (json[end] >= '0' && json[end] <= '9') ||
            json[end] == '.' || json[end] == 'e' || json[end] == 'E'))
    {
        ++end;
    }

    if (end == pos)
    {
        return false;
    }

    std::string_view num = json.substr(pos, end - pos);

    *out_price = std::strtod(num.data(), nullptr);
    return true;
}

bool fetch_nvda_price(StockQuote& out)
{
    auto resp = muc::net::https_get(URL);

    if (resp.status != 200)
    {
        ESP_LOGE(TAG, "HTTP error: %d", resp.status);
        return false;
    }

    ESP_LOGI(TAG, "JSON: %s", resp.body);

    std::string_view json(resp.body);

    double price = 0.0;
    if (!extract_price(json, &price))
    {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return false;
    }

    out.price = price;
    return true;
}

} // namespace muc::stock
