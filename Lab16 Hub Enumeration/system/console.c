/*...................................................................*/
/*                                                                   */
/*   Module:  console.c                                              */
/*   Version: 2014.0                                                 */
/*   Purpose: Interface between video screen text and stdio          */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2014, Sean Lawless                    */
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
#include <string.h>
#include <stdio.h>

#if ENABLE_VIDEO

extern void ConsoleEnableKeyboard(void);
extern int KeyboardEnabled;
extern int ScreenUp;

/*...................................................................*/
/*   cputc: Console putc (put character)                             */
/*                                                                   */
/*   Input: c the character to put                                   */
/*...................................................................*/
static void cputc(char c)
{
  DisplayCharacter(c, COLOR_WHITE);
}

/*...................................................................*/
/*   cputs: Console puts (put string)                                */
/*                                                                   */
/*   Input: s the string to put                                      */
/*...................................................................*/
static void cputs(const char *s)
{
  DisplayString(s, strlen(s), COLOR_WHITE);
  DisplayCharacter('\n', COLOR_WHITE);
}

/*...................................................................*/
/* Console: initialize Video Text Console                            */
/*                                                                   */
/*   Input: console_state is pointer to the console to initialize    */
/*                                                                   */
/*  Return: Zero (0) on success                                      */
/*...................................................................*/
int Console(struct shell_state *console_state)
{
  /* If screen is avaiable, initialize shell state for video console.*/
  if (ScreenUp)
  {
    console_state->result = TASK_FINISHED;
    console_state->cmd = NULL;
    console_state->flush = NULL;
    console_state->putc = cputc;
    console_state->puts = cputs;
    console_state->getc = NULL;
    console_state->check = NULL;

    /* display video console active notice */
    console_state->puts("Video console activated");

#if ENABLE_USB_HID
    /* Enable interactive console if keyboard is already up. */
    if (KeyboardEnabled)
      ConsoleEnableKeyboard();
#endif
  }
  return 0;
}

#endif /* ENABLE_VIDEO */

