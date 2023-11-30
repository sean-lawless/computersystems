/*...................................................................*/
/*                                                                   */
/*   Module:  endpoint.c                                             */
/*   Version: 2020.0                                                 */
/*   Purpose: USB Endpoint                                           */
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
#include <usb/device.h>
#include <usb/endpoint.h>
#include <assert.h>

#if ENABLE_USB

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* EndpointControl: Initialize the device control endpoint           */
/*                                                                   */
/*      Inputs: endpoint is the endpoint to initialize               */
/*              device is the owner of the control endpoint          */
/*...................................................................*/
void EndpointControl(Endpoint *endpoint, Device *device)
{
  assert(endpoint != 0);
  endpoint->device = device;
  endpoint->number = 0;
  endpoint->type = EndpointTypeControl;
  endpoint->directionIn = FALSE;
  endpoint->maxPacketSize = MAX_PACKET_SIZE;
  endpoint->interval = 1;
  endpoint->nextPID = PIDSetup;

  assert (endpoint->device != 0);
}

/*...................................................................*/
/* EndpointFromDescr: Initialize an endpoint based on a descriptor   */
/*                                                                   */
/*      Inputs: endpoint is the endpoint to initialize               */
/*              device is the owner of the control endpoint          */
/*              desc is the descriptor of the endpoint               */
/*...................................................................*/
void EndpointFromDescr(Endpoint *endpoint, Device *device,
                       const EndpointDescriptor *desc)
{
  u8 interval;

  assert(endpoint != 0);
  endpoint->device = device;
  endpoint->interval = 1;

  assert(endpoint->device != 0);
  assert(desc != 0);
  assert(desc->length == sizeof *desc);
  assert(desc->descriptorType == DESCRIPTOR_ENDPOINT);

  switch (desc->attributes & 0x03)
  {
    case 2:
      endpoint->type = EndpointTypeBulk;
      endpoint->nextPID = PIDData0;
      break;

    case 3:
      endpoint->type = EndpointTypeInterrupt;
      endpoint->nextPID = PIDData0;
      break;

    default:
      assert(0); // endpoint type not supported
      return;
  }

  endpoint->number = desc->endpointAddress & 0x0F;
  endpoint->directionIn = desc->endpointAddress & 0x80 ? TRUE : FALSE;
  endpoint->maxPacketSize = ((desc->maxPacketSize2 << 8) +
                             desc->maxPacketSize);

  if (endpoint->type == EndpointTypeInterrupt)
  {
    interval = desc->interval;
    if (interval < 1)
      interval = 1;

    // USB 2.0 specification chapter 9.6.6
    if (endpoint->device->speed != SpeedHigh)
    {
      endpoint->interval = interval;
    }
    else
    {
      u32 value;

      // Trim interval if it exceeds maximum
      if (interval > 16)
        interval = 16;

      // Calculate value of interval and assign to endpoint
      value = 1 << (interval - 1);
      endpoint->interval = value / 8;

      // If result is less than zero, set to one (1)
      if (endpoint->interval < 1)
        endpoint->interval = 1;
    }
  }
}

/*...................................................................*/
/* EndpointCopy: Copy one endpoint configuration to another          */
/*                                                                   */
/*      Inputs: dst is the destination (copy to) endpoint            */
/*              src is the source (copy from) endpoint               */
/*              device is the destination endpoint device            */
/*...................................................................*/
void EndpointCopy(Endpoint *dst, Endpoint *src, Device *device)
{
  assert (dst != 0);
  assert (src != 0);

  dst->device = device;
  assert(dst->device != 0);

  dst->number  = src->number;
  dst->type    = src->type;
  dst->directionIn  = src->directionIn;
  dst->maxPacketSize  = src->maxPacketSize;
  dst->interval       = src->interval;
  dst->nextPID   = src->nextPID;
}

/*...................................................................*/
/* EndpointRelease: Release an endpoint that is no longer needed     */
/*                                                                   */
/*      Inputs: endpoint is the endpoint to release                  */
/*...................................................................*/
void EndpointRelease(Endpoint *endpoint)
{
  assert(endpoint != 0);
  endpoint->device = 0;
}

/*...................................................................*/
/* EndpointGetNextPID: Return the next PID (see USB 2.0 spec)        */
/*                                                                   */
/*      Inputs: endpoint is the endpoint to retrieve PID for         */
/*              statusStage is the status of the stage data          */
/*...................................................................*/
PID EndpointGetNextPID(Endpoint *endpoint, int statusStage)
{
  assert(endpoint != 0);
  if (statusStage)
  {
    assert(endpoint->type == EndpointTypeControl);
    return PIDData1;
  }

  return endpoint->nextPID;
}

/*...................................................................*/
/* EndpointSkipPID: Skip the PID ahead after a successful transfer   */
/*                                                                   */
/*      Inputs: endpoint is the endpoint to retrieve PID for         */
/*              packets is the number of packets to skip ahead       */
/*              statusStage is the status of the stage data          */
/*...................................................................*/
void EndpointSkipPID(Endpoint *endpoint, u32 packets, int statusStage)
{
  assert(endpoint != 0);
  assert((endpoint->type == EndpointTypeControl)||
         (endpoint->type == EndpointTypeBulk) ||
         (endpoint->type == EndpointTypeInterrupt));

  if (!statusStage)
  {
    switch (endpoint->nextPID)
    {
    case PIDSetup:
      endpoint->nextPID = PIDData1;
      break;

    case PIDData0:
      if (packets & 1)
        endpoint->nextPID = PIDData1;
      break;

    case PIDData1:
      if (packets & 1)
        endpoint->nextPID = PIDData0;
      break;

    default:
      assert (0);
      break;
    }
  }
  else
  {
    assert(endpoint->type == EndpointTypeControl);
    endpoint->nextPID = PIDSetup;
  }
}

/*...................................................................*/
/* EndpointResetPID: Reset the PID for an endpoint                   */
/*                                                                   */
/*      Inputs: endpoint is the endpoint to retrieve PID for         */
/*...................................................................*/
void EndpointResetPID(Endpoint *endpoint)
{
  assert(endpoint != 0);
  assert(endpoint->type == EndpointTypeBulk);

  endpoint->nextPID = PIDData0;
}

#endif /* ENABLE_USB */
