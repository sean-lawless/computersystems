/*...................................................................*/
/*                                                                   */
/*   Module:  transfer.c                                             */
/*   Version: 2020.0                                                 */
/*   Purpose: DesignWare USB host transfer of variable staged data   */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2020, Sean Lawless                    */
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
/*                                                                   */
/* The FreeBSD Project, Copyright 1992-2020                          */
/*   Copyright 1992-2020 The FreeBSD Project.                        */
/*   Permission granted under MIT license (Public Domain).           */
/*                                                                   */
/* Embedded Xinu, Copyright (C) 2013.                                */
/*   Permission granted under Creative Commons Zero (Public Domain). */
/*                                                                   */
/* Alex Chadwick (CSUD), Copyright (C) 2012.                         */
/*   Permission granted under MIT license (Public Domain).           */
/*                                                                   */
/* Thanks to Linux/Circle/uspi, and Rene Stange specifically, for    */
/* the quality reference model and runtime register debug output.    */
/*                                                                   */
/* Additional thanks to the Raspberry Pi bare metal forum community. */
/* https://www.raspberrypi.org/forums/viewforum.php?f=72             */
/*...................................................................*/
#include <board.h>
#include <stdio.h>
#include <assert.h>
#if ENABLE_USB
#include <usb/request.h>
#include <usb/device.h>
#include "dwhc.h"
#include "transfer.h"

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
#define FRAME_PERIODIC_MAX     8

// DWC frame registers
#define HFNUM                  (USB_BASE + 0x408) // host frame number
#define   HFNUM_NUMBER(reg)      ((reg) & 0xFFFF)
#define     DWC_HFNUM_MAX_FRNUM    0x3FFF

/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
typedef enum
{
  StartSplit,
  StartSplitComplete,
  CompleteSplit,
  CompleteRetry,
  CompleteSplitComplete,
  CompleteSplitFailed,
  StateUnknown
} FrameSchedulerState;

/*...................................................................*/
/* Static Local Functions                                            */
/*...................................................................*/

/*...................................................................*/
/* nonperiodic_release: release a non-periodic frame scheduler       */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*...................................................................*/
static void nonperiodic_release(FrameScheduler *base)
{
  FrameSchedulerNonPeriodic *scheduler =
                                     (FrameSchedulerNonPeriodic *)base;
  assert (scheduler != 0);

  scheduler->state = StateUnknown;
}

/*...................................................................*/
/* nonperiodic_start_split: initialize a start split frame scheduler */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*...................................................................*/
static void nonperiodic_start_split(FrameScheduler *base)
{
  FrameSchedulerNonPeriodic *scheduler =
                                     (FrameSchedulerNonPeriodic *)base;
  assert (scheduler != 0);

  scheduler->state = StartSplit;
}

/*...................................................................*/
/* nonperiodic_complete_split: inialize a complete split scheduler   */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*                                                                   */
/*    Returns: TRUE on success, FALSE if error                       */
/*...................................................................*/
static int nonperiodic_complete_split(FrameScheduler *base)
{
  FrameSchedulerNonPeriodic *scheduler =
                                     (FrameSchedulerNonPeriodic *)base;
  assert (scheduler != 0);

  int result = FALSE;

  switch (scheduler->state)
  {
    case StartSplitComplete:
      scheduler->state = CompleteSplit;
      scheduler->tries = 3;
      result = TRUE;
      break;

    case CompleteSplit:
    case CompleteRetry:
      result = TRUE;
      break;

    case CompleteSplitComplete:
    case CompleteSplitFailed:
      break;

    default:
      assert (0);
      break;
  }

  return result;
}

/*...................................................................*/
/* nonperiodic_transaction_complete: non-periodic transfer finished  */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*             status is the status result of the transfer           */
/*...................................................................*/
static void nonperiodic_transaction_complete(FrameScheduler *base,
                                             u32 status)
{
  FrameSchedulerNonPeriodic *scheduler =
                                     (FrameSchedulerNonPeriodic *)base;
  assert (scheduler != 0);

  switch (scheduler->state)
  {
    case StartSplit:
      assert(status & HC_INT_ACK);
      scheduler->state = StartSplitComplete;
      break;

    case CompleteSplit:
    case CompleteRetry:
      if (status & HC_INT_XFER_COMP)
      {
        scheduler->state = CompleteSplitComplete;
      }
      else if (status & (HC_INT_NYET | HC_INT_ACK))
      {
        if (scheduler->tries-- == 0)
          scheduler->state = CompleteSplitFailed;
        else
          scheduler->state = CompleteRetry;
      }
      else if (status & HC_INT_NAK)
      {
        if (scheduler->tries-- == 0)
          scheduler->state = CompleteSplitFailed;
        else
          scheduler->state = CompleteRetry;
      }
      else
      {
        puts("Invalid status");
        assert (0);
      }
      break;

    default:
      assert (0);
      break;
  }
}

/*...................................................................*/
/* nonperiodic_wait_for_frame: empty function for non-periodic       */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*...................................................................*/
static void nonperiodic_wait_for_frame(FrameScheduler *base)
{
}

/*...................................................................*/
/* nonperiodic_is_odd_frame: error function for non-periodic         */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*                                                                   */
/*    Returns: FALSE as unsupported                                  */
/*...................................................................*/
static int nonperiodic_is_odd_frame(FrameScheduler *base)
{
  return FALSE;
}

/*...................................................................*/
/* nosplit_release: empty function for no-split                      */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*...................................................................*/
static void nosplit_release(FrameScheduler *base)
{
}

/*...................................................................*/
/* nosplit_start_split: assert if no-split starts a split            */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*...................................................................*/
static void nosplit_start_split(FrameScheduler *base)
{
  assert(0);
}

/*...................................................................*/
/* nosplit_complete_split: error no-split completes a split          */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*                                                                   */
/*    Returns: FALSE for error                                       */
/*...................................................................*/
static int nosplit_complete_split(FrameScheduler *base)
{
  assert (0);
  return FALSE;
}

/*...................................................................*/
/* nosplit_start_split: assert if no-split completes                 */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*...................................................................*/
static void nosplit_transaction_complete(FrameScheduler *base,
                                         u32 status)
{
  assert (0);
}

/*...................................................................*/
/* nosplit_wait_for_frame: wait for no-split transaction to complete */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*...................................................................*/
static void nosplit_wait_for_frame(FrameScheduler *base)
{
  FrameSchedulerNoSplit *scheduler = (FrameSchedulerNoSplit *)base;

  assert (scheduler != 0);
  scheduler->nextFrame = (HFNUM_NUMBER(REG32(HFNUM)) + 1) &
                         DWC_HFNUM_MAX_FRNUM;

  if (!scheduler->isPeriodic)
  {
    while ((HFNUM_NUMBER(REG32(HFNUM)) & DWC_HFNUM_MAX_FRNUM) !=
           scheduler->nextFrame)
    {
      // do nothing
    }
  }

}

/*...................................................................*/
/* nosplit_is_odd_frame: Check if this frame is odd                  */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*                                                                   */
/*    Returns: TRUE if odd, FALSE if even                            */
/*...................................................................*/
static int nosplit_is_odd_frame(FrameScheduler *base)
{
  FrameSchedulerNoSplit *scheduler = (FrameSchedulerNoSplit *)base;
  assert (scheduler != 0);

  return scheduler->nextFrame & 1 ? TRUE : FALSE;
}

/*...................................................................*/
/* periodic_release: release a periodic frame scheduler              */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*...................................................................*/
static void periodic_release(FrameScheduler *base)
{
  FrameSchedulerPeriodic *scheduler = (FrameSchedulerPeriodic *) base;
  assert (scheduler != 0);

  scheduler->state = StateUnknown;
}

/*...................................................................*/
/* periodic_start_split: start a periodic split                      */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*...................................................................*/
static void periodic_start_split(FrameScheduler *base)
{
  FrameSchedulerPeriodic *scheduler = (FrameSchedulerPeriodic *) base;
  assert (scheduler != 0);

  scheduler->state = StartSplit;
  scheduler->nextFrame = FRAME_PERIODIC_MAX;
}

/*...................................................................*/
/* periodic_complete_split: Complete a periodic split transaction    */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*                                                                   */
/*    Returns: TRUE if sucess, FALSE if error                        */
/*...................................................................*/
static int periodic_complete_split(FrameScheduler *base)
{
  FrameSchedulerPeriodic *scheduler = (FrameSchedulerPeriodic *)base;
  assert (scheduler != 0);

  switch (scheduler->state)
  {
    case StartSplitComplete:
      scheduler->state = CompleteSplit;
      scheduler->tries = scheduler->nextFrame != 5 ? 3 : 2;
      scheduler->nextFrame = (scheduler->nextFrame  + 2) & 7;
      return TRUE;

    case CompleteRetry:
      scheduler->nextFrame = (scheduler->nextFrame + 1) & 7;
      return TRUE;
      break;

    case CompleteSplitComplete:
    case CompleteSplitFailed:
      break;

    default:
      assert (0);
      break;
  }

  return FALSE;
}

/*...................................................................*/
/* periodic_transaction_complete: Complete a periodic transaction    */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*             status is the completion status for the transaction   */
/*...................................................................*/
static void periodic_transaction_complete(FrameScheduler *base,
                                          u32 status)
{
  FrameSchedulerPeriodic *scheduler = (FrameSchedulerPeriodic *)base;
  assert(scheduler != 0);

  switch(scheduler->state)
  {
    case StartSplit:
      assert (status & HC_INT_ACK);
      scheduler->state = StartSplitComplete;
      break;

    case CompleteSplit:
    case CompleteRetry:
      if (status & HC_INT_XFER_COMP)
      {
        scheduler->state = CompleteSplitComplete;
      }
      else if (status & (HC_INT_NYET | HC_INT_ACK))
      {
        if (scheduler->tries-- == 0)
        {
          scheduler->state = CompleteSplitFailed;
        }
        else
        {
          scheduler->state = CompleteRetry;
        }
      }
      else if (status & HC_INT_NAK)
      {
        scheduler->state = CompleteSplitFailed;
      }
      else
        puts("Invalid status");
      break;

    default:
      assert (0);
      break;
  }
}

/*...................................................................*/
/* periodic_wait_for_frame: wait for periodic transaction            */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*...................................................................*/
static void periodic_wait_for_frame(FrameScheduler *base)
{
  FrameSchedulerPeriodic *scheduler = (FrameSchedulerPeriodic *) base;
  assert (scheduler != 0);

  if (scheduler->nextFrame == FRAME_PERIODIC_MAX)
  {
    scheduler->nextFrame = (HFNUM_NUMBER(REG32(HFNUM)) + 1) & 7;
    if (scheduler->nextFrame == 6)
    {
      scheduler->nextFrame++;
    }
  }

  while ((HFNUM_NUMBER(REG32(HFNUM)) & 7) != scheduler->nextFrame)
  {
    // do nothing
  }
}

/*...................................................................*/
/* periodic_is_odd_frame: Check if this frame is odd                 */
/*                                                                   */
/*      Input: base is a pointer to the frame scheduler              */
/*                                                                   */
/*    Returns: TRUE if odd, FALSE if even                            */
/*...................................................................*/
static int periodic_is_odd_frame(FrameScheduler *base)
{
  FrameSchedulerPeriodic *scheduler = (FrameSchedulerPeriodic *) base;
  assert (scheduler != 0);

  return scheduler->nextFrame & 1 ? TRUE : FALSE;
}

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* FrameSchedulerPeriodicInit: Initialize scheduler for periodic     */
/*                                                                   */
/*      Input: scheduler is a pointer to the frame scheduler         */
/*...................................................................*/
void FrameSchedulerPeriodicInit(FrameSchedulerPeriodic *scheduler)
{
  assert (scheduler != 0);

  FrameScheduler *base = (FrameScheduler *) scheduler;

  base->frameSchedulerRelease = periodic_release;
  base->startSplit = periodic_start_split;
  base->completeSplit = periodic_complete_split;
  base->transactionComplete = periodic_transaction_complete;
  base->waitForFrame = periodic_wait_for_frame;
  base->isOddFrame = periodic_is_odd_frame;

  scheduler->state = StateUnknown;
  scheduler->nextFrame = FRAME_PERIODIC_MAX;
}

/*...................................................................*/
/* FrameSchedulerNonPeriodicInit: Initialize scheduler non-periodic  */
/*                                                                   */
/*      Input: scheduler is a pointer to the frame scheduler         */
/*...................................................................*/
void FrameSchedulerNonPeriodicInit(
                                  FrameSchedulerNonPeriodic *scheduler)
{
  assert (scheduler != 0);

  FrameScheduler *base = (FrameScheduler *)scheduler;

  base->frameSchedulerRelease = nonperiodic_release;
  base->startSplit = nonperiodic_start_split;
  base->completeSplit = nonperiodic_complete_split;
  base->transactionComplete = nonperiodic_transaction_complete;
  base->waitForFrame = nonperiodic_wait_for_frame;
  base->isOddFrame = nonperiodic_is_odd_frame;

  scheduler->state = StateUnknown;
}

/*...................................................................*/
/* FrameSchedulerNoSplitInit: Initialize scheduler for no-split      */
/*                                                                   */
/*      Input: scheduler is a pointer to the frame scheduler         */
/*             isPeriodic is the value one (1) if periodic           */
/*...................................................................*/
void FrameSchedulerNoSplitInit(FrameSchedulerNoSplit *scheduler,
                               int isPeriodic)
{
  assert(scheduler != 0);

  FrameScheduler *base = (FrameScheduler *)scheduler;

  base->frameSchedulerRelease = nosplit_release;
  base->startSplit = nosplit_start_split;
  base->completeSplit = nosplit_complete_split;
  base->transactionComplete = nosplit_transaction_complete;
  base->waitForFrame = nosplit_wait_for_frame;
  base->isOddFrame = nosplit_is_odd_frame;

  scheduler->isPeriodic = isPeriodic;
  scheduler->nextFrame = (DWC_HFNUM_MAX_FRNUM + 1);// invalid/none
}

/*...................................................................*/
/* TransferStageDataAttach: Attach URB to a transfer stage data      */
/*                                                                   */
/*      Input: transfer is a pointer to the transfer stage data      */
/*             channel is the channel to use for the transfer        */
/*             urb is the channel to use for the transfer            */
/*             in is the direction, one (1) if in, zero (0) if out   */
/*             statusStage is the current stage of the data transfer */
/*...................................................................*/
void TransferStageDataAttach(TransferStageData *transfer, u32 channel,
                             void *urb, int in, int statusStage)
{
  assert (transfer != 0);

  transfer->channel = channel;
  transfer->urb = urb;
  transfer->in = in;
  transfer->statusStage = statusStage;
  transfer->splitComplete = FALSE;
  transfer->totalBytesTransfered = 0;
  transfer->state = 0;
  transfer->subState = 0;
  transfer->transactionStatus = 0;
  transfer->tempBuffer = 0;
  transfer->frameScheduler = 0;

  assert (transfer->urb != 0);

  transfer->endpoint = ((Request *)transfer->urb)->endpoint;
  assert (transfer->endpoint != 0);
  transfer->device = ((Endpoint *)transfer->endpoint)->device;
  assert (transfer->device != 0);

  transfer->speed = ((Device *)transfer->device)->speed;
  transfer->maxPacketSize =
                       ((Endpoint *)transfer->endpoint)->maxPacketSize;

  transfer->splitTransaction =
                     (((Device *)transfer->device)->hubAddress != 0) &&
                     (transfer->speed != SpeedHigh);

  if (!statusStage)
  {
    int size;

    if (EndpointGetNextPID(transfer->endpoint,statusStage) == PIDSetup)
    {
      transfer->bufferPointer =((Request *)transfer->urb)->setupData;
      transfer->transferSize = sizeof(SetupData);
    }
    else
    {
      transfer->bufferPointer = ((Request *)transfer->urb)->buffer;
      transfer->transferSize = ((Request *)transfer->urb)->bufLen;
    }

    for (transfer->packets = 0,
         size = transfer->transferSize + transfer->maxPacketSize - 1;
         size > transfer->maxPacketSize;
         size -= transfer->maxPacketSize)
      ++transfer->packets;

//    putchar(':');
//    putbyte(packets);
//    putchar(':');
//    putbyte((transfer->transferSize +
//             transfer->maxPacketSize - 1) / transfer->maxPacketSize);
//    putchar('\n');

    if (transfer->splitTransaction)
    {
      if (transfer->transferSize > transfer->maxPacketSize)
        transfer->bytesPerTransaction = transfer->maxPacketSize;
      else
        transfer->bytesPerTransaction = transfer->transferSize;

      transfer->packetsPerTransaction = 1;
    }
    else
    {
      transfer->bytesPerTransaction = transfer->transferSize;
      transfer->packetsPerTransaction = transfer->packets;
    }
  }
  else
  {
    assert (transfer->tempBuffer == 0);
    transfer->tempBuffer = &transfer->TempBuffer;
    assert (transfer->tempBuffer != 0);
    transfer->bufferPointer = transfer->tempBuffer;

    transfer->transferSize = 0;
    transfer->bytesPerTransaction = 0;
    transfer->packets = 1;
    transfer->packetsPerTransaction = 1;
  }

  assert (transfer->bufferPointer != 0);
  assert (((u32) transfer->bufferPointer & 3) == 0);

  if (transfer->splitTransaction)
  {
    if (TransferStageDataIsPeriodic (transfer))
    {
      transfer->frameScheduler =
                  (FrameScheduler *)&transfer->FrameScheduler.periodic;
      FrameSchedulerPeriodicInit(
                   (FrameSchedulerPeriodic *)transfer->frameScheduler);
    }
    else
    {
      transfer->frameScheduler =
               (FrameScheduler *)&transfer->FrameScheduler.nonperiodic;
      FrameSchedulerNonPeriodicInit(
                (FrameSchedulerNonPeriodic *)transfer->frameScheduler);
    }

    assert (transfer->frameScheduler != 0);
  }
  else
  {
    if (((Device *)transfer->device)->hubAddress == 0
        && transfer->speed != SpeedHigh)
    {
      transfer->frameScheduler =
                   (FrameScheduler *)&transfer->FrameScheduler.nosplit;
      FrameSchedulerNoSplitInit(
                     (FrameSchedulerNoSplit *)transfer->frameScheduler,
                     TransferStageDataIsPeriodic(transfer));
      assert(transfer->frameScheduler != 0);
    }
  }
}

/*...................................................................*/
/* TransferStageDataRelease: Release URB from a transfer stage data  */
/*                                                                   */
/*      Input: transfer is a pointer to the transfer stage data      */
/*...................................................................*/
void TransferStageDataRelease(TransferStageData *transfer)
{
  assert (transfer != 0);

  if (transfer->frameScheduler != 0)
  {
    transfer->frameScheduler->frameSchedulerRelease(
                                             transfer->frameScheduler);
    transfer->frameScheduler = 0;
  }

  transfer->bufferPointer = 0;

  if (transfer->tempBuffer != 0)
  {
    transfer->tempBuffer = 0;
  }

  transfer->endpoint = 0;
  transfer->device = 0;
  transfer->urb = 0;
}

/*...................................................................*/
/* TransferStageDataTransactionComplete: Complete a transfer stage   */
/*                                                                   */
/*      Input: transfer is a pointer to the transfer stage data      */
/*             status is the transaction status for the transfer     */
/*             packetsLeft is remaining packets in transfer          */
/*             bytesLeft is the remaining bytes in transfer          */
/*...................................................................*/
void TransferStageDataTransactionComplete(TransferStageData *transfer,
                         u32 status, u32 packetsLeft, u32 bytesLeft)
{
  u32 packetsTransfered =transfer->packetsPerTransaction - packetsLeft;
  u32 bytesTransfered = transfer->bytesPerTransaction - bytesLeft;

  assert(transfer != 0);

  transfer->transactionStatus = status;

  // If error or negative response, return
  if (status & (HC_INT_ERROR_MASK | HC_INT_NAK | HC_INT_NYET))
    return;

  if (transfer->splitTransaction && transfer->splitComplete &&
      (bytesTransfered == 0) && (transfer->bytesPerTransaction > 0))
  {
    bytesTransfered = transfer->maxPacketSize * packetsTransfered;
  }

  transfer->totalBytesTransfered += bytesTransfered;
  transfer->bufferPointer = (u8 *)transfer->bufferPointer +
                                  bytesTransfered;

  if (!transfer->splitTransaction || transfer->splitComplete)
    EndpointSkipPID(transfer->endpoint, packetsTransfered,
                    transfer->statusStage);

  assert(packetsTransfered <= transfer->packets);
  transfer->packets -= packetsTransfered;

  // Trim the bytes per transaction if less
  if (transfer->transferSize - transfer->totalBytesTransfered <
                                         transfer->bytesPerTransaction)
  {
    assert (transfer->totalBytesTransfered <= transfer->transferSize);
    transfer->bytesPerTransaction = transfer->transferSize -
                                        transfer->totalBytesTransfered;
  }
}

/*...................................................................*/
/* TransferStageDataIsPeriodic: Check if transfer stage is periodic  */
/*                                                                   */
/*      Input: transfer is a pointer to the transfer stage data      */
/*                                                                   */
/*    Returns: TRUE if periodic, FALSE otherwise                     */
/*...................................................................*/
int TransferStageDataIsPeriodic(TransferStageData *transfer)
{
  assert (transfer != 0);
  assert (transfer->endpoint != 0);
  EndpointType type = ((Endpoint *)transfer->endpoint)->type;

  return ((type == EndpointTypeInterrupt) ||
          (type == EndpointTypeIsochronous));
}

/*...................................................................*/
/* TransferStageDataGetDeviceAddress: Get transfer device address    */
/*                                                                   */
/*      Input: transfer is a pointer to the transfer stage data      */
/*                                                                   */
/*    Returns: The transfer device address                           */
/*...................................................................*/
u8 TransferStageDataGetEndpointType(TransferStageData *transfer)
{
  assert (transfer != 0);
  assert (transfer->endpoint != 0);

  u32 endpointType = 0;

  switch (((Endpoint *)transfer->endpoint)->type)
  {
    case EndpointTypeControl:
      endpointType = HC_CHAR_EP_TYPE_CONTROL;
      break;

    case EndpointTypeBulk:
      endpointType = HC_CHAR_EP_TYPE_BULK;
      break;

    case EndpointTypeInterrupt:
      endpointType = HC_CHAR_EP_TYPE_INT;
      break;

    default:
      assert (0);
      break;
  }

  return endpointType;
}

/*...................................................................*/
/* TransferStageDataGetPID: Get transfer PID                         */
/*                                                                   */
/*      Input: transfer is a pointer to the transfer stage data      */
/*                                                                   */
/*    Returns: PID on success, zero otherwise                        */
/*...................................................................*/
u8 TransferStageDataGetPID(TransferStageData *transfer)
{
  assert (transfer != 0);
  assert (transfer->endpoint != 0);

  u8 pid = 0;

  switch (EndpointGetNextPID(transfer->endpoint, transfer->statusStage))
  {
  case PIDSetup:
    pid = HC_T_SIZ_PID_SETUP;
    break;

  case PIDData0:
    pid = HC_T_SIZ_PID_DATA0;
    break;

  case PIDData1:
    pid = HC_T_SIZ_PID_DATA1;
    break;

  default:
    assert (0);
    break;
  }

  return pid;
}

/*...................................................................*/
/* TransferStageDataGetStatusMask: Get status mask for transfer      */
/*                                                                   */
/*      Input: transfer is a pointer to the transfer stage data      */
/*                                                                   */
/*    Returns: Bit mask for transfer status                          */
/*...................................................................*/
u32 TransferStageDataGetStatusMask(TransferStageData *transfer)
{
  u32 mask = HC_INT_XFER_COMP | HC_INT_CH_HLTD | HC_INT_ERROR_MASK;
  assert (transfer != 0);

  if (transfer->splitTransaction ||
      TransferStageDataIsPeriodic(transfer))
    mask |= HC_INT_ACK | HC_INT_NAK | HC_INT_NYET;

  return  mask;
}

/*...................................................................*/
/* TransferStageDataGetResultLen: Get transfer result length         */
/*                                                                   */
/*      Input: transfer is a pointer to the transfer stage data      */
/*                                                                   */
/*    Returns: Length of the transfer                                */
/*...................................................................*/
u32 TransferStageDataGetResultLen(TransferStageData *transfer)
{
  assert(transfer != 0);
  if (transfer->totalBytesTransfered > transfer->transferSize)
    return transfer->transferSize;
  return transfer->totalBytesTransfered;
}
#endif /* ENABLE_USB */
