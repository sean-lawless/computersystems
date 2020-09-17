/*...................................................................*/
/*                                                                   */
/*   Module:  main.c                                                 */
/*   Version: 2015.0                                                 */
/*   Purpose: main function for console application                  */
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
#include <system.h>
#include <board.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb/request.h>
#include <usb/device.h>

/*...................................................................*/
/* Global variables                                                  */
/*...................................................................*/
extern int OgSp;
int ScreenUp, UsbUp, NetUp;

#if ENABLE_USB_HID

static char KeyIn;

unsigned int usbKbdCheck(void)
{
  return KeyIn;
}

char usbKbdGetc(void)
{
  char key = KeyIn;

  KeyIn = 0;
  return key;
}

void KeyPressedHandler(const char ascii)
{
  KeyIn = ascii;

  /* If interactive console is not up yet, output to UART. */
  if (ConsoleState.getc == NULL)
    Uart0State.putc(ascii);
}

#endif /* ENABLE_USB_HID */

#if ENABLE_USB
int UsbHostStart(char *command)
{
  if (!UsbUp)
  {
    RequestInit();
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

/*...................................................................*/
/*        main: Application Entry Point                              */
/*                                                                   */
/*     Returns: Exit error                                           */
/*...................................................................*/
int main(void)
{
  // Initialize global variables
  NetUp = UsbUp = ScreenUp = FALSE;

  /* Initialize the hardware. */
  BoardInit();

#if ENABLE_OS
  // Initialize the Operating System (OS) and create system tasks
  OsInit();

#if ENABLE_UART0
  /* Set task specific stdio. */
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

  /* Display the introductory splash. */
  puts("Console application");
  putu32(OgSp);
  puts(" : stack pointer");

#if ENABLE_OS
  /* Run the priority loop scheduler. */
  OsStart();
#elif ENABLE_SHELL
  /* Run the system console shell. */
  SystemShell();
#else
  puts("Press any key to exit application");
  Uart0Getc();
#endif

  /* On OS exit say goodbye. */
  puts("Goodbye");
  return 0;
}
