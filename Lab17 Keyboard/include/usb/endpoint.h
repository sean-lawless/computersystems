/*.....................................................................*/
/*                                                                     */
/*   Module:  endpoint.h                                               */
/*   Version: 2020.0                                                   */
/*   Purpose: Header declarations for USB endpoints                    */
/*                                                                     */
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
#ifndef _ENDPOINT_H
#define _ENDPOINT_H

#include <usb/usb.h>
#include <system.h>

/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
typedef enum
{
  EndpointTypeControl,
  EndpointTypeBulk,
  EndpointTypeInterrupt,
  EndpointTypeIsochronous
}
EndpointType;

struct Device;

typedef struct Endpoint
{
  struct Device *device;
  int number;
  EndpointType type;
  int directionIn;
  u32 maxPacketSize;
  u32 interval; // ms
  PID nextPID;
} Endpoint;

/*...................................................................*/
/* Function Prototypes                                               */
/*...................................................................*/
void EndpointControl(Endpoint *endpoint, struct Device *device);
void EndpointFromDescr(Endpoint *endpoint, struct Device *device,
                       const EndpointDescriptor *desc);
void EndpointRelease(Endpoint *endpoint);
void EndpointCopy(Endpoint *dst, Endpoint *src, struct Device *device);
PID EndpointGetNextPID(Endpoint *endpoint, int statusStage);
void EndpointSkipPID(Endpoint *endpoint, u32 packets, int statusStage);
void EndpointResetPID(Endpoint *endpoint);

#endif
