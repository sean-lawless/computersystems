/*...................................................................*/
/*                                                                   */
/*   Module:  main.c                                                 */
/*   Version: 2019.0                                                 */
/*   Purpose: main function for game application                     */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2019, Sean Lawless                    */
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
#include <system.h>
#include <board.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb/device.h>

extern int OgSp;

int ScreenUp, GameUp, UsbUp, NetUp;

#if ENABLE_USB
int UsbHostStart(char *command)
{
  if (!UsbUp)
  {
    DeviceInit();

    if (!HostEnable())
    {
      puts("Cannot initialize USB host controller interface");
      return TASK_FINISHED;
    }
    UsbUp = TRUE;
  }
  else
    puts("USB host already initialized");

  return TASK_FINISHED;
}

#endif /* ENABLE_USB */

// Defined in memory.map around BSS section
extern u8 __bss_start[];
extern u8 __bss_end[];

/*...................................................................*/
/*        main: Application Entry Point                              */
/*                                                                   */
/*     Returns: Exit error                                           */
/*...................................................................*/
int main(void)
{
  // Zero out the BSS memory (static globals)
  //   Could be in a pre-main function _start() instead?
  bzero(__bss_start, __bss_end - __bss_start);

  // Initialize hardware peripheral configuration
  NetUp = UsbUp = ScreenUp = FALSE;

  /* Initialize the base software interface to the hardware. */
  BoardInit();

#if ENABLE_OS
  // Initialize the Operating System (OS) and create system tasks
  OsInit();

#if ENABLE_UART0
  StdioState = &Uart0State;
  TaskNew(0, ShellPoll, &Uart0State);
#elif ENABLE_UART1
  StdioState = &Uart1State;
  TaskNew(0, ShellPoll, &Uart1State);
#endif

  // Initialize the timer and LED tasks
  TaskNew(1, TimerPoll, &TimerStart);
  TaskNew(MAX_TASKS - 1, LedPoll, &LedState);
#endif /* ENABLE_OS */

  // Initialize user devices

#if ENABLE_AUTO_START
#if ENABLE_VIDEO
  /* Initialize screen and console. */
  ScreenInit();

  /* Register video console with the OS. */
  bzero(&ConsoleState, sizeof(ConsoleState));
  Console(&ConsoleState);
#endif
#if ENABLE_USB
  // Start the USB host
  UsbHostStart(NULL);
#endif
#endif

  /* display the introductory splash */
  puts("Game application");
  putu32(OgSp);
  puts(" : stack pointer");

  putu32((u32)__bss_end);
  putchar('-');
  putu32((u32)__bss_start);
  putchar(':');
  putu32((__bss_end - __bss_start));
  puts(" bytes of BSS zeroed");

#if ENABLE_OS
  /* run the non-interruptive priority loop scheduler */
  OsStart();
#elif ENABLE_SHELL
  SystemShell();
#else
  puts("Press any key to exit application");
  uart0_getc();
#endif

  /* on OS exit say goodbye (never gets here...)*/
  puts("Goodbye");
  return 0;
}
