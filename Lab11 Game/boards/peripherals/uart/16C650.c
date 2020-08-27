/*...................................................................*/
/*                                                                   */
/*   Module:  16C650.c                                               */
/*   Version: 2015.0                                                 */
/*   Purpose: uart0 system interface for 16C650 UART                 */
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
#include <stdio.h>
#include <string.h>

/* NOTE: To use UART0 on RPI 3 requires dtoverlay=pi3-miniuart-bt
 *       or equivalent in boot filesystem config.txt file.
 */
#if ENABLE_UART0

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define UART_BASE          UART0_BASE

/*...................................................................*/
/* Symbols                                                           */
/*...................................................................*/
/*
 * Primary UART (16C650) register map
*/
#define UART_DATA        (UART_BASE + 0x00)
#define   RX_DATA                (0xFF << 0)
#define   DR_FRAME_ERROR         (1 << 8)
#define   DR_PARITY_ERROR        (1 << 9)
#define   DR_BREAK_ERROR         (1 << 10)
#define   DR_OVERRUN_ERROR       (1 << 11)
#define     DATA_ERROR   (DR_FRAME_ERROR | DR_PARITY_ERROR | \
                          DR_BREAK_ERROR | DR_OVERRUN_ERROR)
#define UART_RX_STATUS   (UART_BASE + 0x04)
#define   RX_FRAME_ERROR         (1 << 0)
#define   RX_PARITY_ERROR        (1 << 1)
#define   RX_BREAK_ERROR         (1 << 2)
#define   RX_OVERRUN_ERROR       (1 << 3)
#define     RX_ERROR     (RX_FRAME_ERROR | RX_PARITY_ERROR | \
                          RX_BREAK_ERROR | RX_OVERRUN_ERROR)
#define UART_STATUS      (UART_BASE + 0x18)
#define   CLEAR_TO_SEND          (1 << 0)
#define   BUSY                   (1 << 3)
#define   RX_FIFO_EMPTY          (1 << 4)
#define   TX_FIFO_FULL           (1 << 5)
#define   RX_FIFO_FULL           (1 << 6)
#define   TX_FIFO_EMPTY          (1 << 7)
#define UART_ILPR        (UART_BASE + 0x20)
#define UART_IBRD        (UART_BASE + 0x24)
#define   BAUD_DIVISOR           (0xFFFF << 0)
#define UART_FBRD        (UART_BASE + 0x28)
#define   FRACTIONAL_DIVISOR     (0x1F << 0)
#define UART_LINE_CTRL   (UART_BASE + 0x2C)
#define   BREAK                  (1 << 0)
#define   PARITY                 (1 << 1)
#define   EVEN_PARITY            (1 << 2)
#define   TWO_STOP_BITS          (1 << 3)
#define   ENABLE_FIFO            (1 << 4)
#define   BYTE_WORD_LENGTH       (3 << 5)
#define   STICK_PARITY           (1 << 7)
#define UART_CONTROL     (UART_BASE + 0x30)
#define   ENABLE                 (1 << 0)
#define   LOOPBACK               (1 << 7)
#define   TX_ENABLE              (1 << 8)
#define   RX_ENABLE              (1 << 9)
#define   RTS                    (1 << 11)
#define   RTS_FLOW_CONTROL       (1 << 14)
#define   CTS_FLOW_CONTROL       (1 << 15)
#define UART_IFLS        (UART_BASE + 0x34)
#define UART_IMSC        (UART_BASE + 0x38)
#define UART_RIS         (UART_BASE + 0x3C)
#define UART_MIS         (UART_BASE + 0x40)
#define UART_ICR         (UART_BASE + 0x44)
#define UART_DMACR       (UART_BASE + 0x48)
#define UART_ITCR        (UART_BASE + 0x80)
#define UART_ITIP        (UART_BASE + 0x84)
#define   UART_RTS               (1 << 0)
#define   UART_CTS               (1 << 3)
#define UART_ITOP        (UART_BASE + 0x88)
#define UART_TDR         (UART_BASE + 0x8C)

#define RX_BUFFER_MASK     0xFFF

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
int RxErrorCount;

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/*    Uart0Init: Initialize the UART                                 */
/*...................................................................*/
void Uart0Init(void)
{
  /* Disable the UART before configuring. */
  REG32(UART_LINE_CTRL) = 0;
  REG32(UART_CONTROL) = 0;

  /* Set the baud rate. */
#if RPI >= 3
  // RPI 3/4 has a 48MHz clock as it was intended to work with BT.
  //(48000000 / (16 * 115200) = 26.042
  //(0.042*64)+0.5 = 3
  //115200 baud is int 26 frac 3
  REG32(UART_IBRD) = 26; /* Integer baud rate divisor. */
  REG32(UART_FBRD) = 3; /* Fractional baud rate divisor. */
#else
  // RPI 1/2 has a 3MHz clock
  //(3000000 / (16 * 115200) = 1.627
  //(0.627*64)+0.5 = 40
  //115200 baud is int 1 frac 40
  REG32(UART_IBRD) = 1; /* Integer baud rate divisor */
  REG32(UART_FBRD) = 40; /* Fractional baud rate divisor. */
#endif

  /* Enable the UART and clear the received error count. */
  REG32(UART_LINE_CTRL) = (BYTE_WORD_LENGTH | ENABLE_FIFO);
  REG32(UART_CONTROL) = ENABLE | TX_ENABLE | RX_ENABLE;
  RxErrorCount = 0;
  return;
}

/*...................................................................*/
/*   Uart0Puts: Output a string to the UART                          */
/*                                                                   */
/*       Input: string to output                                     */
/*...................................................................*/
void Uart0Puts(const char *string)
{
  int i;

  /* Loop until the string buffer ends. */
  for (i = 0; string[i] != '\0'; ++i)
    Uart0Putc(string[i]);

  /* The puts() command must end with new line and carriage return. */
  Uart0Putc('\n');
  Uart0Putc('\r');
}

/*...................................................................*/
/*   Uart0Putc: Output one character to the UART                     */
/*                                                                   */
/*       Input: character to output                                  */
/*...................................................................*/
void Uart0Putc(char character)
{
  u32 status;

  /* Read the UART status. */
  status = REG32(UART_STATUS);

  /* Loop until UART transmission line has room to send. */
  while (status & TX_FIFO_FULL)
    status = REG32(UART_STATUS);

  /* Send the character. */
  REG32(UART_DATA) = character;
}

/*...................................................................*/
/* Uart0RxCheck: Return true if any character is in UART FIFO        */
/*                                                                   */
/*     Returns: one '1' if UART receive has a character              */
/*...................................................................*/
u32 Uart0RxCheck(void)
{
  /* If RX FIFO is empty return zero, otherwise one. */
  if (REG32(UART_STATUS) & RX_FIFO_EMPTY)
    return 0;
  else
    return 1;
}

/*...................................................................*/
/*   Uart0Getc: Receive one character from the UART                  */
/*                                                                   */
/*       Input: character to output                                  */
/*                                                                   */
/*     Returns: character received from UART                         */
/*...................................................................*/
char Uart0Getc(void)
{
  u32 character, status;

  /* Loop until UART Rx FIFO is no longer empty. */
  while (!Uart0RxCheck()) ;

  /* Read the character. */
  character = REG32(UART_DATA);

  /* Read Rx status to acknowledge character and check for error. */
  status = REG32(UART_RX_STATUS);

  /* Check for and clear any read status errors. */
  if (status & RX_ERROR)
  {
    /* Clear the error and increment error count. */
    REG32(UART_RX_STATUS) = (status & RX_ERROR);
    RxErrorCount++;
    if (RxErrorCount > 100)
      Uart0Putc('E');
  }

  /* Return the character read. */
  return (RX_DATA & character);
}

/*...................................................................*/
/*  Uart0Flush: Flush all output                                     */
/*...................................................................*/
void Uart0Flush(void)
{
  unsigned int status;

  /* Loop until UART transmit and receive queues are empty. */
  for (status = REG32(UART_STATUS); !(status & RX_FIFO_EMPTY) ||
       !(status & TX_FIFO_EMPTY); status = REG32(UART_STATUS))
  {
    if (!(status & RX_FIFO_EMPTY))
      Uart0Getc();
  }
}

#endif /* ENABLE_UART0 */
