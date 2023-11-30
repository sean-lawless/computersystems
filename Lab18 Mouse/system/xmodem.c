/*...................................................................*/
/*                                                                   */
/*   Module:  xmodem.c                                               */
/*   Version: 2015.0                                                 */
/*   Purpose: xmodem receive system software                         */
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
#include <string.h>
#include <stdio.h>

#if ENABLE_XMODEM

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define XMODEM_PACKET_SIZE     132
#define XMODEM_DATA_SIZE       128
#define XMODEM_DATA_TIMEO      (MICROS_PER_SECOND)
#define XMODEM_DATA_RETRY      30

/*...................................................................*/
/* Symbols                                                           */
/*...................................................................*/
/*
 * Xmodem control characters
*/
#define SOH                    0x01
#define STX                    0x02
#define EOT                    0x04
#define ACK                    0x06
#define NAK                    0x15
#define CPMEOF                 0x1A

struct xmodem_state
{
  u8 *destination, packet[XMODEM_PACKET_SIZE];
  int block, rcvd, retry, i, length;
  struct timer packet_timer;
  struct shell_state *shell;
};

/*...................................................................*/
/* Globals                                                           */
/*...................................................................*/
struct xmodem_state XmodemState;

/*...................................................................*/
/* Local Functions                                                   */
/*...................................................................*/

/*...................................................................*/
/* Process an xmodem packet                                          */
/*                                                                   */
/*       packet - entire Xmodem frame                                */
/*       block - the current block number                            */
/*                                                                   */
/* Returns the block number or -1 if error                           */
/*...................................................................*/
static int process_xmodem(u8 *packet, int block)
{
  u32 i;
  u8 checksum;

  /* Return error if first character is not SOH. */
  if (packet[0] != SOH)
  {
#if ENABLE_VIDEO
    if (ScreenUp)
      ConsoleState.puts("first byte not SOH");
#endif
    return -1;
  }

  /* If inverse block does not match then return error. */
  if (packet[2] != 0xFF - packet[1])
  {
#if ENABLE_VIDEO
    if (ScreenUp)
      ConsoleState.puts("inverse block mismatch");
#endif
    return -1;
  }

  /* Return error if block is greater than current. */
  if (packet[1] > block)
  {
#if ENABLE_VIDEO
    if (ScreenUp)
      ConsoleState.puts("packet block breater then expected");
#endif
    return -1;
  }

  /* Calculate the checksum. */
  for (i = 0, checksum = 0; i < XMODEM_DATA_SIZE; ++i)
    checksum += packet[3 + i];

  /* Return error if checksum does not match. */
  if (checksum != packet[XMODEM_PACKET_SIZE - 1])
  {
#if ENABLE_VIDEO
    if (ScreenUp)
      ConsoleState.puts("packet checksum invalid");
#endif
    return -1;
  }

  /* Return block number on checksum match. */
  return packet[1];
}

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* XmodemInit: initialize the Xmodem protocol                        */
/*...................................................................*/
void XmodemInit(void)
{
  bzero(&XmodemState, sizeof(struct xmodem_state));
}

/*...................................................................*/
/* XmodemStart: start an Xmodem transfer                             */
/*                                                                   */
/*       Input: destination = location to save Xmodem data           */
/*              length = maximum length of Xmodem data               */
/*                                                                   */
/*     Returns: task state structure                                 */
/*...................................................................*/
void *XmodemStart(u8 *destination, int length)
{
  /* return failure if xmodem state is in use */
  /* this could be replaced by a loop if multiple xmodems expected */
  if (XmodemState.block)
    return NULL;

  /* Initialize the xmodem state. */
#if ENABLE_OS
  XmodemState.shell = StdioState;
#elif ENABLE_UART0
  XmodemState.shell = &Uart0State;
#elif ENABLE_UART1
  XmodemState.shell = &Uart1State;
#else
  return NULL;
#endif
  XmodemState.block = 1;
  XmodemState.rcvd = XmodemState.retry = XmodemState.i = 0;
  XmodemState.destination = destination;
  XmodemState.length = length;

  return &XmodemState;
}

/*...................................................................*/
/*  XmodemPoll: poll the Xmodem download                             */
/*                                                                   */
/*       Input: data = pointer to Xmodem state                       */
/*                                                                   */
/*     Returns: task state (TASK_IDLE, TASK_READY, TASK_FINISHED)    */
/*...................................................................*/
int XmodemPoll(void *data)
{
  int result;
  struct xmodem_state *state = data;

  /* Check timer and process timeout if needed. */
  if (TimerRemaining(&state->packet_timer) == 0)
  {
    /* Break out if retry count exceeds maximum. */
    if (++state->retry > XMODEM_DATA_RETRY)
    {
      state->shell->puts("Timeout");

      /* Clear block to free Xmodem and return failure. */
      state->block = 0;
      return TASK_FINISHED;
    }

    /* Restart timer and index. */
    state->packet_timer = TimerRegister(XMODEM_DATA_TIMEO);
    state->i = 0;

#if ENABLE_VIDEO
    if (ScreenUp)
      ConsoleState.puts("Data timeout, send NAK");
#endif

    /* On timeout, flush and send NAK. */
    state->shell->flush();
    state->shell->putc(NAK);
  }

  /* Check if character is available to read. */
  if (state->shell->check())
  {
    /* Get the next character. */
    result = state->shell->getc();

    /* Check if first character of block. */
    if (state->i == 0)
    {
      /* Transfer success if first character is EOT. */
      if (result == EOT)
      {
        /* Received size, in blocks minus start and end frames. */
        state->rcvd = (state->block - 2) * XMODEM_PACKET_SIZE;

        /* Trim EOF markers from last frame and return length. */
        while ((state->rcvd > 0) &&
               (state->destination[state->rcvd - 1] == CPMEOF))
          --state->rcvd;

        /* Send ACK on xmodem transfer success. */
        state->shell->putc(ACK);

        /* Clear block to free Xmodem and return success. */
        state->block = 0;
        puts("Download complete,'run' to execute the application");
        return TASK_FINISHED;
      }
    }

    /* Save the character to the packet and increment count. */
    state->packet[state->i++] = result;

    /* Check if an entire packet has been collected. */
    if (state->i == XMODEM_PACKET_SIZE)
    {
      /* Process the xmodem packet and return the block offset. */
      result = process_xmodem(state->packet, state->block & 0xFF);

      /* If resulting block offset valid, save block */
      if (result >= 0)
      {
        /* Preserve previous block overflows but adjust for resends. */
        state->block = result | (state->block & 0xFFFFFF00); /*  */

        /* Copy the xmodem frame to the destination buffer. */
        memcpy(
          &state->destination[(state->block - 1) * XMODEM_DATA_SIZE],
          &state->packet[XMODEM_PACKET_SIZE - XMODEM_DATA_SIZE - 1],
          XMODEM_DATA_SIZE);

        /* Increment received blocks and clear retry count. */
        ++state->block;

        /* Send ACK on success. */
        state->shell->putc(ACK);
      }
      else
      {
#if ENABLE_VIDEO
        if (ScreenUp)
          ConsoleState.puts("process xmodem failed");
#endif

        /* On failure flush and send NAK. */
        state->shell->flush();
        state->shell->putc(NAK);
      }

      /* Restart the packet timer. */
      state->packet_timer = TimerRegister(XMODEM_DATA_TIMEO);

      /* Clear retry and position counters. */
      state->retry = state->i = 0;
    }
    return TASK_READY;
  }
  else
    return TASK_IDLE;
}

/*...................................................................*/
/* XmodemDownload: download data from a peer using Xmodem            */
/*                                                                   */
/*       Input: destination is address to copy the downloaded data   */
/*              length is the maximum amount of data to download     */
/*                                                                   */
/*     Returns: length of download on success, or -1 if error        */
/*...................................................................*/
int XmodemDownload(u8 *destination, int length)
{
  struct xmodem_state *state;

  /* Initialize xmodem for this transfer, returning if error. */
  state = XmodemStart(destination, length);
  if (state == NULL)
    return -1;

  /* Loop that polls Xmodem until it finishes. */
  for (;XmodemPoll(state) != TASK_FINISHED;) ;

  /* Clear Xmodem state for other tasks and return download length. */
  state->block = 0;
  return state->rcvd;
}

#endif /* ENABLE_XMODEM */
