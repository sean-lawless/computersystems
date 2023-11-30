/*...................................................................*/
/*                                                                   */
/*   Module:  framebuffer.c                                          */
/*   Version: 2018.0                                                 */
/*   Purpose: Raspberry Pi framebuffer                               */
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
/*                                                                   */
// Thank you to the following sources:                               */
/*                                                                   */
/* github.com/raspberrypi/firmware/wiki/Mailbox-property-interface   */
/* www.abbeycat.info/2017/03/06/the-raspberry-pis-vidiocore-iv/      */
/*...................................................................*/
#include <stdio.h>
#include <string.h>
#include <system.h>
#include <board.h>

#if ENABLE_VIDEO

#define MAX_FRAMES 1

typedef struct VideoCoreFrameBuffer
{
  u32 width;      // Physical width of display in pixel
  u32 height;     // Physical height of display in pixel
  u32 virtWidth;  // same as physical width
  u32 virtHeight; // same as physical height
  u32 depth;      // Number of bits per pixel (bpp)
  u32 bufferAddr; // Address of frame buffer
  u32 bufferSize; // Size of frame buffer
}
VideoCoreFrameBuffer;

typedef struct
{
  u8 align[16];
  VideoCoreFrameBuffer vcfb;
} Frame;
Frame Frames[MAX_FRAMES];

int FrameIndex = 0;
int FrameBufferIndex;

/*...................................................................*/
/* FrameBufferInit: Initialize the frame buffer interface            */
/*                                                                   */
/*...................................................................*/
void FrameBufferInit(void)
{
  FrameIndex = 0;
  FrameBufferIndex = 0;
}

/*...................................................................*/
/* FrameBufferNew: Allocate a new framebuffer structure              */
/*                                                                   */
/*     Return: pointer to framebuffer on success, NULL on error      */
/*...................................................................*/
void *FrameBufferNew(void)
{
  void *frame;

  // Return error (NULL) if no more frames available
  if (FrameIndex >= MAX_FRAMES)
    return NULL;

  // Zero out the frame
  bzero(&Frames[FrameIndex], sizeof(Frame));

  // 16 byte bus align the frame pointer
  frame = (void *)(((uintptr_t)&Frames[FrameIndex]) +
                        (16 - (((uintptr_t)&Frames[FrameIndex]) & 15)));

  // Increment frame index and return aligned frame pointer
  FrameIndex++;
  return frame;
}

/*...................................................................*/
/* FrameBufferClose: Close/Deallocate a framebuffer                  */
/*                                                                   */
/*      Input: fb is a pointer to the framebuffer                    */
/*                                                                   */
/*...................................................................*/
void FrameBufferClose(void *fb)
{
  VideoCoreFrameBuffer *frame = fb;

  bzero(frame, sizeof(VideoCoreFrameBuffer));
}

/*...................................................................*/
/* FrameBufferOpen: Open the framebuffer                             */
/*                                                                   */
/*      Input: fb is a pointer to the framebuffer                    */
/*             width is the screen width or X                        */
/*             height is the screen height or Y                      */
/*             depth is the color depth in bpp                       */
/*                                                                   */
/*...................................................................*/
void FrameBufferOpen(void *fb, int width, int height, u32 depth)
{
  VideoCoreFrameBuffer *frame = fb;

  frame->width = width;
  frame->height = height;
  frame->virtWidth  = width;
  frame->virtHeight = height;
  frame->depth      = depth;
  frame->bufferAddr = 0;
  frame->bufferSize = 0;
}

/*...................................................................*/
/* FrameBufferInitialize: Initialize the framebuffer                 */
/*                                                                   */
/*      Input: fb is a pointer to the framebuffer                    */
/*                                                                   */
/*     Return: zero (0) on success, negative value on error          */
/*...................................................................*/
int FrameBufferInitialize(void *fb)
{
  VideoCoreFrameBuffer *frame = fb;

  // Allocate framebuffer through mailbox to GPU
  frame->bufferSize = SetDisplayResolution(&frame->width,
                    &frame->height, &frame->depth, &frame->bufferAddr);

  // Ensure framebuffer is valid, return error if not
  if ((frame->bufferSize == 0) || (frame->bufferAddr == 0))
  {
    puts("Framebuffer create/set resolution failed");
    return -1;
  }

  // Return success
  return 0;
}

/*...................................................................*/
/* FrameBufferGetWidth: Return the width of the framebuffer          */
/*                                                                   */
/*      Input: fb is a pointer to the framebuffer                    */
/*                                                                   */
/*     Return: Width in pixels                                       */
/*...................................................................*/
u32 FrameBufferGetWidth(void *fb)
{
  VideoCoreFrameBuffer *frame = fb;

  return frame->width;
}

/*...................................................................*/
/* FrameBufferGetHeight: Return the height of the framebuffer        */
/*                                                                   */
/*      Input: fb is a pointer to the framebuffer                    */
/*                                                                   */
/*     Return: Height in pixels                                      */
/*...................................................................*/
u32 FrameBufferGetHeight(void *fb)
{
  VideoCoreFrameBuffer *frame = fb;

  return frame->height;
}

/*...................................................................*/
/* FrameBufferGetDepth: Return the color depth of the framebuffer    */
/*                                                                   */
/*      Input: fb is a pointer to the framebuffer                    */
/*                                                                   */
/*     Return: Depth in bits per pixel (bpp)                         */
/*...................................................................*/
u32 FrameBufferGetDepth(void *fb)
{
  VideoCoreFrameBuffer *frame = fb;

  return frame->depth;
}

/*...................................................................*/
/* FrameBufferGetBuffer: Return ARM relative RAM address of buffer   */
/*                                                                   */
/*      Input: fb is a pointer to the framebuffer                    */
/*                                                                   */
/*     Return: Framebuffer memory address (ie pointer to framebuffer)*/
/*...................................................................*/
u32 FrameBufferGetBuffer(void *fb)
{
  VideoCoreFrameBuffer *frame = fb;

  return frame->bufferAddr & ~GPU_MEM_BASE;
}

/*...................................................................*/
/* FrameBufferGetSize: Return size of framebuffer                    */
/*                                                                   */
/*      Input: fb is a pointer to the framebuffer                    */
/*                                                                   */
/*     Return: Framebuffer length in bytes                           */
/*...................................................................*/
u32 FrameBufferGetSize(void *fb)
{
  VideoCoreFrameBuffer *frame = fb;

  return frame->bufferSize;
}

#endif /* ENABLE_VIDEO */
