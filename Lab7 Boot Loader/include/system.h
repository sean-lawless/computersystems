/*...................................................................*/
/*                                                                   */
/*   Module:  system.h                                               */
/*   Version: 2015.0                                                 */
/*   Purpose: system header declarations                             */
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
#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <configure.h>

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
/*
** Common definitions
*/
#define TRUE           1
#define FALSE          0

/*
** Time definitions
*/
#define MICROS_PER_SECOND      1000000 /* Microseconds per second */
#define MICROS_PER_MILLISECOND 1000  /* Microseconds per millisecond */

/*...................................................................*/
/* Macro Definitions                                                 */
/*...................................................................*/

/*
 * Register manipulation macros
*/
#define REG8(address)  (*(volatile unsigned char *)(address))
#define REG16(address) (*(volatile unsigned short *)(address))
#define REG32(address) (*(volatile unsigned int *)(address))

/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
/*
 * Integer types
*/
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef char               i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;
typedef unsigned long      uintptr_t;

/*
 * Timer structures
*/
struct timer
{
  u64 expire;
  u64 last;
};

/*...................................................................*/
/* Global Function Definitions                                       */
/*...................................................................*/
/*
 * Board interface
*/
void BoardInit(void);
void LedOn(void);
void LedOff(void);

/*
 * UART interface
*/
void UartInit(void);
void UartPutc(char character);
void UartPuts(const char *string);
unsigned int UartRxCheck (void);
char UartGetc(void);
void UartFlush(void);

/*
 * Timer interface
*/
#define Sleep(a) usleep((u64)(a) * MICROS_PER_SECOND)
void usleep(u64 microseconds);
struct timer TimerRegister(u64 microseconds);
u64 TimerRemaining(struct timer *tw);
u64 TimerNow(void);

/*
 * Shell interface
*/
void SystemShell(void);

/*
 * String interface
*/
int strlen(const char *string);
void *memcpy(void *dst, const void *src, int length);
int memcmp(const void *data1, const void *data2, int length);

/*
 * Boot Loader interface
*/
int XmodemDownload(u8 *destination, int length);


#endif /* _SYSTEM_H */
