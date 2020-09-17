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

#include <system.h>

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define ENABLE_OS          FALSE
#define   MAX_TASKS          (10 + ENABLE_UART0 + ENABLE_UART1 + \
                              ENABLE_VIDEO)
#define ENABLE_SHELL       FALSE
#define ENABLE_UART0       FALSE /* enable primary UART */
#define ENABLE_UART1       FALSE /* enable secondary UART */
#define ENABLE_JTAG        TRUE  /* enable JTAG debugging */
#define ENABLE_VIDEO       FALSE /* enable video console */
#define   PIXEL_WIDTH           0   /* zero to auto-detect */
#define   PIXEL_HEIGHT          0   /* zero to auto-detect */
#define   COLOR_DEPTH_BITS      16  /* color depth in bits, 32 or 16 */
#define   CONSOLE_X_DIVISOR     1   /* fractional screen, 1 is full */
#define   CONSOLE_Y_DIVISOR     1   /* fractional screen, 1 is full */
#define   CONSOLE_X_ORIENTATION 0   /* in number of characters */
#define   CONSOLE_Y_ORIENTATION 0   /* in number of lines */
#define ENABLE_USB         FALSE /* enable USB host */
#define ENABLE_XMODEM      FALSE /* enable xmodem protocol */
#define ENABLE_BOOTLOADER  FALSE /* enable boot loader */
#define   MAX_BOOT_LENGTH  (1024 * 1024 * 16) /* 16 MB boot image max */
#define ENABLE_AUTO_START  FALSE /* Auto start enabled devices */
#define ENABLE_STDIO       FALSE /* enable STanDard Input and Output */
#define ENABLE_PRINTF      FALSE /* enable printf arguments */
#define ENABLE_ASSERT      FALSE /* enable assertions */
#define ENABLE_MALLOC      FALSE /* enable malloc/free */

/* USB Specific configuration */
#define ENABLE_USB_HID     (TRUE & ENABLE_USB) /* for keyboard/mouse */
#define ENABLE_USB_ETHER   (TRUE & ENABLE_USB) /* enable Ethernet */
#define ENABLE_USB_TASK    (FALSE & ENABLE_USB)/* task for interrupts */

/* Network configuration */
#define ENABLE_IP4         (FALSE && \  /* Inet Protocol v4 */
                            ENABLE_MALLOC && ENABLE_ETHER)
#define ENABLE_IP6         (FALSE && \ /* Inet Protocol v6 */
                            ENABLE_MALLOC && ENABLE_ETHER)
#define ENABLE_UDP         (TRUE & (ENABLE_IP4 || ENABLE_IP4))/* UDP */
#define ENABLE_TCP         (TRUE & (ENABLE_IP4 || ENABLE_IP4))/* TCP */
#define ENABLE_DHCP        (TRUE & ENABLE_UDP)/* Dynamic IP Discovery */
#define ENABLE_ICMP        (TRUE & ENABLE_IP4)/* Internet Control */

/* Derived configuration; DO NOT EDIT BELOW */
#define ENABLE_ETHER       ENABLE_USB_ETHER  /* enable Ethernet */
#define ENABLE_NETWORK     (ENABLE_ETHER && (ENABLE_IP4 || ENABLE_IP6))


#endif /* _CONFIGURE_H */
