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
/* parser_attach: Attach and verify buffer with configuration parser */
/*                                                                   */
/*      Inputs: parser - the configuration parser                    */
/*              buffer - the buffer to attach                        */
/*              bufLen - the length of the buffer                    */
/*...................................................................*/
void parser_attach(ConfigurationParser *parser, const void *buffer,
                   u32 bufLen)
{
  const Descriptor *currentPosition;
  u8 lastDescType = 0;
  void *descEnd;

  assert (parser != 0);
  parser->buffer = (Descriptor *)buffer;
  parser->bufLen = bufLen;
  parser->valid = FALSE;
  parser->endPosition = ((u8 *)parser->buffer) + bufLen;
  parser->currentPosition = parser->buffer;

  assert(parser->buffer != 0);

  // Return if buffer length is invalid
  if ((parser->bufLen < 4) || (parser->bufLen > 512))
  {
    puts("Parser with invalid buffer length");
    return;
  }

  // Return if buffer is not a valid configuration descriptor
  if ((parser->buffer->Configuration.length !=
       sizeof(ConfigurationDescriptor)) ||
      (parser->buffer->Configuration.descriptorType !=
       DESCRIPTOR_CONFIGURATION) ||
      (parser->buffer->Configuration.totalLength >  bufLen))
  {
    puts("Not a valid configuration descriptor");
    return;
  }

  // Assign end position if valid
  if (parser->buffer->Configuration.totalLength < bufLen)
    parser->endPosition = ((u8 *)parser->buffer) +
                            parser->buffer->Configuration.totalLength;

  for (currentPosition = parser->buffer;
       ((u8 *)currentPosition + 2) < (u8 *)parser->endPosition;
       currentPosition = descEnd)
  {
    u8 expectedLen = 0;
    u8 descLen  = currentPosition->Header.length;
    u8 descType = currentPosition->Header.descriptorType;
    descEnd = ((u8 *)currentPosition + descLen);

    if (descEnd > parser->endPosition)
    {
      puts("Descriptor exceeds parser position");
      return;
    }

    switch (descType)
    {
      case DESCRIPTOR_CONFIGURATION:
        if (lastDescType != 0)
          return;
        expectedLen = sizeof(ConfigurationDescriptor);
        break;

      case DESCRIPTOR_INTERFACE:
        if (lastDescType == 0)
          return;
        expectedLen = sizeof(InterfaceDescriptor);
        break;

      case DESCRIPTOR_ENDPOINT:
        if ((lastDescType == 0) ||
            (lastDescType == DESCRIPTOR_CONFIGURATION))
          return;
        expectedLen = sizeof(EndpointDescriptor);
        break;

      default:
        break;
    }

    if ((expectedLen != 0) && (descLen != expectedLen))
    {
      puts("Descriptor length invalid");
      return;
    }

    lastDescType = descType;
    currentPosition = descEnd;
  }

  if (currentPosition != parser->endPosition)
  {
    puts("Current position not the end");
    return;
  }

  // Configuration descriptor is valid
  parser->valid = TRUE;
}

/*...................................................................*/
/* parser_release: Release a buffer from a configuration parser      */
/*                                                                   */
/*      Inputs: parser - the configuration parser                    */
/*...................................................................*/
void parser_release(ConfigurationParser *parser)
{
  assert(parser != 0);
  parser->buffer = 0;
  parser->valid = FALSE;
}

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
  if (!device->configDesc)
    goto finished;

  // Assign and attach the configuration parser
  assert(device->configParser == 0);
  device->configParser = &(device->ConfigParser);
  assert(device->configParser != 0);
  parser_attach(device->configParser, device->configDesc,
                device->configDesc->totalLength);

  if (!device->configParser->valid)
  {
    puts("USB device configuration parser not valid");
    goto finished;
  }

  // Complete the device initialization
  if (device->complete)
  {
    // If completion callback registered, call it
    if (device->comp_dev)
      device->complete(urb, device->comp_dev, context);

    // Otherwise call the default completion callback
    else
      device->complete(urb, param, context);
  }

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

  device->configure = DeviceConfigure;

  device->host = host;
  device->address = DEFAULT_ADDRESS;
  device->speed = speed;
  device->endpoint0 = 0;
  device->hubAddress = hubAddress;
  device->hubPortNumber = hubPortNumber;
  device->deviceDesc = 0;
  device->configDesc = 0;
  device->configParser = 0;
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

  bzero(device->manufacturerString,sizeof(device->manufacturerString));
  bzero(device->productString, sizeof(device->productString));
}

/*...................................................................*/
/* DeviceRelease: Release a device from a host                       */
/*                                                                   */
/*      Inputs: device is the device to detach or release            */
/*...................................................................*/
void DeviceRelease(Device *device)
{
  assert(device != 0);

  if (device->configParser != 0)
  {
    parser_release(device->configParser);
    device->configParser = 0;
  }

  if (device->configDesc != 0)
  {
    device->configDesc = 0;
  }

  if (device->deviceDesc != 0)
  {
    device->deviceDesc = 0;
  }

  if (device->endpoint0 != 0)
  {
    EndpointRelease(device->endpoint0);
    device->endpoint0 = 0;
  }

  device->configure = 0;
  device->host = 0;

  bzero(device->manufacturerString,sizeof(device->manufacturerString));
  bzero(device->productString, sizeof(device->productString));
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

  to->configure = from->configure;

  to->endpoint0 = 0;
  to->deviceDesc = 0;
  to->configDesc = 0;
  to->configParser = 0;

  to->host = from->host;
  to->address = from->address;
  to->speed = from->speed;
  to->hubAddress = from->hubAddress;
  to->hubPortNumber = from->hubPortNumber;
  to->comp_dev = from->comp_dev;
  to->complete = from->complete;

  memcpy(to->manufacturerString, from->manufacturerString, 20);
  memcpy(to->productString, from->productString, 20);

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

  if (from->configDesc != 0)
  {
    unsigned totalLength = from->configDesc->totalLength;
    assert (totalLength <= MAX_CONFIG_DESC_SIZE);

    to->configDesc = (void *)(((uintptr_t)&to->ConfigDesc) +
                     (16 - (((uintptr_t)&to->ConfigDesc) & 15)));
    assert (to->configDesc != 0);

    memcpy (to->configDesc, from->configDesc, totalLength);

    if (from->configParser != 0)
    {
      to->configParser = &(to->ConfigParser);
      assert (to->configParser != 0);
      parser_attach(to->configParser, to->configDesc, totalLength);
    }
  }
}

/*...................................................................*/
/* DeviceGetDescriptor: Look up a descriptor type from the parser    */
/*                                                                   */
/*      Inputs: parser is the attached configuration parser          */
/*              type is the descriptor type to get                   */
/*                                                                   */
/*     Returns: pointer to descriptor of type, or NULL               */
/*...................................................................*/
const Descriptor *DeviceGetDescriptor(ConfigurationParser *parser,
                                      u8 type)
{
  const Descriptor *result = NULL;
  Descriptor *descEnd;
  u8 descLen, descType;

  assert(parser != 0);
  assert(parser->valid);


//  printf("DeviceGetDescriptor curr %d < end %d\n",
//         parser->currentPosition, parser->endPosition);
  while ((void *)parser->currentPosition < parser->endPosition)
  {
    descLen  = parser->currentPosition->Header.length;
    descType = parser->currentPosition->Header.descriptorType;

    descEnd = (void *)((u8 *)parser->currentPosition) + descLen;
    //SKIP_BYTES (parser->currentPosition, descLen);
    assert((void *)descEnd <= parser->endPosition);

    if ((type == DESCRIPTOR_ENDPOINT) &&
        (descType == DESCRIPTOR_INTERFACE))
    {
      break;
    }

    if (descType == type)
    {
      result = parser->currentPosition;
      parser->currentPosition = descEnd;
      break;
    }

    parser->currentPosition = descEnd;
  }

  return result;
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
                                nextAddress, DeviceInitialize, device))
    {
      puts("Cannot set address");
      device->state = STATE_GET_DESCRIPTOR_LENGTH;
      return;
    }
    device->state = STATE_GET_CONFIGURATION_LENGTH;
  }
  else if (device->state == STATE_GET_CONFIGURATION_LENGTH)
  {
    // Assign unique address and configuration descriptor
    device->address = nextAddress++;
    assert(device->configDesc == 0);
    device->configDesc = (void *)(((uintptr_t)&device->ConfigDesc) +
                       (16 - (((uintptr_t)&device->ConfigDesc) & 15)));
    assert(device->configDesc != 0);
    if (HostGetEndpointDescriptor(device->host, device->endpoint0,
              DESCRIPTOR_CONFIGURATION, INDEX_DEFAULT,
              device->configDesc, sizeof(*device->configDesc),
              REQUEST_IN, DeviceInitialize, device) <= 0)
    {
      puts("Cannot get configuration descriptor (short)");
      device->configDesc = 0;
      device->state = STATE_GET_DESCRIPTOR_LENGTH; //restart state
      return;
    }
    device->state = STATE_GET_CONFIGURATION;
  }
  else if (device->state == STATE_GET_CONFIGURATION)
  {
    assert(device->endpoint0 != 0);
    if ((device->configDesc->length != sizeof(*device->configDesc)) ||
     (device->configDesc->descriptorType !=DESCRIPTOR_CONFIGURATION) ||
        (device->configDesc->totalLength > MAX_CONFIG_DESC_SIZE))
    {
      puts("Invalid configuration descriptor");
      device->configDesc = 0;

      device->state = STATE_GET_DESCRIPTOR_LENGTH;
      return;
    }

    // Assign unique address and configuration descriptor
    device->configDesc = (void *)(((uintptr_t)&device->ConfigDesc) +
                       (16 - (((uintptr_t)&device->ConfigDesc) & 15)));
    assert(device->configDesc != 0);
    if (HostGetEndpointDescriptor(device->host, device->endpoint0,
              DESCRIPTOR_CONFIGURATION, INDEX_DEFAULT,
              device->configDesc, device->configDesc->totalLength,
              REQUEST_IN, initialize_complete, device) <= 0)
    {
      puts("Cannot get configuration descriptor");
      device->configDesc = 0;
      device->state = STATE_GET_DESCRIPTOR_LENGTH; //restart state
      return;
    }
    device->state = STATE_GET_DESCRIPTOR_LENGTH; //reset state
  }
}

/*...................................................................*/
/* DeviceConfigure: Configure an initialized device                  */
/*                                                                   */
/*      Inputs: device is the device to configure                    */
/*              complete is the specific, or parent, device callback */
/*              param is the specific, or parent, device structure   */
/*                                                                   */
/*     Returns: pointer to descriptor of type, or NULL               */
/*...................................................................*/
int DeviceConfigure(Device *device, void (complete)
                  (void *urb, void *param, void *context), void *param)
{
  assert(device != 0);
  assert(device->host != 0);
  assert(device->endpoint0 != 0);

  // Error if configuration descriptor not assigned
  if (device->configDesc == 0)
    return FALSE;

  // Set to the current configuration and pass the callers complete
  // function and parameter for execution afterward
  if (!HostSetEndpointConfiguration(device->host, device->endpoint0,
                             device->configDesc->configurationValue,
                             complete, param))
  {
    puts("Failed to set configuration");
    return FALSE;
  }

  return TRUE;
}

#endif /* ENABLE_USB */
