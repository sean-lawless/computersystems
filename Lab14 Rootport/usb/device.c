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
#include <usb/request.h>
#include <usb/device.h>
#include <usb/endpoint.h>
#include <board.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#if ENABLE_USB

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
#define STATE_GET_DESCRIPTOR_LENGTH     0
#define STATE_GET_DESCRIPTOR            1
#define STATE_SET_ENDPOINT_ADDRESS      2
#define STATE_GET_CONFIGURATION_LENGTH  3
#define STATE_GET_CONFIGURATION         4

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
u8 nextAddress;

/*...................................................................*/
/* Static Local Functions                                            */
/*...................................................................*/

/*...................................................................*/
/* initialize_complete: The complete callback for device initialize  */
/*                                                                   */
/*      Inputs: urb - the USB Request Buffer                         */
/*              param - the USB device                               */
/*              context - unused                                     */
/*...................................................................*/
static void initialize_complete(void *urb, void *param, void *context)
{
  Device *device = param;

  // If configuration descriptor not present, fail configuration
  if (!device->deviceDesc)
    goto finished;

  printf("Device detected, descriptor type %d (%d) with endpoint length %d\n",
         device->deviceDesc->descriptorType,
         device->deviceDesc->length,
         device->endpoint0->maxPacketSize);

finished:
  // Free the urb
  if (urb)
    FreeRequest(urb);

  return;
}

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
  device->endpoint0 = 0;
  device->hubAddress = hubAddress;
  device->hubPortNumber = hubPortNumber;
  device->deviceDesc = 0;
  device->comp_dev = NULL;
  device->complete = NULL;

  device->state = STATE_GET_DESCRIPTOR_LENGTH;

  assert (device->host != 0);

  // Initialize the control endpoint
  assert(device->endpoint0 == 0);
  device->endpoint0 = &(device->Endpoint0);
  assert(device->endpoint0 != 0);
  EndpointControl(device->endpoint0, device);

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

  if (device->deviceDesc != 0)
  {
    device->deviceDesc = 0;
  }

  if (device->endpoint0 != 0)
  {
    EndpointRelease(device->endpoint0);
    device->endpoint0 = 0;
  }

  device->host = 0;
}

/*...................................................................*/
/*  DeviceCopy: Copy details from one device to another              */
/*                                                                   */
/*      Inputs: to is the device copy destination                    */
/*              from is the device copy source                       */
/*...................................................................*/
void DeviceCopy(Device *to, Device *from)
{
  assert(to != 0);
  assert(from != 0);

  to->endpoint0 = 0;
  to->deviceDesc = 0;

  to->host = from->host;
  to->address = from->address;
  to->speed = from->speed;
  to->hubAddress = from->hubAddress;
  to->hubPortNumber = from->hubPortNumber;
  to->comp_dev = from->comp_dev;
  to->complete = from->complete;

  to->endpoint0 = &(to->Endpoint0);
  assert (to->endpoint0 != 0);

  EndpointCopy(to->endpoint0, from->endpoint0, to);

  if (from->deviceDesc != 0)
  {
    to->deviceDesc = (void *)(((uintptr_t)&to->DeviceDesc) +
                     (16 - (((uintptr_t)&to->DeviceDesc) & 15)));
    assert(to->deviceDesc != 0);
    memcpy(to->deviceDesc, from->deviceDesc, sizeof(DeviceDescriptor));
  }
}

/*...................................................................*/
/* DeviceInitialize: Aysnchr state machine to initialize a device    */
/*                                                                   */
/*      Inputs: urb is unused                                        */
/*              param is a pointer to the device                     */
/*              context is unused                                    */
/*...................................................................*/
void DeviceInitialize(void *urb, void *param, void *context)
{
  Device *device = param;

  assert(device != 0);
  assert(device->host != 0);
  assert(device->endpoint0 != 0);

  //always free the last URB before sending a new one
  if (urb)
    FreeRequest(urb);

//  printf("In DeviceInitialize with state %d\n", device->state);
  if (device->state == STATE_GET_DESCRIPTOR_LENGTH)
  {
    // Calculate 16 byte aligned device descriptor pointer
    assert (device->deviceDesc == 0);
    device->deviceDesc = (void *)(((uintptr_t)&device->DeviceDesc) +
                     (16 - (((uintptr_t)&device->DeviceDesc) & 15)));
    assert (sizeof *device->deviceDesc >= MAX_PACKET_SIZE);

    // Submit USB request to read the device descriptor
    if (HostGetEndpointDescriptor(device->host, device->endpoint0,
              DESCRIPTOR_DEVICE, INDEX_DEFAULT,
              device->deviceDesc, MAX_PACKET_SIZE,
              REQUEST_IN, DeviceInitialize, device) <= 0)
    {
      puts("Cannot get device descriptor (short)");
      device->deviceDesc = 0;
      device->state = STATE_GET_DESCRIPTOR_LENGTH;
      return;
    }

    // Increment the device configuration state
    device->state = STATE_GET_DESCRIPTOR;
  }
  else if (device->state == STATE_GET_DESCRIPTOR)
  {
    assert(device->deviceDesc != 0);

    if ((device->deviceDesc->length != sizeof(*device->deviceDesc)) ||
        (device->deviceDesc->descriptorType != DESCRIPTOR_DEVICE))
    {
      printf("Invalid device descriptor (0x%x, 0x%x) len %d, type %d",
             device, device->deviceDesc, device->deviceDesc->length,
             device->deviceDesc->descriptorType);
      device->deviceDesc = 0;

      device->state = STATE_GET_DESCRIPTOR_LENGTH;
      return;
    }

    // Save the descriptor maximum packet size
    device->endpoint0->maxPacketSize =
                                     device->deviceDesc->maxPacketSize;

    if (HostGetEndpointDescriptor(device->host, device->endpoint0,
              DESCRIPTOR_DEVICE, INDEX_DEFAULT,
              device->deviceDesc, sizeof(*device->deviceDesc),
              REQUEST_IN, DeviceInitialize, device) <= 0)
    {
      puts("Cannot get device descriptor");
      device->deviceDesc = 0;
      device->state = STATE_GET_DESCRIPTOR_LENGTH;
      return;
    }

    //increase state
    device->state = STATE_SET_ENDPOINT_ADDRESS;
  }
  else if (device->state == STATE_SET_ENDPOINT_ADDRESS)
  {
    if (nextAddress > MAX_ADDRESS)
    {
      puts("Too many devices");
      device->state = 0;
      return;
    }

    if (!HostSetEndpointAddress(device->host, device->endpoint0,
                                nextAddress, initialize_complete, device))
    {
      puts("Cannot set address");
      device->state = STATE_GET_DESCRIPTOR_LENGTH;
      return;
    }
    device->state = STATE_GET_DESCRIPTOR_LENGTH; //reset state
  }
}

#endif /* ENABLE_USB */
