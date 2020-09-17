/*...................................................................*/
/*                                                                   */
/*   Module:  hub.c                                                  */
/*   Version: 2020.0                                                 */
/*   Purpose: USB Hub Device driver                                  */
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
#include <string.h>
#include <usb/usb.h>
#include <usb/request.h>
#include <usb/device.h>

#if ENABLE_USB

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define MAX_HUBS               2
#define MAX_PORTS              8 // Must be multiple of 8

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
// Device Class
#define DEVICE_CLASS_HUB       9

// Class-specific Requests
#define RESET_TT               9

// Descriptor Type
#define DESCRIPTOR_HUB         0x29

// Feature Selectors
#define PORT_RESET             4
#define PORT_POWER             8

// Port Status
#define PORT_CONNECTION_MASK   (1 << 0)
#define PORT_ENABLE_MASK       (1 << 1)
#define PORT_OVER_CURRENT_MASK (1 << 3)
#define PORT_RESET_MASK        (1 << 4)
#define PORT_POWER_MASK        (1 << 8)
#define PORT_LOW_SPEED_MASK    (1 << 9)
#define PORT_HIGH_SPEED_MASK   (1 << 10)

// Hub configuration states
#define STATE_SET_INTERFACE    0
#define STATE_ENDPOINT_DESCR   1
#define STATE_ENUMERATE_PORTS  2

// Hub enumerations per port states
//   Each port requires the 4 state transitions below for enumeration.
#define STATE_ENUM_GET_STATUS  0
#define STATE_ENUM_PORT_RESET  1
#define STATE_ENUM_PORT_STATUS 2
#define STATE_ENUM_DEVICE      3

/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
typedef struct HubDescriptor
{
  u8 descLength;
  u8 descriptorType;
  u8 numPorts;
  u16 characteristics;
  u8 pwrOn2PwrGood;
  u8 controlCurrent;
  u8 deviceRemoveable[MAX_PORTS / 8]; // 1 bit per port
  u8 portPowerMask[MAX_PORTS / 8]; // 1 bit per port
} __attribute__((packed)) HubDescriptor;

typedef struct PortStatus
{
  u16 status;
  u16  change;
} __attribute__((packed)) PortStatus;

typedef struct Hub
{
  Device device; // Must be first element in this structure
  u32 ports, init_state;
  const InterfaceDescriptor *interfaceDesc;

  // References to structures populated on configuration success
  HubDescriptor *hubDesc;
  Device *devices[MAX_PORTS];
  PortStatus *status[MAX_PORTS];

  // Declare enough structures for hub and all ports
  HubDescriptor HubDesc;
  Device Devices[MAX_PORTS];
  PortStatus Status[MAX_PORTS];
}
Hub;

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
Hub HubDevice[MAX_HUBS];

/*...................................................................*/
/* Static Function Definitions                                       */
/*...................................................................*/

/*...................................................................*/
/* enumerate_ports_complete: State machine to enumerate hub ports    */
/*                                                                   */
/* Perform the state machine to enumerate all USB hub ports.         */
/*   1. Turn on power to all ports                                   */
/*   2. Read the port enable status of each port                     */
/*   3. Create generic device if port enabled                        */
/*   4. Read and configure if USB device is recognized               */
/*                                                                   */
/*      Inputs: urb is unused                                        */
/*              param is the hub device                              */
/*              context is unused                                    */
/*...................................................................*/
static void enumerate_ports_complete(void *urb, void *param,
                                     void *context)
{
  Hub *hub = param;
  Endpoint *endpoint0;
  int nPort;

  if (hub == NULL)
    return;
  if (hub->device.host == NULL)
    return;
  endpoint0 = hub->device.endpoint0;
  if (endpoint0 == NULL)
    return;

begin:
  /* The first loop through number of ports is to turn on power */
  if (hub->init_state < hub->ports)
  {
    //always free the last URB before sending a new one
    if (urb)
      FreeRequest(urb);
    urb = NULL;
    nPort = hub->init_state;
    if (HostEndpointControlMessage(hub->device.host, endpoint0,
           REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER, SET_FEATURE,
           PORT_POWER, nPort + 1, 0, 0,
           enumerate_ports_complete, hub) < 0)
    {
      printf("Cannot power port %u", nPort + 1);
      hub->init_state = 0;
      return;
    }
  }
  else
  {
    if ((hub->init_state - hub->ports) % 4 != STATE_ENUM_GET_STATUS)
    {
      //always free the last URB before sending a new one
      if (urb)
        FreeRequest(urb);
      urb = NULL;
    }

    if (hub->init_state == hub->ports)
    {
      /* After powering port, delay by PwrOn2PwrGood in milliseconds */
//      printf("Power on 2 power good delay = %d\n",
//             hub->hubDesc->pwrOn2PwrGood);

      // Ignore the power delay, use half second USB maximum delay
      usleep(MICROS_PER_SECOND/ 2);
    }

    nPort = (hub->init_state - hub->ports) / 4;

    if (nPort < hub->ports)
    {
//      printf("In: port = %d, state = %d\n", nPort, hub->init_state);

      if ((hub->init_state - hub->ports) % 4 == STATE_ENUM_GET_STATUS)
      {
//        printf("In: if ((hub->init_state - hub->ports) rem 4 == 0), port = %d\n", nPort);
        // now detect devices, reset and initialize them
        assert (hub->status[nPort] == 0);
        hub->status[nPort] = &(hub->Status[nPort]);
        assert (hub->status[nPort] != 0);

        if (HostEndpointControlMessage(hub->device.host, endpoint0,
          REQUEST_IN | REQUEST_CLASS | REQUEST_TO_OTHER,
          GET_STATUS, 0, nPort + 1, hub->status[nPort], 4,
          enumerate_ports_complete, hub) < 0)
        {
          puts("Cannot get status of port");// %u", nPort);
          hub->init_state = 0;
          return;
        }
      }
      else if ((hub->init_state - hub->ports) % 4 ==
                                                  STATE_ENUM_PORT_RESET)
      {
//        printf("In: if ((hub->init_state - hub->ports) rem 4 == 1), port = %d\n", nPort);
        assert (hub->status[nPort]->status & PORT_POWER_MASK);

        if (HostEndpointControlMessage(hub->device.host, endpoint0,
          REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
          SET_FEATURE, PORT_RESET, nPort + 1, 0, 0,
          enumerate_ports_complete, hub) < 0)
        {
          puts("Cannot reset port");

          hub->init_state = 0;
          return;
        }
      }
      else if ((hub->init_state - hub->ports) % 4 ==
                                                 STATE_ENUM_PORT_STATUS)
      {
//        printf("In: if ((hub->init_state - hub->ports) rem 4 == 2), port = %d\n", nPort);

        usleep(MICROS_PER_MILLISECOND * 200);

        if (HostEndpointControlMessage(hub->device.host, endpoint0,
          REQUEST_IN | REQUEST_CLASS | REQUEST_TO_OTHER,
          GET_STATUS, 0, nPort + 1, hub->status[nPort], 4,
          enumerate_ports_complete, hub) < 0)
        {
          puts("ERROR - HostEndpointControlMessage failed for port enable");
          hub->init_state = 0;
          return;
        }
      }
      else if (((hub->init_state - hub->ports)) % 4 ==STATE_ENUM_DEVICE)
      {
        Speed speed = SpeedUnknown;

//        puts("In: else if ((HubInitState - hub->ports)) % 4 == 3");
        usleep(MICROS_PER_MILLISECOND * 200);
        if (!(hub->status[nPort]->status & PORT_CONNECTION_MASK))
        {
          printf("Port %d is not connected 0x%x\n", nPort,
                 hub->status[nPort]->status);
          hub->init_state++;
          goto begin;
        }

        if (!(hub->status[nPort]->status & PORT_ENABLE_MASK))
        {
          printf("Port %d is not enabled 0x%x\n", nPort,
                 hub->status[nPort]->status);
          hub->init_state++;
          goto begin;
        }

        if (hub->status[nPort]->status & PORT_LOW_SPEED_MASK)
        {
          speed = SpeedLow;
        }
        else if (hub->status[nPort]->status & PORT_HIGH_SPEED_MASK)
        {
          speed = SpeedHigh;
        }
        else
        {
          speed = SpeedFull;
        }

        // Allocate and attach the device
        assert (hub->devices[nPort] == 0);
        hub->devices[nPort] = &(hub->Devices[nPort]);
        assert(hub->devices[nPort] != 0);
        printf("Attach port %d, address %d\n", nPort,
               hub->device.address);
        DeviceAttach(hub->devices[nPort], hub->device.host, speed,
                     hub->device.address, nPort + 1);

        // Register callback and initialize the device
        hub->devices[nPort]->complete = enumerate_ports_complete;
        hub->devices[nPort]->comp_dev = hub;
        DeviceInitialize(NULL, hub->devices[nPort], hub);
      }
    }

    // Final state loop through ports will create specific devices
    // from all attached devices.
    else if (hub->init_state >= ((hub->ports + 1) * 4) - 1)
    {
      // now configure devices
      nPort = (hub->init_state - ((hub->ports + 1) * 4));
      if (nPort < 0)
        nPort = 0;

      // Configure ports in reverse order (high port to low)
      // This forces the secondary hub in port 0 to be configured last
      // which is required.
      nPort = hub->ports - 1 - nPort;

//      printf("In: port = %d, state = %d\n", nPort, hub->init_state);
//      puts("In: else if (hub->init_state > (hub->ports + 1) * 4)");
      if ((nPort >= 0) && (nPort < hub->ports))
      {
        Device *child;

        // If port is empty, try next port
        if (hub->devices[nPort] == 0)
        {
          ++hub->init_state;
          goto begin;
        }

        // now create specific device from default device
        child = DeviceCreate(hub->devices[nPort]);
        if (child != 0)
        {
          // If device already assigned, release it
          if (hub->devices[nPort])
            DeviceRelease(hub->devices[nPort]);

          hub->devices[nPort] = child;
          hub->devices[nPort]->complete = enumerate_ports_complete;
          hub->devices[nPort]->comp_dev = hub;

          puts("Configure new device");
          if (!(*hub->devices[nPort]->configure)(hub->devices[nPort], NULL, NULL))
          {
            puts("Port: Cannot configure device");
            ++hub->init_state;
            goto begin;
          }
        }
        else
        {
          printf("Port %u: Device is not supported\n", nPort);

          DeviceRelease(hub->devices[nPort]);
          hub->devices[nPort] = 0;

          ++hub->init_state;
          goto begin;
        }
      }
      else
      {
        hub->init_state = 0;
        puts("Hub Enumeration Complete");
        return;
      }
    }
    else
    {
      puts("Hub Enumeration Failed");
      hub->init_state = 0; // success, restart state machine
      return;
    }

  }

  /* Update the hub initialization state once for fallthrough */
  ++hub->init_state;
}

/*...................................................................*/
/* enumerate_ports: start looking for devices on hub physical ports  */
/*                                                                   */
/*      Inputs: hub is the hub device                                */
/*                                                                   */
/*     Returns: TRUE if success, FALSE otherwise                     */
/*...................................................................*/
static int enumerate_ports(Hub *hub)
{
  assert (hub != 0);

  Endpoint *endpoint0 = hub->device.endpoint0;

  assert(hub->device.host != 0);
  assert(endpoint0 != 0);
  assert(hub->ports > 0);

  /* Restart the hub state machine */
  hub->init_state = 0;

  /* Start enumerating the USB ports by turning on power to Port0. */
  if (HostEndpointControlMessage(hub->device.host, endpoint0,
      REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
      SET_FEATURE, PORT_POWER, 1, 0, 0,
      enumerate_ports_complete, hub) < 0)
  {
      puts("Cannot send control message to power port 0");
      return FALSE;
  }
  return TRUE;
}

/*...................................................................*/
/* configure_complete: State machine to configure a hub              */
/*                                                                   */
/*      Inputs: urb is USB Request Buffer (URB)                      */
/*              param is the hub device                              */
/*              context is unused                                    */
/*...................................................................*/
static void configure_complete(void *urb, void *param, void *context)
{
  Hub *hub = param;
  const InterfaceDescriptor *interfaceDesc = hub->interfaceDesc;

  //always free the last URB before sending a new one
  if (urb)
    FreeRequest(urb);

  if (hub->hubDesc == 0)
  {
    // 16 bype align the hub descriptor
    hub->hubDesc = (void *)(((uintptr_t)&hub->HubDesc) +
                            (16 - (((uintptr_t)&hub->HubDesc) & 15)));
  }
  assert (hub->hubDesc != 0);
  assert (hub->device.host != 0);

//  printf("configure_complete state %d\n", hub->init_state);

  if ((hub->init_state == STATE_SET_INTERFACE) &&
      (interfaceDesc->alternateSetting != 0))
  {
      if (HostEndpointControlMessage(hub->device.host,
            hub->device.endpoint0, REQUEST_OUT | REQUEST_TO_INTERFACE,
            SET_INTERFACE, interfaceDesc->alternateSetting,
            interfaceDesc->interfaceNumber, 0, 0, configure_complete,
            hub) < 0)
      {
        puts("Cannot set interface");
        return;
      }
      hub->init_state = STATE_ENDPOINT_DESCR;
  }
  else if (hub->init_state <= STATE_ENDPOINT_DESCR)
  {
    if (HostGetEndpointDescriptor(hub->device.host,
          hub->device.endpoint0, DESCRIPTOR_HUB,
          INDEX_DEFAULT, hub->hubDesc,
          sizeof(HubDescriptor), REQUEST_IN | REQUEST_CLASS,
          configure_complete, hub) <= 0)
    {
      puts("Cannot get hub descriptor");
      hub->hubDesc = 0;
      hub->init_state = STATE_SET_INTERFACE;
      return;
    }
    hub->init_state = STATE_ENUMERATE_PORTS;
  }
  else if (hub->init_state == STATE_ENUMERATE_PORTS)
  {

    hub->ports = hub->hubDesc->numPorts;
    if (hub->ports > MAX_PORTS)
    {
      puts("Too many ports");
      hub->hubDesc = 0;
      hub->init_state = STATE_SET_INTERFACE;
      return;
    }

    printf("Begin USB hub enumeration of %d ports...\n",
           hub->ports);
    hub->init_state = 0;
    if (!enumerate_ports(hub))
    {
      puts("Port enumeration failed");
      hub->init_state = STATE_SET_INTERFACE;
      return;
    }
  }
}

/*...................................................................*/
/*   configure: Start hub device configuration                       */
/*                                                                   */
/*      Inputs: device is hub device                                 */
/*              complete is callback function invoked upon completion*/
/*              param is unused                                      */
/*...................................................................*/
static int configure(Device *device, void (complete)
                  (void *urb, void *param, void *context), void *param)
{
  Hub *hub = (Hub *)device;
  const DeviceDescriptor *deviceDesc;
  const ConfigurationDescriptor *configDesc;
  const InterfaceDescriptor *interfaceDesc;
  const EndpointDescriptor *endpointDesc;

  assert(hub != 0);
  deviceDesc = hub->device.deviceDesc;
  assert(deviceDesc != 0);

  if ((deviceDesc->deviceClass != DEVICE_CLASS_HUB) ||
      (deviceDesc->deviceSubClass    != 0) ||
      (deviceDesc->deviceProtocol    != 2) || // multiple TTs
      (deviceDesc->numConfigurations != 1))
  {
    puts("Unsupported hub");
    return FALSE;
  }

  configDesc = (ConfigurationDescriptor *)DeviceGetDescriptor(
                    hub->device.configParser, DESCRIPTOR_CONFIGURATION);
  if ((configDesc == 0) || (configDesc->numInterfaces != 1))
  {
    puts("USB hub configuration descriptor not valid");

    return FALSE;
  }

  while ((interfaceDesc = (InterfaceDescriptor *)
           DeviceGetDescriptor(hub->device.configParser,
                                            DESCRIPTOR_INTERFACE)) != 0)
  {
    if ((interfaceDesc->interfaceClass != DEVICE_CLASS_HUB) ||
        (interfaceDesc->interfaceSubClass != 0) ||
        (interfaceDesc->interfaceProtocol != 2))
      continue;

    if (interfaceDesc->numEndpoints != 1)
    {
      puts("USB hub interface not valid (num endpoints != 1)");
      return FALSE;
    }

    endpointDesc = (EndpointDescriptor *)DeviceGetDescriptor(
                         hub->device.configParser, DESCRIPTOR_ENDPOINT);
    if ((endpointDesc == 0) ||
        ((endpointDesc->endpointAddress & 0x80) != 0x80) || // input EP
        ((endpointDesc->attributes & 0x3F) != 0x03))  // interrupt EP
    {
      puts("USB hub endpoints not valid");
      return FALSE;
    }

    break;
  }

  if (interfaceDesc == 0)
  {
    puts("USB hub no interface descriptor");
    return FALSE;
  }

  hub->interfaceDesc = interfaceDesc;
  if (!DeviceConfigure(&hub->device, configure_complete, hub))
  {
    puts("Cannot set configuration");
    return FALSE;
  }

  return TRUE;
}

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* HubAttach: Attach a hub to a generic USB device                   */
/*                                                                   */
/*       Input: device is generic USB device                         */
/*                                                                   */
/*     Returns: void pointer to resulting hub device                 */
/*...................................................................*/
void *HubAttach(Device *device)
{
  static int hubIndex = 0;
  u32 nPort = 0;
  Hub *hub;

  if (hubIndex > MAX_HUBS)
  {
    puts("Maximum hubs exceeded.");
    return NULL;
  }

  hub = &HubDevice[hubIndex++];

  assert(hub != 0);

  assert (device != 0);
  putbyte(device->deviceDesc->deviceClass);
  putchar('-');
  putbyte(device->deviceDesc->deviceSubClass);


  DeviceCopy(&hub->device, device);
  hub->device.configure = configure;

  hub->hubDesc = 0;
  bzero(&hub->HubDesc, sizeof(HubDescriptor) + 16);

  hub->ports = 0;
  hub->init_state = STATE_SET_INTERFACE;

  for (nPort = 0; nPort < MAX_PORTS; nPort++)
  {
    hub->devices[nPort] = 0;
    hub->status[nPort] = 0;
    bzero(&hub->Devices[nPort], sizeof(Device));
    bzero(&hub->Status[nPort], sizeof(PortStatus));
  }
  return hub;
}

/*...................................................................*/
/* HubRelease: Release a previously attached hub                     */
/*                                                                   */
/*       Input: device the hub device                                */
/*...................................................................*/
void HubRelease(void *device)
{
  u32 port = 0;
  Hub *hub = device;

  assert(hub != 0);

  // Zero out all the devices and status
  for (port = 0; port < hub->ports; port++)
  {
    if (hub->status[port] != 0)
      hub->status[port] = 0;

    if (hub->devices[port] != 0)
    {
      DeviceRelease(hub->devices[port]);
      hub->devices[port] = 0;
    }
  }

  hub->ports = 0;
  if (hub->hubDesc != 0)
    hub->hubDesc = 0;

  DeviceRelease(&hub->device);
}

#endif /* ENABLE_USB */
