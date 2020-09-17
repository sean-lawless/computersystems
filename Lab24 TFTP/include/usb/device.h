/*...................................................................*/
/*                                                                   */
/*   Module:  device.h                                               */
/*   Version: 2020.0                                                 */
/*   Purpose: Represent a video screen to draw/type onto             */
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
#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <usb/usb.h>
#include <usb/endpoint.h>
#include <string.h>
#include <system.h>

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define MAX_CONFIG_DESC_SIZE    512


/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
struct Host;

// Descriptor union
typedef union
{
  struct
  {
    u8 length;
    u8 descriptorType;
  }
  Header;

  ConfigurationDescriptor Configuration;
  InterfaceDescriptor Interface;
  EndpointDescriptor Ep;
} __attribute__((packed)) Descriptor;

typedef struct ConfigurationParser
{
  const Descriptor *buffer;
  unsigned bufLen;
  int valid;
  const void *endPosition;
  const Descriptor *currentPosition;
}
ConfigurationParser;

typedef struct Device
{
  struct Host *host;
  struct Endpoint *endpoint0;
  DeviceDescriptor *deviceDesc;
  ConfigurationDescriptor *configDesc;
  ConfigurationParser *configParser;

  // Declare and leave space for device configuration
  struct Endpoint Endpoint0;
  DeviceDescriptor DeviceDesc;
  u8 dev_descr_pad[16]; // Add padding to 16 byte align DeviceDesc
  ConfigurationDescriptor ConfigDesc;
  u8 config_descr_pad[MAX_CONFIG_DESC_SIZE + 16]; // Pad to 16 bytes
  ConfigurationParser ConfigParser;

  // Asynchronous configuration state machine variables
  void *comp_dev;
  void (*complete)(void *urb, void *param, void *context);
  int (*configure)(struct Device *device, void (complete)
              (void *urb, void *param, void *context), void *param);
  int state;

  // Device configuration details
  Speed speed;
  u8 address, hubAddress, hubPortNumber;
  char manufacturerString[20];
  char productString[20];
}
Device;

/*...................................................................*/
/* Function Prototypes                                               */
/*...................................................................*/
void DeviceAttach(Device *device, struct Host *host,
                     Speed Speed, u8 hubAddress, u8 hubPortNumber);
void DeviceRelease(Device *device);


Device *DeviceCreate(Device *parent);
void DeviceInit(void);
void DeviceCopy(Device *to, Device *from);
void DeviceInitialize(void *urb, void *param, void *context);
int DeviceConfigure (Device *device, void (complete)
              (void *urb, void *param, void *context), void *param);
const Descriptor *DeviceGetDescriptor(ConfigurationParser *parser,
                                      u8 type);

#endif
