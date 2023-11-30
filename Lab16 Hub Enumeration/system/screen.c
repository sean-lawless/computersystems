/*...................................................................*/
/*                                                                   */
/*   Module:  screen.c                                               */
/*   Version: 2018.0                                                 */
/*   Purpose: Interface video screen to framebuffer                  */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2018, Sean Lawless                    */
/*                                                                   */
/*                      ALL RIGHTS RESERVED                          */
/*                                                                   */
/* Redistribution and use in source, binary or derived forms, with   */
/* or without modification, are permitted provided that the          */
/* following conditions are met:                                     */
/*                                                                   */
/*  1. Redistributions in any form, including but not limited to     */
/*     source code, binary, or derived works, must include the above */
/*     copyright notice, this list of conditions and the following   */
/*     disclaimer.                                                   */
/*                                                                   */
/*  2. Any change or addition to this copyright notice requires the  */
/*     prior written permission of the above copyright holder.       */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED ''AS IS''. ANY EXPRESS OR IMPLIED       */
/* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES */
/* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE       */
/* DISCLAIMED. IN NO EVENT SHALL ANY AUTHOR AND/OR COPYRIGHT HOLDER  */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/*...................................................................*/
#include <configure.h>
#include <system.h>
#include <board.h>
#include <string.h>
#include <stdio.h>

#if ENABLE_VIDEO

/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
typedef struct
{
  u32 initWidth;
  u32 initHeight;
  void *frameBuffer;
  ScreenColor *buffer;
  u32 size;
  u32 width;
  u32 height;

  u32 cursorX;
  u32 cursorY;
  u32 cursorOffsetX;
  u32 cursorOffsetY;
  u32 cursorHeight;
  u32 cursorWidth;
#if USE_SCREEN_TEXT
  char screenText[SCREEN_TEXT_HEIGHT][SCREEN_TEXT_WIDTH];
#endif
} ScreenDevice;

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
ScreenDevice TheScreen;

/*...................................................................*/
/* Local Static Functions                                            */
/*...................................................................*/

/*...................................................................*/
/* set_pixel: Set an individual pixel to a color                     */
/*                                                                   */
/*   Input: screen is the screen device to set the pixel             */
/*          x is the X position (width) of the pixel                 */
/*          y is the Y position (height) of the pixel                */
/*          color is the 32 bit RGBA color of the pixel              */
/*...................................................................*/
static void set_pixel(ScreenDevice *screen, u32 x, u32 y,
                      u32 color)
{
#if COLOR_DEPTH_BITS == 32
  ScreenColor screenColor = color;
#else
  ScreenColor screenColor;

  // Convert 32 bpp RGBA color to 16 bpp RGB color
  //   Isolate and divide each 8 bit color value by 4 to fit in 5 bits
  screenColor = COLOR16((((color >> 16) & 0xFF) / 4),// Red
                        (((color >> 8) & 0xFF) / 4), // Green
                        (((color) & 0xFF) / 4));     // Blue
#endif

  // If a valid x and y, assign the color to the framebuffer
  if ((x < screen->width) && (y < screen->height))
    screen->buffer[(FrameBufferGetWidth(screen->frameBuffer) * y) + x]=
                                                           screenColor;
}

/*...................................................................*/
/* clear_display: clear the entire screen display                    */
/*                                                                   */
/*   Input: screen is pointer to the video screen                    */
/*...................................................................*/
static void clear_display(ScreenDevice *screen)
{
  unsigned size;
  ScreenColor *buffer;

  buffer = screen->buffer;
  size = screen->size / sizeof(ScreenColor);

  while (size--)
    *buffer++ = COLOR_BLACK;
}

/*...................................................................*/
/* screen_init: Initialize the video screen                          */
/*                                                                   */
/*   Input: screen is pointer to the video screen                    */
/*          width is the desired pixel width                         */
/*          height is the desired pixel height                       */
/*...................................................................*/
static void screen_init(ScreenDevice *screen, u32 width, u32 height)
{
  screen->initWidth = width;
  screen->initHeight = height;
  screen->frameBuffer = 0;
  screen->buffer = 0;
  screen->cursorX = CONSOLE_X_ORIENTATION * CharacterWidth();
  screen->cursorY = CONSOLE_Y_ORIENTATION * CharacterHeight();
  screen->cursorOffsetX = screen->cursorX;
  screen->cursorOffsetY = screen->cursorY;
  screen->cursorWidth = width / CONSOLE_X_DIVISOR;
  screen->cursorHeight = height / CONSOLE_Y_DIVISOR;

  // Check cursor console sizes and trim to screen size if necessary
  if (screen->cursorOffsetX + screen->cursorWidth > screen->initWidth)
    screen->cursorWidth = screen->initWidth - screen->cursorOffsetX;
  if (screen->cursorOffsetY + screen->cursorHeight >screen->initHeight)
    screen->cursorHeight = screen->initHeight - screen->cursorOffsetY;

#if USE_SCREEN_TEXT
  bzero(screen->screenText, SCREEN_TEXT_WIDTH * SCREEN_TEXT_HEIGHT);
#endif
}

/*...................................................................*/
/* screen_close: Close the video screen                              */
/*                                                                   */
/*   Input: screen is pointer to the video screen                    */
/*...................................................................*/
static void screen_close(ScreenDevice *screen)
{
  screen->buffer = 0;
  FrameBufferClose(screen->frameBuffer);
  screen->frameBuffer = 0;
}

/*...................................................................*/
/* screen_device_init: Initialize the video screen                   */
/*                                                                   */
/*   Input: screen is pointer to the video screen                    */
/*                                                                   */
/*  Return: Zero (0) on success, negative on error                   */
/*...................................................................*/
static int screen_device_init(ScreenDevice *screen)
{
  // Allocate a new framebuffer if not already assigned
  if (!screen->frameBuffer)
    screen->frameBuffer = FrameBufferNew();
  if (!screen->frameBuffer)
  {
    puts("Frame buffer new failed");
    return -1;
  }

  // Open and initialize the framebuffer
  FrameBufferOpen(screen->frameBuffer, screen->initWidth,
                  screen->initHeight, COLOR_DEPTH_BITS);
  if (FrameBufferInitialize(screen->frameBuffer))
  {
    puts("FrameBufferInitialize(FrameBuffer) failed");
    return -1;
  }

  // Ensure color depth matches build parameters
  if (FrameBufferGetDepth(screen->frameBuffer) != COLOR_DEPTH_BITS)
  {
    puts("FrameBufferGetDepth(FrameBuffer) failed");
    return -1;
  }

  // Polpulate the screen from the resulting framebuffer
  screen->buffer = (ScreenColor *)FrameBufferGetBuffer(
                                                  screen->frameBuffer);
  screen->size   = FrameBufferGetSize(screen->frameBuffer);
  screen->width  = FrameBufferGetWidth(screen->frameBuffer);
  screen->height = FrameBufferGetHeight(screen->frameBuffer);
  screen->cursorWidth = screen->width / CONSOLE_X_DIVISOR;
  screen->cursorHeight = screen->height / CONSOLE_Y_DIVISOR;

  // Return success
  return 0;
}

/*...................................................................*/
/*  scroll: Scroll the video cursor up one character line            */
/*                                                                   */
/*   Input: screen is pointer to the video screen                    */
/*...................................................................*/
static void scroll(ScreenDevice *screen)
{
  u32 lines = CharacterHeight();
  ScreenColor *to, *from = NULL;
  int cursorY;
  u32 size = screen->cursorWidth * sizeof(ScreenColor);
#if USE_SCREEN_TEXT
  u32 x, y;
  int length, i;
  char *current;
  char *next;
#endif

  u32 scroll_time = TimerNow();

  // Move the cursor up one line
  screen->cursorY -= CharacterHeight();

  // Move every line of screen output up one cursor height
  for (cursorY = screen->cursorOffsetY; cursorY < screen->cursorY;
       cursorY += CharacterHeight())
  {
#if USE_SCREEN_TEXT
    current = screen->screenText[cursorY / CharacterHeight()];
    next = current + SCREEN_TEXT_WIDTH;

    // Clear the current line
    y = CONSOLE_Y_ORIENTATION + cursorY;
    for (i = 0, length = strlen(current); i < length; ++i)
    {
      x = CONSOLE_X_ORIENTATION + (i * CharacterWidth());
      display_char(current[i], x, y, COLOR_BLACK);
    }

    // Move to next line up the screen
    for (i = 0, length = strlen(next); i < length; ++i)
    {
      x = CONSOLE_X_ORIENTATION + (i * CharacterWidth());
      display_char(next[i], x, y, NORMAL_COLOR);
      current[i] = next[i];
    }
    current[i] = '\0';

#else
    for (lines = 0; lines < CharacterHeight(); ++lines)
    {
      to = (screen->buffer + CONSOLE_X_ORIENTATION +(lines + cursorY) *
                    FrameBufferGetWidth(screen->frameBuffer));
      from = (screen->buffer + CONSOLE_X_ORIENTATION +
              ((lines + cursorY + CharacterHeight()) *
               FrameBufferGetWidth(screen->frameBuffer)));
      memcpy(to, from, size);
    }
#endif
  }

#if USE_SCREEN_TEXT
  // Clear the current line
  y = CONSOLE_Y_ORIENTATION + screen->cursorY;
  for (i = 0, length = strlen(next); i < length; ++i)
  {
    x = CONSOLE_X_ORIENTATION + (i * CharacterWidth());
    display_char(next[i], x, y, COLOR_BLACK);
    next[i] = '\0';
  }
#else
  // Erase last copied line
  if (from)
  {
    for (lines = 0; lines < CharacterHeight(); ++lines)
    {
      from = (screen->buffer + CONSOLE_X_ORIENTATION +
              ((lines + screen->cursorY) *
               FrameBufferGetWidth(screen->frameBuffer)));
      size = (screen->cursorWidth * sizeof(ScreenColor));

      if (((u32)from - (u32)screen->buffer) + (size * sizeof(u32)) >
          FrameBufferGetSize(screen->frameBuffer))
      {
        puts("Framebuffer overflow detected, scroll aborted.");
        putbyte(lines);
        return;
      }

      // Set all framebuffer data to zero (black)
      memset(from, COLOR_BLACK, size);
    }
  }
#endif

  scroll_time = TimerNow() - scroll_time;

//  putbyte((u8)(scroll_time >> 24));
//  putbyte((u8)(scroll_time >> 16));
//  putbyte((u8)(scroll_time >> 8));
//  putbyte((u8)scroll_time);
}

/*...................................................................*/
/* carriage_return: Left justify the cursor                          */
/*                                                                   */
/*...................................................................*/
static void carriage_return(void)
{
  // Set the cursor x position to the starting offset
  TheScreen.cursorX = TheScreen.cursorOffsetX;
}

/*...................................................................*/
/* carriage_down: Move cursor down one line                          */
/*                                                                   */
/*...................................................................*/
static void cursor_down(void)
{
  TheScreen.cursorY += CharacterHeight();

  // Scroll screen if not enough room
  if (TheScreen.cursorY + CharacterHeight() >= TheScreen.cursorHeight)
    scroll(&TheScreen);
}

/*...................................................................*/
/* new_line: Left justify and move cursor down one line              */
/*                                                                   */
/*...................................................................*/
static void new_line()
{
  carriage_return();
  cursor_down();
}

/*...................................................................*/
/* cursor_left: Move cursor left one character                       */
/*                                                                   */
/*...................................................................*/
static void cursor_left()
{
  // If room, move back one character
  if (TheScreen.cursorX >= CharacterWidth())
  {
    TheScreen.cursorX -= CharacterWidth();
  }

  // Otherwise move to right edge and up one line
  else
  {
    if (TheScreen.cursorY >= CharacterHeight())
    {
      TheScreen.cursorX = TheScreen.cursorWidth - CharacterWidth();
      TheScreen.cursorY -= CharacterHeight();
    }
  }
}

/*...................................................................*/
/* cursor_right: Move cursor right one character                     */
/*                                                                   */
/*...................................................................*/
static void cursor_right()
{
  TheScreen.cursorX += CharacterWidth();
  if (TheScreen.cursorX >= TheScreen.cursorWidth)
  {
    new_line();
  }
}

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* DisplayCursorChar: Display a character on the screen              */
/*                                                                   */
/*   Input: ascii is the character to display                        */
/*          x is X position (width) of the starting pixel            */
/*          y is Y position (height) of the starting pixel           */
/*          color is 32 bit RGBA color of the character to display   */
/*...................................................................*/
void DisplayCursorChar(char ascii, u32 x, u32 y, u32 color)
{
  u32 fontX, fontY;
  // Font map starts with space, so convert ascii to a font offset
  int font_offset = ascii - ' ';

  // Return if invalid ascii conversion
  if (font_offset < 0)
    return;

  // Display the character pixels based on font map
  for (fontY = 0; fontY < CharacterHeight(); fontY++)
  {
    for (fontX = 0; fontX < CharacterWidth(); fontX++)
    {
      // Display the pixel color if character pixel set
      if (CharacterPixel(font_offset, fontX, fontY))
        set_pixel(&TheScreen, x + fontX, y + fontY, color);

      // space is used to clear backspace so set to black
      else if (ascii == ' ')
        set_pixel(&TheScreen, x + fontX, y + fontY, COLOR_BLACK);
    }
  }

  // Update the screen text with this character
#if USE_SCREEN_TEXT
  assert(cursorY / CharacterHeight() <= SCREEN_TEXT_HEIGHT);
  assert(cursorX / CharacterWidth() <= SCREEN_TEXT_WIDTH);
  TheScreen.screenText[cursorY / CharacterHeight()]
                      [cursorX / CharacterWidth()] = ascii;
#endif
}

/*...................................................................*/
/* ScreenInit: Initialize the video screen                           */
/*                                                                   */
/*   Return: Zero (0) on success, negative if error                  */
/*...................................................................*/
int ScreenInit(void)
{
  ScreenUp = FALSE;
  bzero(&TheScreen, sizeof(ScreenDevice));
  FrameBufferInit();

  screen_init(&TheScreen, PIXEL_WIDTH, PIXEL_HEIGHT);
  puts("Initializing HDMI port...");
  if (screen_device_init(&TheScreen))
  {
    puts("failed, no HDMI monitor connected?");
    screen_close(&TheScreen);
    return -1;
  }
  else
  {
    puts("HDMI display detected");
    ScreenUp = TRUE;
  }

  return 0;
}

/*...................................................................*/
/* ScreenClose: Close the video screen                               */
/*                                                                   */
/*...................................................................*/
void ScreenClose(void)
{
  screen_close(&TheScreen);
  bzero(&TheScreen, sizeof(ScreenDevice));
  bzero(&ConsoleState, sizeof(ConsoleState));
}

/*...................................................................*/
/* ScreenClear: Clear the video screen display                       */
/*                                                                   */
/*...................................................................*/
void ScreenClear(void)
{
  if (ScreenUp)
    clear_display(&TheScreen);
}

/*...................................................................*/
/* SetPixel: Set a screen pixel to a color                           */
/*                                                                   */
/*   Input: x is X position (width) of the pixel                     */
/*          y is Y position (height) of the pixel                    */
/*          color is the new 32 bit RGBA color of the pixel          */
/*...................................................................*/
void SetPixel(u32 x, u32 y, u32 color)
{
  set_pixel(&TheScreen, x, y, color);
}

/*...................................................................*/
/* DisplayChar: Display character at screen current cursor           */
/*                                                                   */
/*   Input: ascii of the character to display                        */
/*          color is the new 32 bit RGBA color of the pixel          */
/*...................................................................*/
void DisplayChar(char ascii, u32 color)
{
  // Display only standard ascii characters
  if (' ' <= ascii && ascii <= '~')
  {
    // Display at the current cursor location and then move cursor
    DisplayCursorChar(ascii, TheScreen.cursorX, TheScreen.cursorY,
                      color);
    cursor_right();
  }
}

/*...................................................................*/
/* DisplayCharacter: Process character to display to video screen    */
/*                                                                   */
/*   Input: ascii of the character to display                        */
/*          color is the new 32 bit RGBA color of the pixel          */
/*...................................................................*/
void DisplayCharacter(char ch, u32 color)
{
    switch(ch)
    {
      case '\b':
        cursor_left();
        break;

      case '\n':
        new_line();
        break;

      case '\r':
        carriage_return();
        break;

      default:
        DisplayChar(ch, color);
        break;
    }
}

/*...................................................................*/
/* DisplayString: Process string to display to video screen          */
/*                                                                   */
/*   Input: string of the string to display                          */
/*          length is the length of the string                       */
/*          color is the color to display the string as              */
/*                                                                   */
/*  Return: length of the string displayed                           */
/*...................................................................*/
int DisplayString(const char *string, int length, u32 color)
{
  int result = 0;

  // Loop until there is no more length in the string
  for (;length; --length)
  {
    // Return length displayed if string NULL character is found
    if (*string == '\0')
      return result;

    // Display the character and move to the next in string
    DisplayCharacter(*string++, color);
    result++;
  }

  // Return the amount of characters displayed
  return result;
}

#endif /* ENABLE_VIDEO */
