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
#include <string.h>

/*...................................................................*/
/* Global variables                                                  */
/*...................................................................*/
extern int OgSp;

/*...................................................................*/
/*        main: Application Entry Point                              */
/*                                                                   */
/*     Returns: Exit error                                           */
/*...................................................................*/
int main(void)
{
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

  // Initialize the LED task
  TaskNew(MAX_TASKS - 1, LedPoll, &LedState);
#endif /* ENABLE_OS */

  /* Display the introductory splash. */
  puts("Console application");
  putu32(OgSp);
  puts(" : stack pointer");

#if ENABLE_VIDEO
  /* Initialize screen and if success draw a diagonal white line. */
  if (ScreenInit() == 0)
  {
    int x, y;

    // Draw white line down and to right from top left corner.
    for (x = 0, y = 0; x < 100; ++x, ++y)
      SetPixel(x, y, COLOR_WHITE);
  }
#endif

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

