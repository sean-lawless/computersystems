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

  // Return success
  return 0;
}

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* ScreenInit: Initialize the video screen                           */
/*                                                                   */
/*   Return: Zero (0) on success, negative if error                  */
/*...................................................................*/
int ScreenInit(void)
{
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
}

/*...................................................................*/
/* ScreenClear: Clear the video screen display                       */
/*                                                                   */
/*...................................................................*/
void ScreenClear(void)
{
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

#endif /* ENABLE_VIDEO */
