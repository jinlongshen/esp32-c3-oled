extern "C"
{
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
}

static const char* TAG = "OLED_SSD1306_ABROBOT";

// Board pins (Abrobot ESP32-C3 0.42" OLED)
#define SDA_PIN GPIO_NUM_5
#define SCL_PIN GPIO_NUM_6
#define I2C_FREQ 400000

// Confirmed I2C address
#define OLED_ADDR 0x3C

// SSD1306 internal buffer size
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGES (SSD1306_HEIGHT / 8)

// Panel-visible window (from GitHub repo for this exact board)
// Visible area: 73 x 64, at x+27, y+24
#define VIS_X_OFFSET 27
#define VIS_Y_OFFSET 24
#define VIS_WIDTH 73
#define VIS_HEIGHT 64

// Use classic I2C driver on I2C_NUM_0
#define OLED_I2C_PORT I2C_NUM_0

static const uint8_t A_5x8[5] = {0x7C, 0x12, 0x11, 0x12, 0x7C};

// --- I2C helpers (classic driver API) ---

static esp_err_t oled_cmd(uint8_t c)
{
    // control byte 0x00 = command
    uint8_t buf[2] = {0x00, c};

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd)
        return ESP_FAIL;

    esp_err_t err = ESP_OK;
    err |= i2c_master_start(cmd);
    err |=
        i2c_master_write_byte(cmd, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    err |= i2c_master_write(cmd, buf, sizeof(buf), true);
    err |= i2c_master_stop(cmd);

    if (err == ESP_OK)
    {
        err =
            i2c_master_cmd_begin(OLED_I2C_PORT, cmd, 100 / portTICK_PERIOD_MS);
    }

    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "cmd 0x%02X failed: %s", c, esp_err_to_name(err));
    }
    return err;
}

static esp_err_t oled_data(const uint8_t* data, size_t len)
{
    if (len > SSD1306_WIDTH)
        len = SSD1306_WIDTH;

    uint8_t buf[1 + SSD1306_WIDTH];
    buf[0] = 0x40; // control byte 0x40 = data
    memcpy(&buf[1], data, len);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd)
        return ESP_FAIL;

    esp_err_t err = ESP_OK;
    err |= i2c_master_start(cmd);
    err |=
        i2c_master_write_byte(cmd, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    err |= i2c_master_write(cmd, buf, 1 + len, true);
    err |= i2c_master_stop(cmd);

    if (err == ESP_OK)
    {
        err =
            i2c_master_cmd_begin(OLED_I2C_PORT, cmd, 100 / portTICK_PERIOD_MS);
    }

    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "data len %d failed: %s", (int)len, esp_err_to_name(err));
    }
    return err;
}

// Set page and column in SSD1306 (0..7 pages, 0..127 columns)
static void ssd1306_set_page_col(uint8_t page, uint8_t col)
{
    page &= 0x07;
    col &= 0x7F;

    oled_cmd(0xB0 | page);                // page address
    oled_cmd(0x00 | (col & 0x0F));        // lower column
    oled_cmd(0x10 | ((col >> 4) & 0x0F)); // upper column
}

// Canonical SSD1306 init sequence (128x64)
static void ssd1306_init()
{
    // Display off
    oled_cmd(0xAE);

    // Set display clock divide / oscillator frequency
    oled_cmd(0xD5);
    oled_cmd(0x80);

    // Set multiplex ratio (1/64)
    oled_cmd(0xA8);
    oled_cmd(0x3F);

    // Set display offset
    oled_cmd(0xD3);
    oled_cmd(0x00);

    // Set display start line to 0
    oled_cmd(0x40);

    // Charge pump setting
    oled_cmd(0x8D);
    oled_cmd(0x14); // enable charge pump (for internal VCC)

    // Memory addressing mode: horizontal
    oled_cmd(0x20);
    oled_cmd(0x00);

    // Segment remap: column address 127 is mapped to SEG0
    oled_cmd(0xA1);

    // COM output scan direction: remapped (COM[N-1] to COM0)
    oled_cmd(0xC8);

    // Set COM pins hardware configuration
    oled_cmd(0xDA);
    oled_cmd(0x12);

    // Set contrast control
    oled_cmd(0x81);
    oled_cmd(0x7F);

    // Set pre-charge period
    oled_cmd(0xD9);
    oled_cmd(0xF1);

    // Set VCOMH deselect level
    oled_cmd(0xDB);
    oled_cmd(0x40);

    // Disable entire display on, resume to RAM content
    oled_cmd(0xA4);

    // Normal display (not inverted)
    oled_cmd(0xA6);

    // Clear full 128x64 buffer
    uint8_t zeros[SSD1306_WIDTH];
    memset(zeros, 0x00, sizeof(zeros));
    for (int page = 0; page < SSD1306_PAGES; ++page)
    {
        ssd1306_set_page_col(page, 0);
        oled_data(zeros, SSD1306_WIDTH);
    }

    // Display on
    oled_cmd(0xAF);
}

// Draw a white bar in the visible window at y ≈ 24 (page = 3)
static void draw_visible_bar()
{
    // Page for y = 24 is 24 / 8 = 3
    uint8_t page = (VIS_Y_OFFSET / 8); // 24/8 = 3

    // We'll draw 32 columns wide bar starting at x = 27
    const int bar_width = 32;
    uint8_t buf[bar_width];

    // Each byte is a vertical column of 8 pixels; set all bits = 1
    memset(buf, 0xFF, sizeof(buf));

    // Set page and column inside the 128x64 buffer
    ssd1306_set_page_col(page, VIS_X_OFFSET); // x = 27
    oled_data(buf, bar_width);

    ESP_LOGI(TAG, "Drew white bar at x=%d..%d, page=%d (y≈%d..%d)",
             VIS_X_OFFSET, VIS_X_OFFSET + bar_width - 1, page, page * 8,
             page * 8 + 7);
}

void draw_char_A(int x, int y)
{
    int col = VIS_X_OFFSET + x;
    int page = (VIS_Y_OFFSET + y) / 8;

    ssd1306_set_page_col(page, col);
    oled_data(A_5x8, 5);
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG,
             "Starting SSD1306 test for Abrobot 0.42\" OLED (classic I2C)");

    // --- I2C BUS SETUP (classic driver) ---
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = SDA_PIN;
    conf.scl_io_num = SCL_PIN;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_FREQ;
#if SOC_I2C_SUPPORT_REF_TICK
    conf.clk_flags = 0; // use default clock source
#endif

    ESP_ERROR_CHECK(i2c_param_config(OLED_I2C_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(OLED_I2C_PORT, conf.mode, 0, 0, 0));
    ESP_LOGI(TAG, "I2C master initialized on port %d", OLED_I2C_PORT);

    // Sanity ping: send a NOP command (0x00) with control byte 0x00
    uint8_t ping_buf[2] = {0x00, 0x00};

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t ping_err = ESP_OK;
    ping_err |= i2c_master_start(cmd);
    ping_err |=
        i2c_master_write_byte(cmd, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    ping_err |= i2c_master_write(cmd, ping_buf, sizeof(ping_buf), true);
    ping_err |= i2c_master_stop(cmd);
    if (ping_err == ESP_OK)
    {
        ping_err =
            i2c_master_cmd_begin(OLED_I2C_PORT, cmd, 100 / portTICK_PERIOD_MS);
    }
    i2c_cmd_link_delete(cmd);

    ESP_LOGI(TAG, "Ping transmit result: %s", esp_err_to_name(ping_err));

    // --- SSD1306 INIT ---
    ssd1306_init();
    ESP_LOGI(TAG, "SSD1306 init done");

    // --- DRAW SOMETHING IN THE VISIBLE WINDOW ---
    draw_visible_bar();
    ESP_LOGI(TAG, "If the offsets are correct, you should see a bright bar");

    // draw_char_A(1, 12);
    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
