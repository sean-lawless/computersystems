/*...................................................................*/
/*                                                                   */
/*   Module:  main.c                                                 */
/*   Version: 2015.0                                                 */
/*   Purpose: main function for LED error codes example              */
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

/*...................................................................*/
/*        main: Application Entry Point                              */
/*                                                                   */
/*     Returns: Exit error                                           */
/*...................................................................*/
int main(void)
{
  u8 i, code, tmp;

  /* Initialize the hardware. */
  BoardInit();

  /* Loop, testing different error codes */
  for (code = 1;; ++code)
  {
    /* Blink out the high order nibble of the byte error code. */
    tmp = ((code >> 4) & 0xF);
    for (i = 0; i < tmp; ++i)
    {
      LedOn();
      usleep(MICROS_PER_SECOND / 4);
      LedOff();
      usleep(MICROS_PER_SECOND / 4);
    }

    /* Sleep one second in between nibbles. */
    Sleep(1);

    /* Blink out the low order nibble of the byte error code. */
    tmp = code & 0xF;
    for (i = 0; i < tmp; ++i)
    {
      LedOn();
      usleep(MICROS_PER_SECOND / 4);
      LedOff();
      usleep(MICROS_PER_SECOND / 4);
    }

    /* Sleep three seconds and then repeat the error code. */
    Sleep(3);
  }
  return 0;
}


