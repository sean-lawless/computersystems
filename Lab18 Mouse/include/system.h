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
#define MICROS_PER_MILLISECOND 1000  /* Microseconds per millisecond */

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

/*
 * Alternate integer types
*/
typedef unsigned char      u8_t;
typedef unsigned short     u16_t;
typedef unsigned int       u32_t;
typedef unsigned long long u64_t;
typedef char               s8_t;
typedef short              s16_t;
typedef int                s32_t;
typedef long long          s64_t;

/*
 * Longer named integer types
*/
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef char               int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;

#if COLOR_DEPTH_BITS == 16
  #define COLOR16(red, green, blue)   (((((u16)red) & 0x1F) << 11) | \
                                       ((((u16)green) & 0x1F) << 6) | \
                                       ((((u16)blue) & 0x1F)))
  typedef u16 ScreenColor;
#elif COLOR_DEPTH_BITS == 32
  typedef u32 ScreenColor;
#else
  #error COLOR_DEPTH_BITS must be 16 or 32
#endif

/*
 * List structures
*/
struct double_link
{
  struct double_link *next;
  struct double_link *previous;
};

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
  struct double_link list;
  struct timer expire;
  int (*poll) (u32 id, void *data, void *context);
  void *data;
  void *context;
};

/*
 * Task structures
*/
struct task
{
  struct double_link list;
  int priority;
  int (*poll) (void *data);
  void *data;
  void *stdio;
};

struct ShellCmd
{
  char *command;
  int (*function)(const char *command);
};

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
  struct ShellCmd *cmd;
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
extern struct timer_task *TimerStart;
extern int ScreenUp, GameUp, UsbUp;

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
void *XmodemStart(u8 *destination, int length);
int XmodemDownload(u8 *destination, int length);
int XmodemPoll(void *data);

/*
 * Timer interface
*/
void TimerInit(void);
#define Sleep(a) usleep((u64)(a) * MICROS_PER_SECOND)
void usleep(u64 microseconds);
struct timer TimerRegister(u64 microseconds);
u64 TimerRemaining(struct timer *tw);
struct timer_task *TimerSchedule(u32 usec,
                       int (*poll) (u32 id, void *data, void *context),
                       void *data, void *context);
int TimerServiceCancel(void *poll, void *data);
void TimerCancel(struct timer_task *tt);
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
 * Malloc Interface
*/
void MallocInit(uintptr_t base, u32 size);
u32 MallocRemaining(void);

/*
 * Operating System interface
*/
void OsInit(void);
void OsStart(void);
struct task *TaskNew(int priority, int (*poll) (void *data),
                     void *data);
int  TaskEnd(struct task *endingTask);

/*
 * Host Controller asynchronous USB interface
*/
int HostEnable(void);
void HostDisable(void);
void HostSubmitAsyncRequest(void *urb, void *param, void *context);
int HostEndpointTransfer(void *host, void *endpoint,
  void *buffer, unsigned bufSize, void (complete)(void *urb,
  void *param, void *context));
int HostEndpointControlMessage(void *hub, void *endpoint,
  u8 requestType, u8 request, u16 value, u16 index, void *data,
  u16 dataSize, void (complete)(void *urb, void *param, void *context),
  void *param);
// Configuration
int HostGetEndpointDescriptor(void *device, void *endpoint, u8 type,
  u8 index, void *buffer, unsigned bufSize, u8 requestType,
  void (complete)(void *urb, void *param, void *context), void *param);
int HostSetEndpointAddress(void *device, void *endpoint,
              u8 deviceAddress, void (complete)(void *urb, void *param,
              void *context), void *param);
int HostSetEndpointConfiguration(void *device, void *endpoint,
      u8 configurationValue, void (complete)(void *urb, void *param,
      void *context), void *param);
int HostGetPortSpeed(void);

/*
 * Ethernet interface
*/
const u8 *LanGetMAC(void);
int LanDeviceSendFrame(const void *buffer, u32 length);
int LanReceiveAsync(void);

/*
 * Double linked list inline functions
*/
// Insert after a list element
static inline void ListInsertAfter(void *i, void *c)
{
  struct double_link *item = i, *current = c;

  item->next = current->next;
  item->previous = current;
  current->next = item;
  if (item->next)
    item->next->previous = item;
}

// Insert before a list element
static inline void ListInsertBefore(void *i, void *c)
{
  struct double_link *item = i, *current = c;

  item->next = current;
  item->previous = current->previous;
  current->previous = item;
  if (item->previous)
    item->previous->next = item;
}

// Remove from a list
static inline void ListRemove(struct double_link item)
{
  if (item.previous)
  {
    item.previous->next = item.next;
    if (item.next)
      item.next->previous = item.previous;
  }
  else if (item.next)
    item.next->previous = NULL;

  item.next = NULL;
  item.previous = NULL;
}

// Append to the end of a list
static inline void ListAppend(void *i, void *l)
{
  struct double_link *item = i, *list = l, *current;

  // Loop until the end of either NULL ending and circular list
  for (current = list; (current->next && (current->next != list));
       current = current->next) ;

  // Insert the node after the last node
  if (current)
    ListInsertAfter(item, current);
}

#endif /* _SYSTEM_H */
