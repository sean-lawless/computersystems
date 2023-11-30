/*...................................................................*/
/*                                                                   */
/*   Module:  configure.h                                            */
/*   Version: 2015.0                                                 */
/*   Purpose: system configuration declarations                      */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2015, Sean Lawless                    */
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
#ifndef _CONFIGURE_H
#define _CONFIGURE_H

// Application configuration
#define ENABLE_GAME        (TRUE && ENABLE_VIDEO)

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define ENABLE_OS          TRUE
#define ENABLE_SHELL       TRUE

#define ENABLE_UART0       TRUE  /* enable primary UART */
#define ENABLE_UART1       FALSE /* enable secondary UART */
#define ENABLE_JTAG        TRUE  /* enable JTAG debugging */
#define ENABLE_VIDEO       TRUE  /* enable video console */
#define   PIXEL_WIDTH           640 /* zero to auto-detect */
#define   PIXEL_HEIGHT          480 /* zero to auto-detect */
#define   COLOR_DEPTH_BITS      16  /* color depth in bits, 32 or 16 */
#define   CONSOLE_X_DIVISOR     4   /* fractional screen, 1 is full */
#define   CONSOLE_Y_DIVISOR     1   /* fractional screen, 1 is full */
#define   CONSOLE_X_ORIENTATION 0   /* in number of characters */
#define   CONSOLE_Y_ORIENTATION 10  /* in number of lines */
#define ENABLE_USB         TRUE  /* enable USB host */
#define ENABLE_XMODEM      TRUE  /* enable xmodem */
#define ENABLE_BOOTLOADER  FALSE /* enable boot loader */
#define   MAX_BOOT_LENGTH  (1024 * 1024 * 16) /* 16 MB boot image max */
#define ENABLE_PRINTF      TRUE  /* printf arguments */
#define ENABLE_ASSERT      (TRUE && ENABLE_PRINTF)/*enable assertions */
#define ENABLE_AUTO_START  FALSE /* Auto start enabled devices */

/* USB Specific configuration */
#define ENABLE_USB_TASK    (FALSE && ENABLE_USB)/* USB intr task */

/* DO NOT EDIT BELOW : Derived configurations */
#define MAX_TASKS          (10 + ENABLE_UART0 + ENABLE_UART1 + \
                            ENABLE_VIDEO + ENABLE_USB_TASK)

#endif /* _CONFIGURE_H */
