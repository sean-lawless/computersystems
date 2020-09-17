/*...................................................................*/
/*                                                                   */
/*   Module:  mouse.c                                                */
/*   Version: 2020.0                                                 */
/*   Purpose: USB Standard Mouse Device driver                       */
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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <usb/hid.h>
#include <usb/request.h>
#include <usb/device.h>

#if ENABLE_USB_HID

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
#define MOUSE_REPORT_SIZE  3

/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
typedef void MouseStatusHandler(u32 buttons, int displacementX,
                                int displacementY);

typedef struct MouseDevice
{
  Device device; // Must be first element in this structure

  u8 interfaceNumber;
  u8 alternateSetting;

  Endpoint *reportEndpoint;
  Endpoint *interruptEndpoint;
  Endpoint ReportEndpoint;
  Endpoint InterruptEndpoint;

  MouseStatusHandler *statusHandler;

  Request *urb;
  u8 reportBuffer[MOUSE_REPORT_SIZE];
}
MouseDevice;

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
MouseDevice Mouse;
MouseDevice *Mouse1;
int MouseInitState = 0, MouseEnabled = 0;

extern int MouseUp(const char *command);

/*...................................................................*/
/* Static Local Functions                                            */
/*...................................................................*/

/*...................................................................*/
/* status_update: Mouse status change update callback                */
/*                                                                   */
/*      Inputs: buttons is the change in buttons                     */
/*              displacementY is the mouse change in Y direction     */
/*              displacementX is the mouse change in X direction     */
/*...................................................................*/
static void status_update(u32 buttons, int displacementX,
                          int displacementY)
{
#if ENABLE_PRINTF
  printf("(%db%dx%dy)", buttons, displacementX, displacementY);
#else
  Uart0State.putc('.');
#endif
}

/*...................................................................*/
/* configure_complete: State machine to configure a mouse            */
/*                                                                   */
/*      Inputs: urb is USB Request Buffer (URB)                      */
/*              param is the mouse device                            */
/*              context is unused                                    */
/*...................................................................*/
static void configure_complete(void *urb, void *param, void *pContext)
{
  MouseDevice *mouse = (MouseDevice *)param;

  //always free the last URB before sending a new one
  if (urb)
    FreeRequest(urb);

  if ((mouse->alternateSetting != 0) && (MouseInitState == 0))
  {
    puts("Mouse alternate settings is not zero!");
    if (HostEndpointControlMessage (mouse->device.host,
          mouse->device.endpoint0,
          REQUEST_OUT | REQUEST_TO_INTERFACE, SET_INTERFACE,
          mouse->alternateSetting,
          mouse->interfaceNumber, 0, 0, configure_complete, mouse) < 0)
    {
      puts("Cannot set interface");

      return;
    }
    MouseInitState = 1;
  }
  else if (MouseInitState == 2)
  {
    Mouse1 = mouse;
    MouseInitState = 0; //reset upon success
    if (mouse->device.complete)
    {
      mouse->device.complete(NULL, mouse->device.comp_dev, NULL);
      mouse->device.complete = NULL;
    }

#if ENABLE_AUTO_START
    // Automatically start the mouse for the user if configured
    MouseUp(NULL);
#endif
   }
  else
  {
    if (HostEndpointControlMessage (mouse->device.host,
          mouse->device.endpoint0, REQUEST_OUT | REQUEST_CLASS |
          REQUEST_TO_INTERFACE, SET_PROTOCOL, BOOT_PROTOCOL,
          mouse->interfaceNumber, 0, 0, configure_complete, mouse) < 0)
    {
      puts("Cannot set boot protocol");

      // Reset mouse configuraiton state upon failure
      MouseInitState = 0;
      return;
    }

    // Set mouse to final configuration state
    MouseInitState = 2;
  }
}

/*...................................................................*/
/*   configure: Start mouse device configuration                     */
/*                                                                   */
/*      Inputs: device is keyboard device                            */
/*              complete is callback function invoked upon completion*/
/*              param is unused                                      */
/*...................................................................*/
static int configure(Device *device, void (complete)
                  (void *urb, void *param, void *context), void *param)
{
  MouseDevice *mouse = (MouseDevice *)device;
  InterfaceDescriptor *interfaceDesc;
  ConfigurationDescriptor *pConfDesc;
  EndpointDescriptor *endpointDesc;

  assert (mouse != 0);
  pConfDesc = (ConfigurationDescriptor *)DeviceGetDescriptor(
                  mouse->device.configParser, DESCRIPTOR_CONFIGURATION);

  if ((pConfDesc == 0) || (pConfDesc->numInterfaces <  1))
  {
    puts("USB mouse configuration descriptor not valid");
    return FALSE;
  }

  while ((interfaceDesc = (InterfaceDescriptor *)
         DeviceGetDescriptor(mouse->device.configParser,
                                            DESCRIPTOR_INTERFACE)) != 0)
  {
    if ((interfaceDesc->numEndpoints <  1) ||
        (interfaceDesc->interfaceClass != 0x03) || // HID Class
        (interfaceDesc->interfaceSubClass != 0x01) || // Boot Subclass
        (interfaceDesc->interfaceProtocol != 0x02))   // Mouse
    {
      continue;
    }

    mouse->interfaceNumber  = interfaceDesc->interfaceNumber;
    mouse->alternateSetting = interfaceDesc->alternateSetting;

    endpointDesc = (EndpointDescriptor *)DeviceGetDescriptor(
                       mouse->device.configParser, DESCRIPTOR_ENDPOINT);
    // Skip if empty of input endpoint
    if ((endpointDesc == 0) ||
        (endpointDesc->endpointAddress & 0x80) != 0x80)
      continue;

    // Assign interrupt endpoint if attribute allows
    if ((endpointDesc->attributes & 0x3F) == 0x03)
    {
      assert (mouse->interruptEndpoint == 0);
      mouse->interruptEndpoint = &(mouse->InterruptEndpoint);
      assert (mouse->interruptEndpoint != 0);
      EndpointFromDescr(mouse->interruptEndpoint, &mouse->device,
                        endpointDesc);
    }

    // Otherwise use the report endpoint
    else
    {
      assert (mouse->reportEndpoint == 0);
      mouse->reportEndpoint = &(mouse->ReportEndpoint);
      assert (mouse->reportEndpoint != 0);
      EndpointFromDescr(mouse->reportEndpoint, &mouse->device,
                        endpointDesc);
    }

    continue;
  }

  if ((mouse->reportEndpoint == 0) && (mouse->interruptEndpoint == 0))
  {
    puts("USB mouse endpoint not found");
    return FALSE;
  }

  if (!DeviceConfigure (&mouse->device, configure_complete, mouse))
  {
    puts("Cannot set configuration");
    return FALSE;
  }

  return TRUE;
}

/*...................................................................*/
/*  completion: Mouse URB completion parses mouse events             */
/*                                                                   */
/*       Input: urb is the completed USB Request Buffer (URB)        */
/*              param is the mouse device                            */
/*              context is unused                                    */
/*...................................................................*/
static void completion(void *request, void *param, void *context)
{
  Request *urb = request;
  MouseDevice *mouse = (MouseDevice *)context;

  assert(mouse != 0);
  assert(urb != 0);
  assert (mouse->urb == urb);

  if ((urb->status != 0) && (urb->resultLen == MOUSE_REPORT_SIZE) &&
      (mouse->statusHandler != 0))
  {
    assert (mouse->reportBuffer != 0);

    // Call report handler with x and y mouse movement offsets
    (*mouse->statusHandler) (mouse->reportBuffer[0],
              (mouse->reportBuffer[1] > 0x7F) ?
              ((int)mouse->reportBuffer[1] | -0x100) :
              (int)mouse->reportBuffer[1],
              (mouse->reportBuffer[2] > 0x7F) ?
              ((int)mouse->reportBuffer[2] | -0x100) :
              (int)mouse->reportBuffer[2]);
  }

  // Reuse the URB
  RequestRelease(mouse->urb);

  /* Attach URB to the device, preferring the interrupt endpoint. */
  if (mouse->interruptEndpoint)
    RequestAttach(mouse->urb, mouse->interruptEndpoint,
                  mouse->reportBuffer, MOUSE_REPORT_SIZE, 0);
  else if (mouse->reportEndpoint)
    RequestAttach(mouse->urb, mouse->reportEndpoint,
                  mouse->reportBuffer, MOUSE_REPORT_SIZE, 0);
  else
    assert(FALSE);
  RequestSetCompletionRoutine(mouse->urb, completion, 0, mouse);

  // Submit the request URB
  HostSubmitAsyncRequest(mouse->urb, mouse->device.host, NULL);
}

/*...................................................................*/
/* start_request: Initiate URB for mouse events                      */
/*                                                                   */
/*       Input: mouse is the mouse device                            */
/*                                                                   */
/*     Returns: zero (0) one success, FALSE otherwise                */
/*...................................................................*/
static int start_request(MouseDevice *mouse)
{
  assert(mouse != 0);
  assert(mouse->reportBuffer != 0);

  assert(mouse->urb == 0);
  mouse->urb = NewRequest();
  assert(mouse->urb != 0);
  bzero(mouse->urb, sizeof(Request));

  /* Attach URB to the device, preferring the interrupt endpoint. */
  if (mouse->interruptEndpoint)
    RequestAttach(mouse->urb, mouse->interruptEndpoint,
                  mouse->reportBuffer, MOUSE_REPORT_SIZE, 0);
  else if (mouse->reportEndpoint)
    RequestAttach(mouse->urb, mouse->reportEndpoint,
                  mouse->reportBuffer, MOUSE_REPORT_SIZE, 0);
  else
    return FALSE;
  RequestSetCompletionRoutine(mouse->urb, completion, 0, mouse);

  // Submit the request URB
  HostSubmitAsyncRequest(mouse->urb, mouse->device.host, NULL);
  return TRUE;
}

extern int UsbUp;

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* MouseAttach: Attach a device to a mouse device                    */
/*                                                                   */
/*       Input: device is the generic USB device                     */
/*                                                                   */
/*     Returns: void pointer to the new mouse device                 */
/*...................................................................*/
void *MouseAttach(Device *device)
{
  MouseDevice *mouse = &Mouse;
  assert(mouse != 0);

  MouseInitState = 0;
  MouseEnabled = 0;

  DeviceCopy(&mouse->device, device);
  mouse->device.configure = configure;

  mouse->reportEndpoint = 0;
  mouse->interruptEndpoint = 0;
  mouse->statusHandler = 0;
  mouse->urb = 0;
  assert(mouse->reportBuffer != 0);

  return mouse;
}

/*...................................................................*/
/* MouseRelease: Release a mouse device                              */
/*                                                                   */
/*       Input: device is the USB mouse device                       */
/*...................................................................*/
void MouseRelease(Device *device)
{
  MouseDevice *mouse = (void *)device;
  assert (mouse != 0);

  if (mouse->reportEndpoint != 0)
  {
    EndpointRelease(mouse->reportEndpoint);
    mouse->reportEndpoint = 0;
  }

  if (mouse->interruptEndpoint != 0)
  {
    EndpointRelease(mouse->interruptEndpoint);
    mouse->interruptEndpoint = 0;
  }

  DeviceRelease(&mouse->device);
}

/*...................................................................*/
/*     MouseUp: Activate a discovered and configured USB mouse       */
/*                                                                   */
/*       Input: command is unused                                    */
/*                                                                   */
/*     Returns: TASK_FINISHED as it is a shell command               */
/*...................................................................*/
int MouseUp(const char *command)
{
  if (!UsbUp)
    puts("USB not initialized");

  else if (!MouseEnabled)
  {
    MouseEnabled = TRUE;

    Mouse1->statusHandler = status_update;
    start_request(Mouse1);
  }
  else
    puts("USB not up or USB mouse already initialized");

  return TASK_FINISHED;
}

#endif /* ENABLE_USB_HID */
