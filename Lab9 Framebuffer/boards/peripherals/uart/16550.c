/*...................................................................*/
/*                                                                   */
/*   Module:  uart0_16550.c                                          */
/*   Version: 2015.0                                                 */
/*   Purpose: uart0 functions for Raspberry Pi                       */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2015-2019, Sean Lawless               */
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


#if ENABLE_UART1

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define UART_BASE          UART1_BASE
#define   BAUD_DIV         270 /* ((250000000 / 115200) / 8) - 1 */

/*...................................................................*/
/* Symbols                                                           */
/*...................................................................*/
/*
 * 16550 based mini UART register map
*/
#define UART_ENABLE      (UART_BASE + 0x04)
#define   ENABLE                 (1 << 0)
#define UART_IO          (UART_BASE + 0x40)
#define   RX_DATA                (0xFF << 0)
#define UART_IE          (UART_BASE + 0x44)
#define UART_II          (UART_BASE + 0x48)
#define UART_LINE_CTRL   (UART_BASE + 0x4C)
#define   BYTE_WORD_LENGTH       (1 << 0)
#define UART_MC          (UART_BASE + 0x50)
#define UART_STATUS      (UART_BASE + 0x54)
#define   RX_DATA_READY          (1 << 0)
#define   RX_OVERRUN             (1 << 1)
#define   TX_FIFO_EMPTY          (1 << 5)
#define UART_MOD_STATUS  (UART_BASE + 0x58)
#define UART_SCRATCH     (UART_BASE + 0x5C)
#define UART_CONTROL     (UART_BASE + 0x60)
#define   ENABLE_RX              (1 << 0)
#define   ENABLE_TX              (1 << 1)
#define UART_STAT        (UART_BASE + 0x64)
#define UART_BAUD        (UART_BASE + 0x68)

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/*   Uart1Init: Initialize the secondary UART                        */
/*                                                                   */
/*...................................................................*/
void Uart1Init(void)
{
  u32 gpio;

  /* Turn on UART and then disable it before configuring. */
  REG32(UART_ENABLE) = ENABLE;
  REG32(UART_LINE_CONTROL) = 0;
  REG32(UART_CONTROL) = 0;

  /* Set the baud rate. */
  REG32(UART_BAUD) = BAUD_DIV;

  /* Enable the UART and clear the received error count. */
  REG32(UART_LINE_CTRL) = BYTE_WORD_LENGTH;
  REG32(UART_CONTROL) = ENABLE_RX | ENABLE_TX;
  return;
}

/*...................................................................*/
/*   Uart1Puts: Output a string to the UART                          */
/*                                                                   */
/*       Input: string to output                                     */
/*...................................................................*/
void Uart1Puts(const char *string)
{
  int i;

  /* Loop until the string buffer ends. */
  for (i = 0; string[i] != '\0'; ++i)
    Uart1Putc(string[i]);

  /* The puts() command must end with new line and carriage return. */
  Uart1utc('\n');
  Uart1Putc('\r');
}

/*...................................................................*/
/*   Uart1Putc: Output one character to the UART                     */
/*                                                                   */
/*       Input: character to output                                  */
/*...................................................................*/
void Uart1Putc(char character)
{
  u32 status;

  /* Read the UART status. */
  status = REG32(UART_STATUS);

  /* Loop until UART transmission line has room to send. */
  while (!(status & TX_FIFO_EMPTY))
    status = REG32(UART_STATUS);

  /* Send the character. */
  REG32(UART_IO) = character;
}

/*...................................................................*/
/* Uart1RxCheck: Retrun true if character is waiting in UART FIFO    */
/*                                                                   */
/*     Returns: one '1' if UART receive has a character              */
/*...................................................................*/
u32 Uart1RxCheck(void)
{
  /* If RX FIFO is empty return zero, otherwise one. */
  if (REG32(UART_STATUS) & RX_DATA_READY)
    return 1;
  else
    return 0;
}

/*...................................................................*/
/*   Uart1Getc: Receive one character from the UART                  */
/*                                                                   */
/*       Input: character to output                                  */
/*...................................................................*/
char Uart1Getc(void)
{
  u32 character, status;

  /* Loop until UART Rx FIFO is no longer empty. */
  for (status = REG32(UART_STATUS); !(status & RX_DATA_READY);
       status = REG32(UART_STATUS)) ;

  /* Read the character. */
  character = REG32(UART_IO);

  /* Return the character read. */
  return (RX_DATA & character);
}

/*...................................................................*/
/*  Uart1Flush: Flush all output                                     */
/*                                                                   */
/*...................................................................*/
void Uart1Flush(void)
{
  u32 status;

  /* Loop until UART transmit and receive queues are empty. */
  for (status = REG32(UART_STATUS); (status & RX_DATA_READY) ||
       !(status & TX_FIFO_EMPTY); status = REG32(UART_STATUS))
  {
    if ((status & RX_DATA_READY))
      Uart1Getc();
  }
}

#endif /* ENABLE_UART1 */
