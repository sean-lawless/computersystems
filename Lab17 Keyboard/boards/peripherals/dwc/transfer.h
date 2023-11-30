/*...................................................................*/
/*                                                                   */
/*   Module:  transfer.h                                             */
/*   Version: 2020.0                                                 */
/*   Purpose: DesignWare USB host transfer interface                 */
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
#ifndef TRANSFER_H
#define TRANSFER_H

#include <system.h>
#include <usb/usb.h>

/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
typedef struct FrameScheduler
{
  void (*frameSchedulerRelease)(struct FrameScheduler *scheduler);
  void (*startSplit)(struct FrameScheduler *scheduler);
  int (*completeSplit)(struct FrameScheduler *scheduler);
  void (*transactionComplete)(struct FrameScheduler *scheduler,
                              u32 status);
  void (*waitForFrame)(struct FrameScheduler *scheduler);
  int (*isOddFrame)(struct FrameScheduler *scheduler);
} FrameScheduler;

typedef struct FrameSchedulerPeriodic
{
  FrameScheduler frameScheduler; // must be first

  u32 state;
  u32 tries;

  u32 nextFrame;
} FrameSchedulerPeriodic;

typedef struct FrameSchedulerNonPeriodic
{
  FrameScheduler frameScheduler; // must be first

  u32 state;
  u32 tries;
} FrameSchedulerNonPeriodic;

typedef struct FrameSchedulerNoSplit
{
  FrameScheduler frameScheduler; // must be first

  int isPeriodic;
  u32 nextFrame;
} FrameSchedulerNoSplit;

typedef struct TransferStageData
{
  unsigned channel;
  void *urb;
  int in;
  int statusStage;

  int splitTransaction;
  int splitComplete;

  void *device;
  void *endpoint;
  Speed speed;
  u32 maxPacketSize;

  u32 transferSize;
  u32 packets;
  u32 bytesPerTransaction;
  u32 packetsPerTransaction;
  u32 totalBytesTransfered;

  u32 state;
  u32 subState;
  u32 transactionStatus;

  u32 *tempBuffer;
  u32 TempBuffer;
  void *bufferPointer;

  FrameScheduler *frameScheduler;
  union {
    FrameSchedulerPeriodic periodic;
    FrameSchedulerNonPeriodic nonperiodic;
    FrameSchedulerNoSplit nosplit;
  } FrameScheduler;
} TransferStageData;

/*...................................................................*/
/* Global Function Prototypes                                        */
/*...................................................................*/
void TransferStageDataAttach(TransferStageData *transfer, u32 nChannel,
                             void *urb, int in, int statusStage);
void TransferStageDataRelease(TransferStageData *transfer);
void TransferStageDataTransactionComplete(TransferStageData *transfer,
                        u32 nStatus, u32 nPacketsLeft, u32 nBytesLeft);
int TransferStageDataIsPeriodic(TransferStageData *transfer);
u8 TransferStageDataGetEndpointType(TransferStageData *transfer);
u8 TransferStageDataGetPID(TransferStageData *transfer);
u32 TransferStageDataGetStatusMask(TransferStageData *transfer);
u32 TransferStageDataGetResultLen(TransferStageData *transfer);

#endif /* TRANSFER_H */
