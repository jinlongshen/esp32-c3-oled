ESP32-C3 OLED Project Guide (Abrobot/01Space Variant)

🛠 Build & Control Commands

Build: idf.py build

Flash: idf.py -p /dev/ttyUSB0 flash

Monitor: idf.py monitor (Ctrl+] to exit)

Clean: idf.py fullclean

🔌 Hardware Configuration (Abrobot / 01Space)

Variant: Abrobot ESP32-C3 0.42" OLED

Display: SSD1306 (72x40 pixels mapped to 128x64 RAM)

I2C SDA: GPIO 5

I2C SCL: GPIO 6

Address: 0x3C

Verified Geometry:

Width: 72, Height: 40

X Offset: 28

Y Offset: 24

Onboard LED: GPIO 8 (Blue)

📂 Code Intelligence

Pin Definitions: Check components/I2CDevice/inc/I2CBus.h.

Geometry Constants: Found in components/oled/inc/display_geometry.h.

I2C Implementation: components/I2CDevice/ and main/src/I2CBus.cpp.

UI Logic: LVGL tasks and screen clearing are in main/src/ui_consumer_task.cpp.

🤖 Optimized Agent Prompts (For Claude CLI)

Verify Alignment: claude -p "Read components/oled/inc/display_geometry.h. Ensure kDefaultGeometry uses width=72, height=40, x_offset=28, and y_offset=24. If it differs, update the file to match CLAUDE.md."

Fix UI Offset: claude -p "Analyze the SSD1306 initialization in components/ssd1306/src/. Verify that the full RAM width is set to 128 (X: 0-127) with active window 72 while the active window is 72x40."

Deep Symbol Search: claude -p "Use grep -r to find where [SYMBOL_NAME] is defined. Start in components/ and main/ directories."

📝 Coding Standards

Style: LLVM/Google C++ (refer to .clang-format)

Error Handling: Use ESP_ERROR_CHECK() for all esp_err_t returns.

Logging: Use ESP_LOGI(TAG, ...) instead of printf.
