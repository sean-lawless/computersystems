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
#include <stdio.h>
#include <system.h>

/*...................................................................*/
/*        main: Application Entry Point                              */
/*                                                                   */
/*     Returns: Exit error                                           */
/*...................................................................*/
int main(void)
{
  int i;

  /* Initialize the hardware. */
  BoardInit();

  /* Display the introductory splash. */
  puts("Console application");
  puts("  Copyright 2015 Sean Lawless, All rights reserved.\n");

  puts("Check timer. Sleep 1s loop. Press key to exit.");
  for (i = 0; ; ++i)
  {
    /* Break out of loop if a key was pressed */
    if (UartRxCheck())
      break;

    /* Sleep one second and print the next character. */
    Sleep(1);
    putchar('0' + i);

    /* If all the way to 'Z' then start over at zero. */
    if (i == 42)
    {
      i = 0;
      putchar('\n');
      putchar('\r');
    }
  }

  /* Get and discard the key press and output a newline afterward. */
  getchar();
  putchar('\n');
  putchar('\r');

  puts("Check timers. Sleep 1/10s loop. Press key to exit.");
  for (i = 0; ; ++i)
  {
    /* Break out of loop if a key was pressed */
    if (UartRxCheck())
      break;

    /* Sleep one tenth of a second and print next character. */
    usleep(MICROS_PER_SECOND / 10);
    putchar('0' + i);

    /* If all the way to 'Z' the start over at zero. */
    if (i == 42)
    {
      i = 0;
      putchar('\n');
      putchar('\r');
    }
  }

  /* Get and discard the key press and output a newline afterward. */
  getchar();
  putchar('\n');
  putchar('\r');

  /* Run the system console shell. */
  UartPuts("Entering the system shell");
  SystemShell();

  /* On shell exit say goodbye */
  puts("Goodbye");
  return 0;
}


