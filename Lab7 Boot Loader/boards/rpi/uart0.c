/*...................................................................*/
/*                                                                   */
/*   Module:  uart0.c                                                */
/*   Version: 2015.0                                                 */
/*   Purpose: uart0 functions for Raspberry Pi                       */
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

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define XMODEM_PACKET_SIZE 132
#define XMODEM_DATA_SIZE   128
#define XMODEM_DATA_TIMEO  (MICROS_PER_SECOND)
#define XMODEM_DATA_RETRY  30

/*...................................................................*/
/* Symbols                                                           */
/*...................................................................*/
/*
 * Primary UART (16C650) register map
*/
#define UART0_DATA       (UART0_BASE + 0x00)
#define   RX_DATA                (0xFF << 0)
#define   DR_FRAME_ERROR         (1 << 8)
#define   DR_PARITY_ERROR        (1 << 9)
#define   DR_BREAK_ERROR         (1 << 10)
#define   DR_OVERRUN_ERROR       (1 << 11)
#define     DATA_ERROR   (DR_FRAME_ERROR | DR_PARITY_ERROR | \
                          DR_BREAK_ERROR | DR_OVERRUN_ERROR)
#define UART0_RX_STATUS  (UART0_BASE + 0x04)
#define   RX_FRAME_ERROR         (1 << 0)
#define   RX_PARITY_ERROR        (1 << 1)
#define   RX_BREAK_ERROR         (1 << 2)
#define   RX_OVERRUN_ERROR       (1 << 3)
#define     RX_ERROR     (RX_FRAME_ERROR | RX_PARITY_ERROR | \
                          RX_BREAK_ERROR | RX_OVERRUN_ERROR)
#define UART0_STATUS     (UART0_BASE + 0x18)
#define   CLEAR_TO_SEND          (1 << 0)
#define   BUSY                   (1 << 3)
#define   RX_FIFO_EMPTY          (1 << 4)
#define   TX_FIFO_FULL           (1 << 5)
#define   RX_FIFO_FULL           (1 << 6)
#define   TX_FIFO_EMPTY          (1 << 7)
#define UART0_ILPR       (UART0_BASE + 0x20)
#define UART0_IBRD       (UART0_BASE + 0x24)
#define   BAUD_DIVISOR           (0xFFFF << 0)
#define UART0_FBRD       (UART0_BASE + 0x28)
#define   FRACTIONAL_DIVISOR     (0x1F << 0)
#define UART0_LINE_CTRL  (UART0_BASE + 0x2C)
#define   BREAK                  (1 << 0)
#define   PARITY                 (1 << 1)
#define   EVEN_PARITY            (1 << 2)
#define   TWO_STOP_BITS          (1 << 3)
#define   ENABLE_FIFO            (1 << 4)
#define   BYTE_WORD_LENGTH       (3 << 5)
#define   STICK_PARITY           (1 << 7)
#define UART0_CONTROL    (UART0_BASE + 0x30)
#define   ENABLE                 (1 << 0)
#define   LOOPBACK               (1 << 7)
#define   TX_ENABLE              (1 << 8)
#define   RX_ENABLE              (1 << 9)
#define   RTS                    (1 << 11)
#define   RTS_FLOW_CONTROL       (1 << 14)
#define   CTS_FLOW_CONTROL       (1 << 15)
#define UART0_IFLS       (UART0_BASE + 0x34)
#define UART0_IMSC       (UART0_BASE + 0x38)
#define UART0_RIS        (UART0_BASE + 0x3C)
#define UART0_MIS        (UART0_BASE + 0x40)
#define UART0_ICR        (UART0_BASE + 0x44)
#define UART0_DMACR      (UART0_BASE + 0x48)
#define UART0_ITCR       (UART0_BASE + 0x80)
#define UART0_ITIP       (UART0_BASE + 0x84)
#define   UART_RTS               (1 << 0)
#define   UART_CTS               (1 << 3)
#define UART0_ITOP       (UART0_BASE + 0x88)
#define UART0_TDR        (UART0_BASE + 0x8C)

#define RX_BUFFER_MASK     0xFFF

/*
 * Xmodem control characters
*/
#define SOH                    0x01
#define STX                    0x02
#define EOT                    0x04
#define ACK                    0x06
#define NAK                    0x15
#define CPMEOF                 0x1A

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
int RxErrorCount;

/*...................................................................*/
/* Local Functions                                                   */
/*...................................................................*/
#if ENABLE_BOOTLOADER
/*...................................................................*/
/* Process an xmodem packet                                          */
/*                                                                   */
/*       packet - entire Xmodem frame                                */
/*       block - the current block number                            */
/*                                                                   */
/* Returns the block number or -1 if error                           */
/*...................................................................*/
static int process_xmodem(unsigned char *packet, int block)
{
  unsigned int i;
  unsigned char checksum;

  /* return error if first character is not SOH */
  if (packet[0] != SOH)
    return -1;

  /* if inverse block does not match then return error */
  if (packet[2] != 0xFF - packet[1])
    return -1;

  /* return error if block is greater than current */
  if (packet[1] > block)
    return -1;

  /* calculate the checksum */
  for (i = 0, checksum = 0; i < XMODEM_DATA_SIZE; ++i)
    checksum += packet[3 + i];

  /* return block number on checksum match */
  if (checksum == packet[XMODEM_PACKET_SIZE - 1])
    return packet[1];

  /* otherwise checksum error */
  else
    return -1;
}
#endif

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/*    UartInit: Initialize the UART                                  */
/*                                                                   */
/*...................................................................*/
void UartInit(void)
{
  u32 gpio;

  /* Disable the UART before configuring. */
  REG32(UART0_LINE_CTRL) = 0;
  REG32(UART0_CONTROL) = 0;

  /* Select Alternate 0 for UART over GPIO pins 14 Tx and 15 Rx.*/
  gpio = REG32(GPFSEL1);
  gpio &= ~(7 << 12); /* clear GPIO 14 */
  gpio |= GPIO_ALT0 << 12; /* set GPIO 14 to Alt 0 */
  gpio &= ~(7 << 15); /* clear GPIO 15 */
  gpio |= GPIO_ALT0 << 15; /* set GPIO 15 to Alt 0 */
  REG32(GPFSEL1) = gpio;

  /*
  ** GPPUD can be 0 (disable pull up/down)
  ** (0) disable pull up and pull down to float the GPIO
  ** (1 << 0) enable pull down (low)
  ** (1 << 1) enable pull up (high)
  */

  /* Disable pull up/down for next configured GPIOs so they float. */
  REG32(GPPUD) = GPPUD_OFF;
  usleep(MICROS_PER_MILLISECOND); /* hold time */

  /* Apply above configuration (floating) to UART Rx and Tx GPIOs. */
  REG32(GPPUDCLK0) = (1 << 14) | (1 << 15); /* GPIO 14 and 15 */
  usleep(MICROS_PER_MILLISECOND); /* hold time */

  /* Set the baud rate. */
#if RPI >= 3
  // RPI 3/4 has a 48MHz clock as it was intended to work with BT
  //(48000000 / (16 * 115200) = 26.042
  //(0.042*64)+0.5 = 3
  //115200 baud is int 26 frac 3
  REG32(UART0_IBRD) = 26; /* Integer baud rate divisor */
  REG32(UART0_FBRD) = 3; /* Fractional baud rate divisor */
#else
  // RPI 1/2 has a 3MHz clock by default
  //(3000000 / (16 * 115200) = 1.627
  //(0.627*64)+0.5 = 40
  //115200 baud is int 1 frac 40
  REG32(UART0_IBRD) = 1; /* Integer baud rate divisor */
  REG32(UART0_FBRD) = 40; /* Fractional baud rate divisor */
#endif

  /* Enable the UART and clear the received error count. */
  REG32(UART0_LINE_CTRL) = (BYTE_WORD_LENGTH | ENABLE_FIFO);
  REG32(UART0_CONTROL) = ENABLE | TX_ENABLE | RX_ENABLE;
  RxErrorCount = 0;
  return;
}

/*...................................................................*/
/*    UartPuts: Output a string to the UART                          */
/*                                                                   */
/*       Input: string to output                                     */
/*...................................................................*/
void UartPuts(const char *string)
{
  int i;

  /* Loop until the string buffer ends. */
  for (i = 0; string[i] != '\0'; ++i)
    UartPutc(string[i]);

  /* The puts() command must end with new line and carriage return. */
  UartPutc('\n');
  UartPutc('\r');
}

/*...................................................................*/
/*    UartPutc: Output one character to the UART                     */
/*                                                                   */
/*       Input: character to output                                  */
/*...................................................................*/
void UartPutc(char character)
{
  unsigned int status;

  /* Read the UART status. */
  status = REG32(UART0_STATUS);

  /* Loop until UART transmission line has room to send. */
  while (status & TX_FIFO_FULL)
    status = REG32(UART0_STATUS);

  /* Send the character. */
  REG32(UART0_DATA) = character;
}

/*...................................................................*/
/* UartRxCheck: Retrun true if any character is waiting in UART FIFO */
/*                                                                   */
/*     Returns: one '1' if UART receive has a character              */
/*...................................................................*/
unsigned int UartRxCheck(void)
{
  /* If RX FIFO is empty return zero, otherwise one. */
  if (REG32(UART0_STATUS) & RX_FIFO_EMPTY)
    return 0;
  else
    return 1;
}

/*...................................................................*/
/*    UartGetc: Receive one character from the UART                  */
/*                                                                   */
/*       Input: character to output                                  */
/*                                                                   */
/*...................................................................*/
char UartGetc(void)
{
  u32 character, status;

  /* Loop until UART Rx FIFO is no longer empty. */
  while (!UartRxCheck()) ;

  /* Read the character. */
  character = REG32(UART0_DATA);

  /* Read Rx status to acknowledge character and check for error. */
  status = REG32(UART0_RX_STATUS);

  /* Check for and clear any read status errors. */
  if (status & RX_ERROR)
  {
    /* Clear the error and increment error count. */
    REG32(UART0_RX_STATUS) = status;
    RxErrorCount++;
  }

  /* Return the character read. */
  return (RX_DATA & character);
}

/*...................................................................*/
/*   UartFlush: Flush all output                                     */
/*                                                                   */
/*...................................................................*/
void UartFlush(void)
{
  unsigned int status;

  /* Loop until UART transmit and receive queues are empty. */
  for (status = REG32(UART0_STATUS); !(status & RX_FIFO_EMPTY) ||
       !(status & TX_FIFO_EMPTY); status = REG32(UART0_STATUS))
  {
    if (!(status & RX_FIFO_EMPTY))
      UartGetc();
  }
}

#if ENABLE_BOOTLOADER
/*...................................................................*/
/* XmodemDownload: copy a file with the Xmodem protocol              */
/*                                                                   */
/*       Input: destination = location to save Xmodem data           */
/*              length = maximum length of Xmodem data               */
/*                                                                   */
/*     Returns: the length of the resulting xmodem transfer          */
/*...................................................................*/
int XmodemDownload(unsigned char *destination, int length)
{
  u8 packet[XMODEM_PACKET_SIZE];
  u32 block, retry;
  int i, rcvd;
  int result;
  u64 last_time = 1;
  struct timer packet_expire;

  /* Register the packet transfer expiration timer. */
  packet_expire = TimerRegister(XMODEM_DATA_TIMEO);

  /* Loop until the Xmodem download completes or times out. */
  for (block = 1, rcvd = retry = i = 0; i < XMODEM_PACKET_SIZE; )
  {
    /* Check timer and process timeout if needed. */
    last_time = TimerRemaining(&packet_expire);
    if (last_time == 0)
    {
      /* Break out if retry count exceeds maximum. */
      if (++retry > XMODEM_DATA_RETRY)
        break;

      /* Restart timer and index. */
      packet_expire = TimerRegister(XMODEM_DATA_TIMEO);
      i = 0;

      /* send a NAK */
      UartPutc(NAK);
    }

    /* Check if character is available to read. */
    if (UartRxCheck())
    {
      /* Get the next character. */
      result = UartGetc();

      /* Transfer success if first character is EOT. */
      if ((i == 0) && (result == EOT))
      {
        rcvd = (block - 2) * XMODEM_PACKET_SIZE;

        /* trim EOF markers from last frame and return length. */
        while ((rcvd > 0) && (destination[rcvd - 1] == CPMEOF))
          --rcvd;

        /* send ACK on xmodem transfer success */
        UartPutc(ACK);
        break;
      }

      /* Save the character to the packet and increment count. */
      packet[i++] = result;

      /* Check if an entire packet has been received. */
      if (i == XMODEM_PACKET_SIZE)
      {
        /* Process the xmodem packet and return the block offset. */
        result = process_xmodem(packet, block & 0xFF);

        /* If resulting block offset valid, save block. */
        if (result >= 0)
        {
          /* Update block number and copy to destination buffer. */
          block = result | (block & 0xFFFFFF00); /* overflow */
          memcpy(&destination[(block - 1) * XMODEM_DATA_SIZE],
                 &packet[XMODEM_PACKET_SIZE - XMODEM_DATA_SIZE - 1],
                 XMODEM_DATA_SIZE);

          /* Increment received blocks and clear retry count. */
          ++block;
          retry = 0;

          /* Send ACK on block transfer success. */
          UartPutc(ACK);
        }
        else
        {
          /* On failure flush and send NAK. */
          UartFlush();
          UartPutc(NAK);
        }

        /* Restart the packet timer. */
        packet_expire = TimerRegister(XMODEM_DATA_TIMEO);

        /* Clear packet count for the next packet. */
        i = 0;
      }
    }
  }

  /* Return the length of the resulting xmodem transfer. */
  return rcvd;
}
#endif /* ENABLE_BOOTLOADER */


