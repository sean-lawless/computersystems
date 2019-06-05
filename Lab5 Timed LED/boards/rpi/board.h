/*...................................................................*/
/*                                                                   */
/*   Module:  board.h                                                */
/*   Version: 2015.0                                                 */
/*   Purpose: header declarations for Raspberry Pi board             */
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

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
/*
 * SoC peripheral base addresses
*/
#if RPI == 1
#define PERIPHERAL_BASE 0x20000000 /* RPi B+ */
#else
#define PERIPHERAL_BASE 0x3F000000 /* RPi 2/3 */
#endif
#define TIMER_BASE      (PERIPHERAL_BASE | 0x003000)
#define GPIO_BASE       (PERIPHERAL_BASE | 0x200000)

/*
 * GPIO SELECT, SET and CLEAR register addresses
*/

// GPIO function select (GFSEL) registers have 3 bits per GPIO
#define GPFSEL0         (GPIO_BASE | 0x0) // GPIO select 0
#define GPFSEL1         (GPIO_BASE | 0x4) // GPIO select 1
#define GPFSEL2         (GPIO_BASE | 0x8) // GPIO select 2
#define GPFSEL3         (GPIO_BASE | 0xC) // GPIO select 3
#define GPFSEL4         (GPIO_BASE | 0x10)// GPIO select 4
#define   GPIO_INPUT         (0 << 0) // GPIO is input      (000)
#define   GPIO_OUTPUT        (1 << 0) // GPIO is output     (001)
#define   GPIO_ALT0          (4)      // GPIO is Alternate0 (100)
#define   GPIO_ALT1          (5)      // GPIO is Alternate1 (101)
#define   GPIO_ALT2          (6)      // GPIO is Alternate2 (110)
#define   GPIO_ALT3          (7)      // GPIO is Alternate3 (111)
#define   GPIO_ALT4          (3)      // GPIO is Alternate4 (011)
#define   GPIO_ALT5          (2)      // GPIO is Alternate5 (010)

// GPIO SET/CLEAR registers have 1 bit per GPIO
#define GPSET0          (GPIO_BASE | 0x1C) // set0 (GPIO 0 - 31)
#define GPSET1          (GPIO_BASE | 0x20) // set1 (GPIO 32 - 63)
#define GPCLR0          (GPIO_BASE | 0x28) // clear0 (GPIO 0 - 31)
#define GPCLR1          (GPIO_BASE | 0x2C) // clear1 (GPIO 32 - 63)

// GPIO Pull Up and Pull Down registers
#define GPPUD           (GPIO_BASE | 0x94)
#define   GPPUD_OFF       (0 << 0)
#define   GPPUD_PULL_DOWN (1 << 0)
#define   GPPUD_PULL_UP   (1 << 1)
#define GPPUDCLK0       (GPIO_BASE | 0x98)
#define GPPUDCLK1       (GPIO_BASE | 0x9C)

/*
 * Timer registers
*/
#define TIMER_CS        (TIMER_BASE | 0x00) // clock status
#define TIMER_CLO       (TIMER_BASE | 0x04) // clock low 32 bytes
#define TIMER_CHI       (TIMER_BASE | 0x08) // clock high 32 bytes

