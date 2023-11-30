/*...................................................................*/
/*                                                                   */
/*   Module:  stdio.h                                                */
/*   Version: 2015.0                                                 */
/*   Purpose: Standard I/O implementation                            */
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
#include <stdio.h>

#if ENABLE_OS
struct shell_state *StdioState;
#endif

/*...................................................................*/
/* Global Function Declaractions                                     */
/*...................................................................*/

/*...................................................................*/
/*     putchar: output a character to the UART                       */
/*                                                                   */
/*      Inputs: character - the character to output                  */
/*                                                                   */
/*     Returns: Success (0)                                          */
/*...................................................................*/
int putchar(char character)
{
#if ENABLE_OS
  StdioState->putc(character);
#else
#if ENABLE_UART0
#if ENABLE_SHELL
  /* Display on UART0. */
  if (Uart0State.putc)
    Uart0State.putc(character);
  else
#endif
    Uart0Putc(character);
#endif
#if ENABLE_UART1
#if ENABLE_SHELL
  /* Display on UART1. */
  if (Uart1State.putc)
    Uart1State.putc(character);
  else
#endif
    Uart1Putc(character);
#endif
#if ENABLE_VIDEO
  /* Display on video screen. */
  if (ScreenUp)
    ConsoleState.putc(character);
#endif
#endif /* ENABLE_OS */
  return 0;
}

/*...................................................................*/
/*     getchar: read a character from the UART                       */
/*                                                                   */
/*     Returns: Character read                                       */
/*...................................................................*/
char getchar(void)
{
#if ENABLE_OS
  return StdioState->getc();
#else
#if ENABLE_UART0
  /* main uart */
#if ENABLE_SHELL
  if (Uart0State.getc)
    return Uart0State.getc();
  else
#endif
    return Uart0Getc();
#elif ENABLE_UART1
  /* secondary uart */
#if ENABLE_SHELL
  if (Uart1State.getc)
    return Uart1State.getc();
  else
#endif
    return Uart1Getc();
#endif
#endif /* ENABLE_OS */
}

/*...................................................................*/
/*     putchar: output a character                                   */
/*                                                                   */
/*      Inputs: string - the string to output                        */
/*                                                                   */
/*     Returns: Success (0)                                          */
/*...................................................................*/
int puts(const char *string)
{
#if ENABLE_OS
  StdioState->puts(string);
#else
#if ENABLE_UART0
#if ENABLE_SHELL
  /* Output to the UART0. */
  if (Uart0State.puts)
    Uart0State.puts(string);
  else
#endif
    Uart0Puts(string);
#endif
#if ENABLE_UART1
#if ENABLE_SHELL
  /* Output to the UART1. */
  if (Uart1State.puts)
    Uart1State.puts(string);
  else
#endif
    Uart1Puts(string);
#endif
#if ENABLE_VIDEO
  /* Output on the video screen. */
  if (ScreenUp)
    ConsoleState.puts(string);
#endif
#endif /* ENABLE_OS */

  return 0;
}

int atoi(char *a)
{
  int i;

  i = 0;
  while (*a)
  {
    i = (i << 3) + (i << 1) + (*a) - '0';
    a++;
  }
  return i;
}

/*...................................................................*/
/*     putbyte: output a byte value to the UART                      */
/*                                                                   */
/*      Inputs: byte - the byte to transmit (Tx) to UART             */
/*                                                                   */
/*     Returns: the value of the byte that was transmitted           */
/*...................................................................*/
u8 putbyte(u8 byte)
{
  u8 nib1, nib2;

  /* convert to nibbles */
  nib1 = 0xF & byte;
  nib2 = (0xF0 & byte) >> 4;

  /* print the most significant nibble */
  if (nib2 > 9)
    putchar((nib2 - 10) + 'A');
  else
    putchar(nib2 + '0');

  /* print the least significant nibble */
  if (nib1 > 9)
    putchar((nib1 - 10) + 'A');
  else
    putchar(nib1 + '0');
  return byte;
}

