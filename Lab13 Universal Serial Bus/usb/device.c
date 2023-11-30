/*...................................................................*/
/*                                                                   */
/*   Module:  device.c                                               */
/*   Version: 2020.0                                                 */
/*   Purpose: USB Device                                             */
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
#include <board.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#if ENABLE_USB

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
u8 nextAddress;

/*...................................................................*/
/* Static Local Functions                                            */
/*...................................................................*/

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* DeviceInit: Initialize the device algorithm                       */
/*                                                                   */
/*...................................................................*/
void DeviceInit(void)
{
  nextAddress = FIRST_ADDRESS;
}

/*...................................................................*/
/* DeviceAttach: Attach a device to a host hub port                  */
/*                                                                   */
/*      Inputs: device is the device to attach                       */
/*              host is the host to attach to                        */
/*              speed is the port speed                              */
/*              hubAddress is the attached hub address               */
/*              hubPortNumber is the attached hub port               */
/*...................................................................*/
void DeviceAttach(Device *device, struct Host *host, Speed speed,
                  u8 hubAddress, u8 hubPortNumber)
{
  assert(device != 0);

  device->host = host;
  device->address = DEFAULT_ADDRESS;
  device->speed = speed;
  device->hubAddress = hubAddress;
  device->hubPortNumber = hubPortNumber;

  device->state = 0;

  assert (device->host != 0);

  assert(hubPortNumber >= 1);
}

/*...................................................................*/
/* DeviceRelease: Release a device from a host                       */
/*                                                                   */
/*      Inputs: device is the device to detach or release            */
/*...................................................................*/
void DeviceRelease(Device *device)
{
  assert(device != 0);

  device->host = 0;
}

#endif /* ENABLE_USB */
