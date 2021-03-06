//
// XXX
//
// simply copied from  https://github.com/afiskon/stm32-ssd1306.git
//
//
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "ssd1306.h"
#include "event_dispatcher.h"
#include "event_list.h"

typedef struct
{
  uint16_t      cx;
  uint16_t      cy;
  bool          inverted;
  bool          initialized;
  bool          display_on;
  bool          dma_in_prog;
} ssd1306_t;

static uint8_t      _frame_buffer[SSD1306_BUFFER_SIZE];
static ssd1306_t    _ssd1306;

static uint8_t      _dma_buffer[SSD1306_BUFFER_SIZE];
static uint8_t      _dma_page;

////////////////////////////////////////////////////////////////////////////////////////////////
//
// SSD1306 I2C interface
//
////////////////////////////////////////////////////////////////////////////////////////////////
static inline void
ssd1306_write_command(uint8_t byte)
{
  HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x00, 1, &byte, 1, HAL_MAX_DELAY);
}

static inline void
ssd1306_write_data(uint8_t* buffer, size_t buff_size)
{
  HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x40, 1, buffer, buff_size, HAL_MAX_DELAY);
}

static inline void
ssd1306_write_page_dma(uint8_t page)
{
  ssd1306_write_command(0xB0 + page);    // Set the current RAM page address.
  ssd1306_write_command(0x00);
  ssd1306_write_command(0x10);

  HAL_I2C_Mem_Write_DMA(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x40, 1, &_dma_buffer[SSD1306_WIDTH * page], SSD1306_WIDTH);
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// I2C DMA complete callback in IRQ context
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
// XXX
// runs in IRQ context
//
void
HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  event_set(1 << DISPATCH_EVENT_I2C_DMA_COMPLETE);
}

//
// XXX
// this runs in user, aka mainloop, context
// actually we really didn't have to do this
// but I really don't like volatile.
// volatile is simply undefinable.
// that is not an engineering.
//
static void
i2c_dma_complete(uint32_t event)
{
  _dma_page++;
  if(_dma_page < SSD1306_HEIGHT / 8)
  {
    ssd1306_write_page_dma(_dma_page);
  }
  else
  {
    _ssd1306.dma_in_prog = false;
    event_set(1 << DISPATCH_EVENT_SCREEN_UPDATE_DONE);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// public interfaces
//
////////////////////////////////////////////////////////////////////////////////////////////////
void ssd1306_init(void)
{
  // Wait for the screen to boot
  HAL_Delay(100);

  ssd1306_set_display_on(false);

  ssd1306_write_command(0x20);        //Set Memory Addressing Mode
  ssd1306_write_command(0x00);        // 00b,Horizontal Addressing Mode; 01b,Vertical Addressing Mode;
                                      // 10b,Page Addressing Mode (RESET); 11b,Invalid

  ssd1306_write_command(0xB0);        //Set Page Start Address for Page Addressing Mode,0-7

#ifdef SSD1306_MIRROR_VERT
  ssd1306_write_command(0xC0);        // Mirror vertically
#else
  ssd1306_write_command(0xC8);        //Set COM Output Scan Direction
#endif

  ssd1306_write_command(0x00);        //---set low column address
  ssd1306_write_command(0x10);        //---set high column address

  ssd1306_write_command(0x40);        //--set start line address - CHECK

  ssd1306_set_contrast(0xFF);

#ifdef SSD1306_MIRROR_HORIZ
  ssd1306_write_command(0xA0);        // Mirror horizontally
#else
  ssd1306_write_command(0xA1);        //--set segment re-map 0 to 127 - CHECK
#endif

#ifdef SSD1306_INVERSE_COLOR
  ssd1306_write_command(0xA7);        //--set inverse color
#else
  ssd1306_write_command(0xA6);        //--set normal color
#endif

  // Set multiplex ratio.
#if (SSD1306_HEIGHT == 128)
  // Found in the Luma Python lib for SH1106.
  ssd1306_write_command(0xFF);
#else
  ssd1306_write_command(0xA8);        //--set multiplex ratio(1 to 64) - CHECK
#endif

#if (SSD1306_HEIGHT == 32)
  ssd1306_write_command(0x1F); 
#elif (SSD1306_HEIGHT == 64)
  ssd1306_write_command(0x3F);
#elif (SSD1306_HEIGHT == 128)
  ssd1306_write_command(0x3F);        // Seems to work for 128px high displays too.
#else
#error "Only 32, 64, or 128 lines of height are supported!"
#endif

  ssd1306_write_command(0xA4);        //0xa4,Output follows RAM content;0xa5,Output ignores RAM content

  ssd1306_write_command(0xD3);        //-set display offset - CHECK
  ssd1306_write_command(0x00);        //-not offset

  ssd1306_write_command(0xD5);        //--set display clock divide ratio/oscillator frequency
  ssd1306_write_command(0xF0);        //--set divide ratio

  ssd1306_write_command(0xD9);        //--set pre-charge period
  ssd1306_write_command(0x22); 

  ssd1306_write_command(0xDA);        //--set com pins hardware configuration - CHECK
#if (SSD1306_HEIGHT == 32)
  ssd1306_write_command(0x02);
#elif (SSD1306_HEIGHT == 64)
  ssd1306_write_command(0x12);
#elif (SSD1306_HEIGHT == 128)
  ssd1306_write_command(0x12);
#else
#error "Only 32, 64, or 128 lines of height are supported!"
#endif

  ssd1306_write_command(0xDB);        //--set vcomh
  ssd1306_write_command(0x20);        //0x20,0.77xVcc

  ssd1306_write_command(0x8D);        //--set DC-DC enable
  ssd1306_write_command(0x14);
  ssd1306_set_display_on(true);       //--turn on SSD1306 panel

  // Clear screen
  ssd1306_fill(ssd1306_color_black);

  // Flush buffer to screen
  ssd1306_update_screen();

  // Set default values for screen object
  _ssd1306.cx = 0;
  _ssd1306.cy = 0;
  _ssd1306.initialized = true;

  event_register_handler(i2c_dma_complete, DISPATCH_EVENT_I2C_DMA_COMPLETE);
}

void ssd1306_fill(ssd1306_color_t color)
{
  uint32_t i;

  for(i = 0; i < sizeof(_frame_buffer); i++)
  {
    _frame_buffer[i] = (color == ssd1306_color_black) ? 0x00 : 0xFF;
  }
}

void ssd1306_update_screen(void)
{
  if(_ssd1306.dma_in_prog)
  {
    return;
  }

  // Write data to each page of RAM. Number of pages
  // depends on the screen height:
  //
  //  * 32px   ==  4 pages
  //  * 64px   ==  8 pages
  //  * 128px  ==  16 pages

  for(uint8_t i = 0; i < SSD1306_HEIGHT/8; i++)
  {
    ssd1306_write_command(0xB0 + i);    // Set the current RAM page address.
    ssd1306_write_command(0x00);
    ssd1306_write_command(0x10);

    ssd1306_write_data(&_frame_buffer[SSD1306_WIDTH*i], SSD1306_WIDTH);
  }
}

void
ssd1306_update_screen_async(void)
{
  if(_ssd1306.dma_in_prog)
  {
    return;
  }

  memcpy(_dma_buffer, _frame_buffer, sizeof(_dma_buffer));

  _dma_page = 0;
  _ssd1306.dma_in_prog = true;

  ssd1306_write_page_dma(_dma_page);
}

void
ssd1306_set_contrast(const uint8_t value)
{
  const uint8_t kSetContrastControlRegister = 0x81;

  ssd1306_write_command(kSetContrastControlRegister);
  ssd1306_write_command(value);
}

void
ssd1306_set_display_on(const bool on)
{
  uint8_t value;

  if(on)
  {
    value = 0xaf;
    _ssd1306.display_on = true;
  }
  else
  {
    value = 0xae;
    _ssd1306.display_on = false;
  }
  ssd1306_write_command(value);
}

bool
ssd1306_get_display_on(void)
{
  return _ssd1306.display_on;
}

void
ssd1306_set_cursor(uint8_t x, uint8_t y)
{
  _ssd1306.cx = x;
  _ssd1306.cy = y;
}

void
ssd1306_draw_pixel(uint8_t x, uint8_t y, ssd1306_color_t color)
{
  if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
  {
    return;
  }

  if(_ssd1306.inverted)
  {
    color = (ssd1306_color_t)!color;
  }

  if(color == ssd1306_color_white)
  {
    _frame_buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
  } else { 
    _frame_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
  }
}

char
ssd1306_write_char(char ch, ssd1306_font_t font, ssd1306_color_t color)
{
  uint32_t i, b, j;
    
  // Check if character is valid
  if (ch < 32 || ch > 126)
    return 0;
    
  // Check remaining space on current line
  if (SSD1306_WIDTH < (_ssd1306.cx + font.width) ||
      SSD1306_HEIGHT < (_ssd1306.cy + font.height))
  {
    // Not enough space on current line
    return 0;
  }
    
  // Use the font to write
  for(i = 0; i < font.height; i++)
  {
    b = font.data[(ch - 32) * font.height + i];
    for(j = 0; j < font.width; j++)
    {
      if((b << j) & 0x8000)
      {
        ssd1306_draw_pixel(_ssd1306.cx + j, (_ssd1306.cy + i), (ssd1306_color_t) color);
      }
      else
      {
        ssd1306_draw_pixel(_ssd1306.cx + j, (_ssd1306.cy + i), (ssd1306_color_t)!color);
      }
    }
  }
    
  // The current space is now taken
  _ssd1306.cx += font.width;
    
  // Return written char for validation
  return ch;
}

char
ssd1306_write_string(char* str, ssd1306_font_t font, ssd1306_color_t color)
{
  while (*str)
  {
    if (ssd1306_write_char(*str, font, color) != *str)
    {
      // Char could not be written
      return *str;
    }

    // Next char
    str++;
  }

  // Everything ok
  return *str;
}

void
ssd1306_printf(uint8_t x, uint8_t y, ssd1306_font_t font, ssd1306_color_t color, const char* fmt, ...)
{
  static char print_buf[64];
  va_list args;

  ssd1306_set_cursor(x, y);

  va_start(args, fmt);
  vsnprintf(print_buf, 64, fmt, args);
  va_end(args);

  ssd1306_write_string(print_buf, font, color);
}

void
ssd1306_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, ssd1306_color_t color)
{
  int32_t deltaX = abs(x2 - x1);
  int32_t deltaY = abs(y2 - y1);
  int32_t signX = ((x1 < x2) ? 1 : -1);
  int32_t signY = ((y1 < y2) ? 1 : -1);
  int32_t error = deltaX - deltaY;
  int32_t error2;

  ssd1306_draw_pixel(x2, y2, color);

  while((x1 != x2) || (y1 != y2))
  {
    ssd1306_draw_pixel(x1, y1, color);
    error2 = error * 2;
    if(error2 > -deltaY)
    {
      error -= deltaY;
      x1 += signX;
    }
    else
    {
      /*nothing to do*/
    }

    if(error2 < deltaX)
    {
      error += deltaX;
      y1 += signY;
    }
    else
    {
      /*nothing to do*/
    }
  }
  return;
}

void
ssd1306_draw_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, ssd1306_color_t color)
{
  ssd1306_line(x1,y1,x2,y1,color);
  ssd1306_line(x2,y1,x2,y2,color);
  ssd1306_line(x2,y2,x1,y2,color);
  ssd1306_line(x1,y2,x1,y1,color);
  return;
}
