/*...................................................................*/
/*                                                                   */
/*   Module:  usb.h                                                  */
/*   Version: 2020.0                                                 */
/*   Purpose: USB common definitions                                 */
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
#ifndef _USB_H
#define _USB_H

#include <board.h>

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
// USB 2.0 specification

// Device Addresses
#define DEFAULT_ADDRESS          0
#define FIRST_ADDRESS            1
#define MAX_ADDRESS              127

// Request Types
#define REQUEST_OUT              0
#define REQUEST_IN               0x80

#define REQUEST_CLASS            0x20
#define REQUEST_VENDOR           0x40

#define REQUEST_TO_INTERFACE     1
#define REQUEST_TO_OTHER         3

// Standard Request Codes
#define GET_STATUS               0
#define CLEAR_FEATURE            1
#define SET_FEATURE              3
#define SET_ADDRESS              5
#define GET_DESCRIPTOR           6
#define SET_CONFIGURATION        9
#define SET_INTERFACE            11

// Descriptor Types
#define DESCRIPTOR_DEVICE        1
#define   INDEX_DEFAULT          0
#define DESCRIPTOR_CONFIGURATION 2
#define DESCRIPTOR_STRING        3
#define DESCRIPTOR_INTERFACE     4
#define DESCRIPTOR_ENDPOINT      5

#define MAX_PACKET_SIZE          8

/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
// PID
typedef enum
{
  PIDSetup,
  PIDData0,
  PIDData1,
}
PID;

// Speed
typedef enum
{
  SpeedLow,
  SpeedFull,
  SpeedHigh,
  SpeedUnknown
}
Speed;

// Setup data
typedef struct
{
  u8 requestType;
  u8 request;
  u16 value;
  u16 index;
  u16 length;
} __attribute__((packed)) SetupData;

// Device Descriptor
typedef struct
{
  u8 length;
  u8 descriptorType;
  u16 usb;
  u8 deviceClass;
  u8 deviceSubClass;
  u8 deviceProtocol;
  u8 maxPacketSize;
  u16 idVendor;
  u16 idProduct;
  u16 device;
  u8 manufacturer;
  u8 product;
  u8 serialNumber;
  u8 numConfigurations;
} __attribute__((packed)) DeviceDescriptor;

// Configuration Descriptor
typedef struct
{
  u8 length;
  u8 descriptorType;
  u16 totalLength;
  u8 numInterfaces;
  u8 configurationValue;
  u8 configuration;
  u8 attributes;
  u8 maxPower;
} __attribute__((packed)) ConfigurationDescriptor;

// Interface Descriptor
typedef struct
{
  u8 length;
  u8 descriptorType;
  u8 interfaceNumber;
  u8 alternateSetting;
  u8 numEndpoints;
  u8 interfaceClass;
  u8 interfaceSubClass;
  u8 interfaceProtocol;
  u8 interface;
} __attribute__((packed)) InterfaceDescriptor;

// Endpoint Descriptor
typedef struct
{
  u8 length;
  u8 descriptorType;
  u8 endpointAddress;
  u8 attributes;
  u8 maxPacketSize;
  u8 maxPacketSize2;
  u8 interval;
} __attribute__((packed)) EndpointDescriptor;

#endif /* _USB_H */
