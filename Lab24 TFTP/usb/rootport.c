/*...................................................................*/
/*                                                                   */
/*   Module:  rootport.c                                             */
/*   Version: 2020.0                                                 */
/*   Purpose: USB standard host virtual root port                    */
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
#include <assert.h>
#include <board.h>
#include <stdlib.h>
#include <stdio.h>
#include <usb/request.h>
#include <usb/device.h>

#if ENABLE_USB

/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
struct Host;

typedef struct HostRootPort
{
  struct Host *host;
  Device *device;
}
HostRootPort;

typedef struct ConfigurationHeader
{
  ConfigurationDescriptor configuration;
  InterfaceDescriptor     interface;
}
ConfigurationHeader;

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
Device RootDevice;
HostRootPort RootPort;

/*...................................................................*/
/* External function prototypes                                      */
/*...................................................................*/
extern void *HubAttach(Device *hub);
extern void HubRelease(Device *hub);
extern void *KeyboardAttach(Device *keyboard);
extern void KeyboardRelease(Device *keyboard);
extern void *MouseAttach(Device *mouse);
extern void MouseRelease(Device *mouse);
extern void *LanDeviceAttach(Device *lan);
extern void LanDeviceRelease(Device *lan);

/*...................................................................*/
/* Static Local Functions                                            */
/*...................................................................*/

/*...................................................................*/
/* initialize_complete: Root port initialization complete callback   */
/*                                                                   */
/*      Inputs: urb is USB Request Buffer (URB)                      */
/*              param is the root device                             */
/*              context is unused                                    */
/*...................................................................*/
static void initialize_complete(void *urb, void *param,
                                void *context)
{
  Device *child;
  Device *root = param;

  // Ceate specific device from default device
  child = DeviceCreate(root);
  if (child != 0)
  {
    // Release the parent device
    DeviceRelease(root);

    // Configure the detected device with the child device
    if (!(*child->configure)(child, NULL, NULL))
    {
      puts("  Cannot configure device");
      DeviceRelease(child);
    }
  }
  else
  {
    puts("  Device is not supported");
    DeviceRelease(root);
  }

  FreeRequest(urb);
}

/*...................................................................*/
/*  new_device: Find specific device from configuration descriptor   */
/*                                                                   */
/*       Input: parent is the generic USB device                     */
/*                                                                   */
/*     Returns: new USB device or NULL if not found/supported        */
/*...................................................................*/
static Device *new_device(Device *parent)
{
  Device *result = NULL;
  ConfigurationHeader *config;

  assert(parent != 0);
  config = (ConfigurationHeader *)parent->configDesc;

  if (0) ;
#if ENABLE_USB_HID
  else if ((config->interface.interfaceClass == 3) &&
           (config->interface.interfaceSubClass == 1) &&
           (config->interface.interfaceProtocol == 1))
  {
    result = KeyboardAttach(parent);
    puts("Keyboard detected");
  }
  else if ((config->interface.interfaceClass == 3) &&
           (config->interface.interfaceSubClass == 1) &&
           (config->interface.interfaceProtocol == 2))
  {
    result = MouseAttach(parent);
    puts("Mouse detected");
  }
#endif /* ENABLE_USB_HID */
#if ENABLE_ETHER && (RPI < 3)
  else if ((parent->deviceDesc->idVendor == 0x424) &&
           (parent->deviceDesc->idProduct == 0xEC00))
  {
    result = LanDeviceAttach(parent);
    puts("Ethernet (lan95xx) detected");
  }
#elif ENABLE_ETHER && (RPI >= 3)
  else if ((parent->deviceDesc->idVendor == 0x0424) &&
           (parent->deviceDesc->idProduct == 0x7800))
  {
    result = LanDeviceAttach(parent);
    puts("Ethernet (lan78xx) detected");
  }
#endif /* ENABLE_ETHER */
  //Check if this device is a hub
  else if (parent->deviceDesc->deviceClass == 9)
  {
    if (!((parent->deviceDesc->deviceClass == 0) ||
         (parent->deviceDesc->deviceClass == 0xFF)))
    {
      puts("Standard Hub detected");
      result = HubAttach(parent);
    }
  }
  else
  {
    puts("Device [class subclass protocol] detected but no driver");
    putchar('V');
    putchar(':');
    putbyte(parent->deviceDesc->idVendor);
    putbyte(parent->deviceDesc->idVendor >> 8);
    putchar(':');
    putbyte(parent->deviceDesc->idProduct);
    putbyte(parent->deviceDesc->idProduct >> 8);
    putchar(' ');
    putchar('D');
    putchar(':');
    putbyte(parent->deviceDesc->deviceClass);
    putchar(' ');
    putbyte(parent->deviceDesc->deviceSubClass);
    putchar(' ');
    putbyte(parent->deviceDesc->deviceProtocol);
    putchar('I');
    putbyte(config->interface.interfaceClass);
    putchar(' ');
    putbyte(config->interface.interfaceSubClass);
    putchar(' ');
    putbyte(config->interface.interfaceProtocol);
    putchar('\n');
    putchar('\r');
  }

  return result;
}

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* DeviceCreate: Create a specific device from a generic device      */
/*                                                                   */
/*       Input: parent is the generic USB device parent              */
/*                                                                   */
/*     Returns: void pointer to specific USB device or NULL          */
/*...................................................................*/
Device *DeviceCreate(Device *parent)
{
  assert (parent != 0);

  Device *result = new_device(parent);
  if (result == 0)
  {
    puts("Create device: device not known or init failed");
    return NULL;
  }

  assert(result != 0);
  return result;
}

/*...................................................................*/
/* RootPortAttach: Attach a USB host to a root port                  */
/*                                                                   */
/*       Input: host is the USB host owner to attach root port       */
/*                                                                   */
/*     Returns: void pointer to the new root port device             */
/*...................................................................*/
void *RootPortAttach(struct Host *host)
{
  HostRootPort *root = &RootPort;
  assert (root != 0);

  root->host = host;
  root->device = 0;

  assert (root->host != 0);
  return root;
}

/*...................................................................*/
/* RootPortRelease: Release a root port device                       */
/*                                                                   */
/*       Input: device is the USB root port device                   */
/*...................................................................*/
void RootPortRelease(Device *device)
{
  HostRootPort *root = (void *)device;
  assert (root != 0);

  if (root->device != 0)
  {
    DeviceRelease(root->device);
    root->device = 0;
  }

  root->host = 0;
}

/*...................................................................*/
/* RootPortInitialize: Initialize a root port device                 */
/*                                                                   */
/*       Input: device is the USB root port device                   */
/*                                                                   */
/*     Returns: TRUE on success, FALSE if error                      */
/*...................................................................*/
int RootPortInitialize(void *device)
{
  HostRootPort *root = device;
  Speed Speed;

  assert(root != 0);
  assert(root->host != 0);

  Speed = HostGetPortSpeed();
  if (Speed == SpeedUnknown)
  {
    puts("Cannot detect port speed");
    return FALSE;
  }

  // first create default device
  assert(root->device == 0);
  root->device = &RootDevice;
  assert(root->device != 0);
  DeviceAttach(root->device, root->host, Speed, 0, 1);

  //Save callback of end of initialization sequence of async operations
  ((Device *)root->device)->complete = initialize_complete;

  // Begin the root hub enumeration of devices
  DeviceInitialize(NULL, root->device, root);

  return TRUE;
}

#endif /* ENABLE_USB */
