#include "libraries/pico_graphics/pico_graphics.hpp"
#include "libraries/pico_vector/pico_vector.hpp"
#include "drivers/st7701/st7701.hpp"

#include "ff.h"

#include "pico/multicore.h"
#include "pico/sync.h"

#include "hardware/dma.h"

#include "../sparkfun_pico/sfe_pico.h"

#include <cstdlib>
#define CLAY_IMPLEMENTATION
#include "clay.h"

Clay_LayoutConfig layoutElement = Clay_LayoutConfig{.padding = {5}};

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

ST7701 *presto;
PicoGraphics_PenRGB565 *display;
PicoVector *vector;

void Clay_PicoGraphics_PenRGB565_Render(Clay_RenderCommandArray renderCommands, PicoGraphics_PenRGB565 *display)
{
  for (int j = 0; j < renderCommands.length; j++)
  {
    Clay_RenderCommand *renderCommand = Clay_RenderCommandArray_Get(&renderCommands, j);
    Clay_BoundingBox boundingBox = renderCommand->boundingBox;
    switch (renderCommand->commandType)
    {
    case CLAY_RENDER_COMMAND_TYPE_TEXT:
    {
      // Raylib uses standard C strings so isn't compatible with cheap slices, we need to clone the string to append null terminator
      Clay_String text = renderCommand->text;
      char *cloned = (char *)malloc(text.length + 1);
      memcpy(cloned, text.chars, text.length);
      cloned[text.length] = '\0';
      display->set_pen(renderCommand->config.textElementConfig->textColor.r, renderCommand->config.textElementConfig->textColor.g, renderCommand->config.textElementConfig->textColor.b);
      display->text(cloned, (Point){static_cast<int32_t>(boundingBox.x), static_cast<int32_t>(boundingBox.y)}, 0, 1, 0, renderCommand->config.textElementConfig->letterSpacing, false);
      free(cloned);
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_IMAGE:
    {
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
    {
      display->set_clip((Rect){static_cast<int32_t>(boundingBox.x), static_cast<int32_t>(boundingBox.y), static_cast<int32_t>(boundingBox.width), static_cast<int32_t>(boundingBox.height)});
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
    {
      display->remove_clip();
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
    {
      Clay_RectangleElementConfig *config = renderCommand->config.rectangleElementConfig;
      display->set_pen(config->color.r, config->color.g, config->color.b);
      display->rectangle((Rect){static_cast<int32_t>(boundingBox.x), static_cast<int32_t>(boundingBox.y), static_cast<int32_t>(boundingBox.width), static_cast<int32_t>(boundingBox.height)});
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_BORDER:
    {
      Clay_BorderElementConfig *config = renderCommand->config.borderElementConfig;
      // Left border
      if (config->left.width > 0)
      {
        display->set_pen(config->left.color.r, config->left.color.g, config->left.color.b);
        display->rectangle((Rect){static_cast<int32_t>(boundingBox.x), static_cast<int32_t>(boundingBox.y + config->cornerRadius.topLeft), static_cast<int32_t>(config->left.width), static_cast<int32_t>(boundingBox.height - config->cornerRadius.topLeft - config->cornerRadius.bottomLeft)});
      }
      // Right border
      if (config->right.width > 0)
      {
        display->set_pen(config->right.color.r, config->right.color.g, config->right.color.b);
        display->rectangle((Rect){static_cast<int32_t>(boundingBox.x + boundingBox.width - config->right.width), static_cast<int32_t>(boundingBox.y + config->cornerRadius.topRight), static_cast<int32_t>(config->right.width), static_cast<int32_t>(boundingBox.height - config->cornerRadius.topRight - config->cornerRadius.bottomRight)});
      }
      // Top border
      if (config->top.width > 0)
      {
        display->set_pen(config->top.color.r, config->top.color.g, config->top.color.b);
        display->rectangle((Rect){static_cast<int32_t>(boundingBox.x + config->cornerRadius.topLeft), static_cast<int32_t>(boundingBox.y), static_cast<int32_t>(boundingBox.width - config->cornerRadius.topLeft - config->cornerRadius.topRight), static_cast<int32_t>(config->top.width)});
      }
      // Bottom border
      if (config->bottom.width > 0)
      {
        display->set_pen(config->bottom.color.r, config->bottom.color.g, config->bottom.color.b);
        display->rectangle((Rect){static_cast<int32_t>(boundingBox.x + config->cornerRadius.bottomLeft), static_cast<int32_t>(boundingBox.y + boundingBox.height - config->bottom.width), static_cast<int32_t>(boundingBox.width - config->cornerRadius.bottomLeft - config->cornerRadius.bottomRight), static_cast<int32_t>(config->bottom.width)});
      }
      if (config->cornerRadius.topLeft > 0)
      {
        display->set_pen(config->top.color.r, config->top.color.g, config->top.color.b);
        display->rectangle((Rect){static_cast<int32_t>(boundingBox.x), static_cast<int32_t>(boundingBox.y), static_cast<int32_t>(config->cornerRadius.topLeft), static_cast<int32_t>(config->top.width)});
      }
      if (config->cornerRadius.topRight > 0)
      {
        display->set_pen(config->top.color.r, config->top.color.g, config->top.color.b);
        display->rectangle((Rect){static_cast<int32_t>(boundingBox.x + boundingBox.width - config->cornerRadius.topRight), static_cast<int32_t>(boundingBox.y), static_cast<int32_t>(config->cornerRadius.topRight), static_cast<int32_t>(config->top.width)});
      }
      if (config->cornerRadius.bottomLeft > 0)
      {
        display->set_pen(config->bottom.color.r, config->bottom.color.g, config->bottom.color.b);
        display->rectangle((Rect){static_cast<int32_t>(boundingBox.x), static_cast<int32_t>(boundingBox.y + boundingBox.height - config->cornerRadius.bottomLeft), static_cast<int32_t>(config->cornerRadius.bottomLeft), static_cast<int32_t>(config->bottom.width)});
      }
      if (config->cornerRadius.bottomRight > 0)
      {
        display->set_pen(config->bottom.color.r, config->bottom.color.g, config->bottom.color.b);
        display->rectangle((Rect){static_cast<int32_t>(boundingBox.x + boundingBox.width - config->cornerRadius.bottomRight), static_cast<int32_t>(boundingBox.y + boundingBox.height - config->cornerRadius.bottomRight), static_cast<int32_t>(config->cornerRadius.bottomRight), static_cast<int32_t>(config->bottom.width)});
      }
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
    {
      break;
    }
    default:
    {
      printf("Error: unhandled render command.");
      exit(1);
    }
    }
  }
}

int main()
{
  set_sys_clock_khz(240000, true);
  stdio_init_all();
  sfe_pico_alloc_init();

  uint64_t totalMemorySize = Clay_MinMemorySize();
  Clay_Arena clayMemory = Clay_Arena{.label = CLAY_STRING("Clay Memory Arena"), .capacity = totalMemorySize, .memory = (char *)malloc(totalMemorySize)};
  Clay_Initialize(clayMemory, Clay_Dimensions{FRAME_WIDTH, FRAME_HEIGHT});

  gpio_init(LCD_CS);
  gpio_put(LCD_CS, 1);
  gpio_set_dir(LCD_CS, 1);

  sleep_ms(5000);

  printf("Hello\n");

  presto = new ST7701(FRAME_WIDTH, FRAME_HEIGHT, ROTATE_0, SPIPins{spi1, LCD_CS, LCD_CLK, LCD_DAT, PIN_UNUSED, LCD_DC, BACKLIGHT}, back_buffer);
  display = new PicoGraphics_PenRGB565(FRAME_WIDTH, FRAME_HEIGHT, front_buffer);

  printf("Init: ...");
  presto->init();
  printf("Init: OK\n");

  sleep_ms(1000);

  while (true)
  {
    Clay_BeginLayout();
    CLAY(CLAY_RECTANGLE({.color = {255, 255, 255, 0}}), CLAY_LAYOUT(layoutElement)) {}
    Clay_RenderCommandArray renderCommands = Clay_EndLayout();

    display->set_pen(0);
    display->clear();
    Clay_PicoGraphics_PenRGB565_Render(renderCommands, display);
    presto->update(display);
  }
}