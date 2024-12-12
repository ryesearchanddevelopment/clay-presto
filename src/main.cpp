#include "libraries/pico_graphics/pico_graphics.hpp"
#include "libraries/pico_vector/pico_vector.hpp"
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

#define FRAME_WIDTH 240
#define FRAME_HEIGHT 240
static const uint BACKLIGHT = 45;
static const uint LCD_CLK = 26;
static const uint LCD_CS = 28;
static const uint LCD_DAT = 27;
static const uint LCD_DC = -1;
static const uint LCD_D0 = 1;

uint16_t back_buffer[FRAME_WIDTH * FRAME_HEIGHT];
uint16_t front_buffer[FRAME_WIDTH * FRAME_HEIGHT];

ST7701* presto;
PicoGraphics_PenRGB565* display;
PicoVector *vector;


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

    printf("SD card mounted!");

    presto = new ST7701(FRAME_WIDTH, FRAME_HEIGHT, ROTATE_0, SPIPins{spi1, LCD_CS, LCD_CLK, LCD_DAT, PIN_UNUSED, LCD_DC, BACKLIGHT}, back_buffer);
    display = new PicoGraphics_PenRGB565(FRAME_WIDTH, FRAME_HEIGHT, front_buffer);
    vector = new PicoVector(display);

    printf("Init: ...");
    presto->init();
    printf("Init: OK\n");

    printf("Font: ...");
    vector->set_font("/marker-high.af", 30);
    printf("Font: OK\n");

    pp_mat3_t t = pp_mat3_identity();
    pp_mat3_translate(&t, 50, 50);

    while (true) {
        display->set_pen(0);
        display->clear();
        display->set_pen(65535);
        display->rectangle({20, 20, 200, 200});
        display->set_pen(0);
        vector->text("Hello World, how are you today?", 240, 240, &t);
        display->text("Hello", {30, 30}, 240);
        presto->update(display);
    }
}