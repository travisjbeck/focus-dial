#pragma once

// Brand color definitions
#define BRAND_COLOR_PRIMARY 0xC2E189  // Primary brand color #c2e189 (light green)
#define BRAND_COLOR_PRIMARY_RGB 0xC2E189
#define BRAND_COLOR_PRIMARY_R 0xC2
#define BRAND_COLOR_PRIMARY_G 0xE1
#define BRAND_COLOR_PRIMARY_B 0x89

// Scaled brand colors for NeoPixel operation (40% intensity)
#define BRAND_COLOR_SCALED 0x4D5822  // 40% of primary color
#define BRAND_COLOR_SCALED_R 0x4D    // 40% of 0xC2 = 77
#define BRAND_COLOR_SCALED_G 0x58    // 40% of 0xE1 = 88
#define BRAND_COLOR_SCALED_B 0x22    // 40% of 0x89 = 34

// LVGL color version
#define BRAND_COLOR_PRIMARY_LV lv_color_hex(0xC2E189)

// Other brand colors can be added here as needed