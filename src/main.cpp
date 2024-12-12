#include "libraries/pico_graphics/pico_graphics.hpp"
#include "drivers/st7701/st7701.hpp"

#include "ff.h"

#include "pico/multicore.h"
#include "pico/sync.h"

#include "hardware/dma.h"

using namespace pimoroni;

FATFS fs;
FIL fil;
FIL audio_file;
FRESULT fr;

#define FRAME_WIDTH 480
#define FRAME_HEIGHT 480
static const uint BACKLIGHT = 45;
static const uint LCD_CLK = 26;
static const uint LCD_CS = 28;
static const uint LCD_DAT = 27;
static const uint LCD_DC = -1;
static const uint LCD_D0 = 1;

uint16_t frame_buffer[FRAME_WIDTH * FRAME_HEIGHT];

ST7701* presto;
PicoGraphics_PenRGB565* display;



int main() {
    set_sys_clock_khz(240000, true);
    stdio_init_all();

    gpio_init(LCD_CS);
    gpio_put(LCD_CS, 1);
    gpio_set_dir(LCD_CS, 1);

    sleep_ms(5000);
    printf("Hello\n");

    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
      printf("Failed to mount SD card, error: %d\n", fr);
      return 0;
    }

    presto = new ST7701(FRAME_WIDTH, FRAME_HEIGHT, ROTATE_0, SPIPins{spi1, LCD_CS, LCD_CLK, LCD_DAT, PIN_UNUSED, LCD_DC, BACKLIGHT}, frame_buffer);
    display = new PicoGraphics_PenRGB565(FRAME_WIDTH, FRAME_HEIGHT, frame_buffer);

    printf("Init\n");

    while (true) {
        display->set_pen(0);
        display->clear();
        presto->update(display);
    }
}