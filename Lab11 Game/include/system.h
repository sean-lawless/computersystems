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
/* Configuration                                                     */
/*...................................................................*/
#define COMMAND_LENGTH   80

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
/*
** Common definitions
*/
#define TRUE           1
#define FALSE          0
#define NULL           0

/*
** Time definitions
*/
#define MICROS_PER_SECOND      1000000 /* Microseconds per second */
#define MICROS_PER_MILLISECOND 1000    /* Microseconds per millisecond */

/*
** Polled task return values
*/
#define TASK_IDLE      0
#define TASK_READY     1
#define TASK_FINISHED  2

/*...................................................................*/
/* Macro Definitions                                                 */
/*...................................................................*/

/*
 * Register manipulation macros
*/
#define REG8(address)  (*(volatile unsigned char *)(address))
#define REG16(address) (*(volatile unsigned short *)(address))
#define REG32(address) (*(volatile unsigned int *)(address))

/*
 * Predefined colors (true color 32 bit RGBA)
 *
*/
#define COLOR_WHITE      Color32(255, 255, 255, 255)
#define COLOR_RED        Color32(255, 0, 0, 255)
#define COLOR_GREEN      Color32(0, 255, 0, 255)
#define COLOR_BLUE       Color32(0, 0, 255, 255)
#define COLOR_BROWN      Color32(165, 42, 42, 255)
#define COLOR_GOLDEN     Color32(218, 165, 32, 255)
#define COLOR_ORANGERED  Color32(255, 69, 0, 255)
#define COLOR_DEEPSKYBLUE Color32(0, 191, 255, 255)
#define COLOR_SIENNA     Color32(160, 82, 45, 255)
#define COLOR_MAGENTA    Color32(255, 0, 255, 255)
#define COLOR_YELLOW     Color32(255, 255, 0, 255)
#define COLOR_BLACK      Color32(0, 0, 0, 0)

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
typedef unsigned long      size_t;
typedef unsigned long      uintptr_t;

#if COLOR_DEPTH_BITS == 16
  #define COLOR16(red, green, blue)   ((((u16)red) & 0x1F) << 11 \
                                    | (((u16)green) & 0x1F) << 6 \
                                    | (((u16)blue) & 0x1F))
  typedef u16 ScreenColor;
#elif COLOR_DEPTH_BITS == 32
  typedef u32 ScreenColor;
#else
  #error COLOR_DEPTH_BITS must be 16 or 32
#endif

/*
 * Timer structures
*/
struct timer
{
  u64 expire;
  u64 last;
};

struct timer_task
{
  struct timer expire;
  int (*poll) (u32 id, void *data, void *context);
  void *data;
  void *context;
  struct timer_task *next;
};

struct timer_state
{
  struct timer_task *timers;
};

/*
 * Task structures
*/
typedef struct
{
  char *command;
  int (*function)(const char *command);
} ShellCmd;

struct task
{
  int (*poll) (void *data);
  void *data;
  void *stdio;
} Task;

/*
 * State structures
*/
struct led_state
{
  int state;
  struct timer expire;
};

struct shell_state
{
  u8 command[COMMAND_LENGTH], i;
  ShellCmd *cmd;
  void *param;
  int result;
  char (*getc)(void);
  void (*putc)(char character);
  void (*puts)(const char *string);
  void (*flush)(void);
  u32  (*check)(void);
};

/*...................................................................*/
/* External Global Variables                                         */
/*...................................................................*/
extern u32 LedTime;
extern struct led_state LedState;
extern struct shell_state Uart0State, Uart1State, ConsoleState;
extern struct timer_state TimerState;
extern int ScreenUp, GameUp;

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
 * UART interfaces
*/
void Uart0Init(void);
void Uart1Init(void);

/*
 * Xmodem interface
*/
void *XmodemStart(unsigned char *destination, int length);
int XmodemDownload(unsigned char *destination, int length);
int XmodemPoll(void *data);

/*
 * Timer interface
*/
#define Sleep(a) usleep((u64)(a) * MICROS_PER_SECOND)
void usleep(u64 microseconds);
struct timer TimerRegister(u64 microseconds);
u64 TimerRemaining(struct timer *tw);
struct timer TimerSchedule(unsigned int usec,
                        int (*poll) (u32 id, void *data, void *context),
                        void *data, void *context);
int TimerCancel(void *poll, void *data);
u64 TimerNow(void);

/*
 * System interface
*/
void SystemShell(void);
int ShellPoll(void *data);
int LedPoll(void *data);
int TimerPoll(void *data);

/*
 * Screen interface
*/
int  ScreenInit(void);
void ScreenClose(void);
void SetPixel(u32 x, u32 y, u32 color);
void ScreenClear();
void DisplayCharacter(char ascii, u32 color);
int  DisplayString(const char *string, int length, u32 color);
void DisplayCursorChar(char ascii, u32 x, u32 y, u32 color);
u32  Color32(u8 red, u8 green, u8 blue, u8 alpha);

/*
 * Console Interface
*/
int Console(struct shell_state *console_state);

/*
 * Font Interface
*/
int CharacterPixel(char ch, u32 x, u32 y);
u32 CharacterHeight();
u32 CharacterWidth();

/*
 * Operating System interface
*/
void OsInit(void);
void OsStart(void);
int  TaskNew(int priority, int (*poll) (void *data), void *data);
int  TaskEnd(int priority);

#endif /* _SYSTEM_H */
