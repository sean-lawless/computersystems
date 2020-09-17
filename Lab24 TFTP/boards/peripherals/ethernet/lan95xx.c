/*...................................................................*/
/*                                                                   */
/*   Module:  lan95xx.c                                              */
/*   Version: 2018.0                                                 */
/*   Purpose: Microchip LAN95XX USB driver                           */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2018, Sean Lawless                    */
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
/*...................................................................*/
#include <system.h>
#include <board.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <usb/request.h>
#include <usb/device.h>

#if ENABLE_ETHER && (RPI < 3)

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define STATIC_MAC_ADDRESS FALSE
#define FRAME_BUFFER_SIZE  1600
#define MAC_ADDRESS_SIZE   6

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
// SMSC/LAN 9xxx Register Map
// Register map derived from if_smscreg.h in FreeBSD, which itself is
// from information Copyright (c) 2007-2008 SMSC (now MicroChip?)
#define ID_REV             0x00
#define INTR_STATUS        0x08
#define RX_CFG             0x0C
#define TX_CFG             0x10
#define TX_CFG_ON            (1 << 2)
#define HW_CFG             0x14
#define PM_CTRL            0x20
#define LED_GPIO_CFG       0x24
#define   LED_GPIO_CFG_FDX_LED (1 << 16)
#define   LED_GPIO_CFG_LNK_LED (1 << 20)
#define   LED_GPIO_CFG_SPD_LED (1 << 24)
#define GPIO_CFG           0x28
#define AFC_CFG            0x2C
#define EEPROM_CMD         0x30
#define EEPROM_DATA        0x34
#define BURST_CAP          0x38
#define GPIO_WAKE          0x64
#define INT_EP_CTL         0x68
#define BULK_IN_DLY        0x6C
#define MAC_CSR            0x100
#define   MAC_CSR_RXEN       (1 << 2)  /* Rx enable */
#define   MAC_CSR_TXEN       (1 << 3)  /* Tx enable */
#define   MAC_CSR_BCAST      (1 << 11) /* broadcast */
#define   MAC_CSR_PRMS       (1 << 18) /* promiscuous */
#define   MAC_CSR_MCPAS      (1 << 19) /* multicast */
#define   MAC_CSR_RCVOWN     (1 << 23) /* half duplex */
#define MAC_ADDRH          0x104
#define MAC_ADDRL          0x108
#define HASHH              0x10C
#define HASHL              0x110
#define MII_ADDR           0x114
#define MII_DATA           0x118
#define FLOW               0x11C
#define VLAN1              0x120
#define VLAN2              0x124
#define WUFF               0x128
#define WUCSR              0x12C
#define COE_CRTL           0x130

// USB Request IDs (Vendor Requests)
#define UR_WRITE_REG       0xA0
#define UR_READ_REG        0xA1
#define UR_GET_STATUS      0xA2

// TX commands (CTRL_0 is first of two 32-bit words in buffer)
#define TX_CTRL_0_LAST_SEG   (1 << 12)
#define TX_CTRL_0_FIRST_SEG  (1 << 13)
// CTRL_1 not used

// RX status (first 32-bit word in buffer)
#define RX_STAT_CRC_ERROR    (1 << 1)  // CRC Error
#define RX_STAT_DRIBBLING    (1 << 2)  // Dribbling
#define RX_STAT_MII_ERROR    (1 << 3)  // Mii Error
#define RX_STAT_WATCHDOG     (1 << 4)  // Watchdog
#define RX_STAT_FRM_TYPE     (1 << 5)  // Frame Type
#define RX_STAT_COLLISION    (1 << 6)  // Collision
#define RX_STAT_FRM_TO_LONG  (1 << 7)  // Frame too long
#define RX_STAT_MULTICAST    (1 << 10) // Multicast
#define RX_STAT_RUNT         (1 << 11) // Runt
#define RX_STAT_LENGTH_ERROR (1 << 12) // Length Error
#define RX_STAT_BROADCAST    (1 << 13) // Broadcast
#define RX_STAT_ERROR        (1 << 15) // Error

#define RX_STAT_FRM_LENGTH(len) (((len) >> 16) & 0x3FFF)
#define RX_STAT_FILTER_FAIL  (1 << 30) // Filter Fail

#define RX_STAT_ERROR_MASK (RX_STAT_FILTER_FAIL | RX_STAT_ERROR | \
          RX_STAT_LENGTH_ERROR | RX_STAT_RUNT | RX_STAT_FRM_TO_LONG | \
          RX_STAT_COLLISION | RX_STAT_WATCHDOG | RX_STAT_MII_ERROR | \
          RX_STAT_DRIBBLING | RX_STAT_CRC_ERROR)

// Lan95xx configuration state machine
#define STATE_HIGH_ADDRESS   0
#define STATE_LOW_ADDRESS    1
#define STATE_GPIO_CFG       2
#define STATE_MAC_ENABLE     3
#define STATE_TX_CFG         4
#define STATE_FINISHED       5

typedef struct Lan95xxDevice
{
  Device device;

  Endpoint *endpointBulkIn;
  Endpoint *endpointBulkOut;
  Endpoint EndpointBulkIn;
  Endpoint EndpointBulkOut;

  int configurationState;
  u8 address[MAC_ADDRESS_SIZE];
  u8 *txBuffer;
  u8 *rxBuffer;
  Request *rxURB;
  u8 TxBuffer[FRAME_BUFFER_SIZE];
  u8 RxBuffer[FRAME_BUFFER_SIZE];
}
Lan95xxDevice;

Lan95xxDevice EtherDevice;
Lan95xxDevice *Eth0 = NULL;

#if ENABLE_NETWORK
int NetStart(char *command);
int NetIn(u8 *frame, int frameLength);
#endif

/*...................................................................*/
/* Static local functions                                            */
/*...................................................................*/

/*...................................................................*/
/*  write_reg: Write data to a LAN register as USB request           */
/*                                                                   */
/*      Input: lan is the USB device                                 */
/*             index is the register number                          */
/*             value is the register value to assign                 */
/*     Output: complete() is the function invoked upon completion    */
/*...................................................................*/
static void write_reg(Lan95xxDevice *lan, u32 index,
     u32 value, void(*complete)(void *urb, void *param, void *context))
{
  static u32 Value;

  assert(lan != 0);

  Endpoint *endpoint = lan->device.endpoint0;
  Value = value; //Copy from stack to persistent (static) memory

  HostEndpointControlMessage(lan->device.host, endpoint,
            REQUEST_OUT | REQUEST_VENDOR, UR_WRITE_REG, 0, index,
            &Value, sizeof(u32), complete, complete ? lan : NULL);
}

/*...................................................................*/
/* configure_complete: Callback for configure state machine          */
/*                                                                   */
/*      Input: urb is not used                                       */
/*             param is a pointer to the LAN USB device              */
/*             context is not used                                   */
/*...................................................................*/
static void configure_complete(void *urb, void *param, void *context)
{
  Lan95xxDevice *lan = (Lan95xxDevice *)param;

  //always free the last URB before sending a new one
  if (urb)
    FreeRequest(urb);
  urb = NULL;

  if (lan->configurationState == STATE_HIGH_ADDRESS)
  {
    u16 macAddressHigh;
    void *addr = &lan->address[4];

    macAddressHigh = *(u16 *)addr;
    write_reg(lan, MAC_ADDRH, macAddressHigh, configure_complete);
  }
  else if (lan->configurationState == STATE_LOW_ADDRESS)
  {
    u32 macAddressLow;
    void *addr = &lan->address[0];

    macAddressLow = *(u32 *)addr;
    write_reg(lan, MAC_ADDRL, macAddressLow, configure_complete);
  }
  else if (lan->configurationState == STATE_GPIO_CFG)

    write_reg(lan, LED_GPIO_CFG, LED_GPIO_CFG_SPD_LED |
              LED_GPIO_CFG_LNK_LED | LED_GPIO_CFG_FDX_LED,
              configure_complete);
  else if (lan->configurationState == STATE_MAC_ENABLE)
    write_reg(lan, MAC_CSR, MAC_CSR_TXEN | MAC_CSR_RXEN,
              configure_complete);
  else if (lan->configurationState == STATE_TX_CFG)
    write_reg(lan, TX_CFG, TX_CFG_ON, configure_complete);
  else if (lan->configurationState == STATE_FINISHED)
  {
    Eth0 = lan;
    assert(Eth0 == lan);

    /* Configuration complete, call complete function if present. */
    lan->configurationState = 0;
    if (lan->device.complete)
    {
      lan->device.complete(NULL, lan->device.comp_dev, NULL);
      lan->device.complete = NULL;
    }
    puts("Lan95xx configuration complete");

#if ENABLE_AUTO_START && ENABLE_NETWORK
    /* Bring up the TCP/IP network */
    NetStart(NULL);
#endif

    return;
  }

  // Advance to the next configuration state
  ++lan->configurationState;
}

/*...................................................................*/
/*  configure: Configuration entry point for Lan78xx driver          */
/*                                                                   */
/*      Input: device is the USB device                              */
/*     Output: complete() is function called after completion        */
/*             param is a pointer that is passed to complete call    */
/*                                                                   */
/*     Return: TRUE on success, FALSE if failure                     */
/*...................................................................*/
static int configure(Device *device, void (complete)
                  (void *urb, void *param, void *context), void *param)
{
  Lan95xxDevice *lan = (Lan95xxDevice *)device;
  const ConfigurationDescriptor *configDesc;
  const InterfaceDescriptor *interfaceDesc;
  const EndpointDescriptor *endpointDesc;

  assert (lan != 0);

#if STATIC_MAC_ADDRESS
  lan->address.address[0] = 0xB8;
  lan->address.address[1] = 0x27;
  lan->address.address[2] = 0xEB;
  lan->address.address[3] = 0xFE;
  lan->address.address[4] = 0x3A;
  lan->address.address[5] = 0x91;
  printf("MAC address is B8:27:EB:FE:3A:91\n");
#else
  if (GetMACAddress(lan->address))
  {
    printf("Get MAC failed. Rebuild with STATIC_MAC_ADDRESS?\n");
    return FALSE;
  }
#endif

  configDesc = (ConfigurationDescriptor *)DeviceGetDescriptor(
                    lan->device.configParser, DESCRIPTOR_CONFIGURATION);
  if ((configDesc == 0) || (configDesc->numInterfaces != 1))
  {
    puts("USB lan configuration not valid");
    return FALSE;
  }

  interfaceDesc = (InterfaceDescriptor *)DeviceGetDescriptor(
                        lan->device.configParser, DESCRIPTOR_INTERFACE);
  if ((interfaceDesc == 0) || (interfaceDesc->numEndpoints != 3) ||
      (interfaceDesc->alternateSetting != 0x00) ||
      (interfaceDesc->interfaceNumber != 0x00))
  {
    puts("USB lan interface not valid");
    return FALSE;
  }

  // Loop through all endpoints, assigning bulk input and output
  while ((endpointDesc = (EndpointDescriptor *)
          DeviceGetDescriptor(lan->device.configParser,
                              DESCRIPTOR_ENDPOINT)) != 0)
  {
    // Check if endpoint is bulk
    if ((endpointDesc->attributes & 0x3F) == 0x02)
    {
      // Check if bulk endpoint is input
      if ((endpointDesc->endpointAddress & 0x80) == 0x80)
      {
        if (lan->endpointBulkIn != 0)
        {
          puts("USB lan config error with bulk in endpoint");
          return FALSE;
        }

        lan->endpointBulkIn = (Endpoint *)&lan->EndpointBulkIn;
        EndpointFromDescr(lan->endpointBulkIn, &lan->device,
                          endpointDesc);
      }

      // Otherwise bulk endpoint is output
      else
      {
        if (lan->endpointBulkOut != 0)
        {
          puts("USB lan config error with bulk out endpoint");
          return FALSE;
        }

        lan->endpointBulkOut = (Endpoint *)&lan->EndpointBulkOut;
        EndpointFromDescr(lan->endpointBulkOut, &lan->device,
                          endpointDesc);
      }
    }
  }

  if ((lan->endpointBulkIn  == 0) || (lan->endpointBulkOut == 0))
  {
    puts("USB lan endpoint not found");
    return FALSE;
  }

  if (!DeviceConfigure(&lan->device, configure_complete, lan))
  {
    puts("Cannot configure lan95xx");
    return FALSE;
  }

  // Return configuration success
  return TRUE;
}

static void receive_complete(void *request, void *param,
                                  void *context)
{
  Request *urb = request;
  u32 resultLength, rxStatus, frameLength;
  Lan95xxDevice *lan = (Lan95xxDevice *)param;
  u8 *buffer;

  assert (lan != 0);
  assert (urb != 0);

  resultLength = urb->resultLen;
  if (resultLength < 4)
    goto finished;

  buffer = lan->rxBuffer;
  rxStatus = *(u32 *)buffer;
  if (rxStatus & RX_STAT_ERROR_MASK)
  {
    printf("RX error (status 0x%X)", rxStatus);
    goto finished;
  }

  // Read the frame buffer from the URB Rx status
  frameLength = RX_STAT_FRM_LENGTH(rxStatus);
  assert(frameLength > 4);

#if ENABLE_NETWORK
  // Remove RX status 4 bytes from the frame
  frameLength -= 4;
  buffer += 4;

  // Inform the network stack of the new frame
  NetIn(buffer, frameLength);
#endif /* ENABLE_NETWORK */

finished:

  //Reuse urb and start another async request
  RequestRelease(urb);
  RequestAttach(urb, lan->endpointBulkIn, lan->rxBuffer,
                FRAME_BUFFER_SIZE, 0);

  //Register the callback and submit the request asynchronously
  RequestSetCompletionRoutine(urb, receive_complete, lan, NULL);
  HostSubmitAsyncRequest(urb, lan->device.host, NULL);
}

/*...................................................................*/
/* Global functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* LanReceiveAsync: Send an Ethernet frame over network              */
/*                                                                   */
/*      Input: buffer is the buffer of the network frame             */
/*             length is the length of the buffer                    */
/*                                                                   */
/*    Returns: Zero on success, failure otherwise                    */
/*...................................................................*/
int LanReceiveAsync(void)
{
  Lan95xxDevice *dev = Eth0;
  if (dev == NULL)
    return -1;

  assert (dev->rxURB == 0);
  dev->rxURB = NewRequest();
  assert (dev->rxURB != 0);
  bzero(dev->rxURB, sizeof(Request));

  dev->rxBuffer = dev->RxBuffer;

  u32 buf = (u32)dev->rxBuffer;
  putbyte(buf); putbyte(buf >> 8);
  putbyte(buf >> 16); putbyte(buf >> 24);

  // Create and submit the endpoint request
  RequestAttach(dev->rxURB, dev->endpointBulkIn, dev->rxBuffer,
                FRAME_BUFFER_SIZE, 0);
  RequestSetCompletionRoutine(dev->rxURB, receive_complete, dev, NULL);
  HostSubmitAsyncRequest(dev->rxURB, dev->device.host, NULL);
  return 0;
}

/*...................................................................*/
/* LanDeviceSendFrame: Send an Ethernet frame over network           */
/*                                                                   */
/*      Input: buffer is the buffer of the network frame             */
/*             length is the length of the buffer                    */
/*                                                                   */
/*    Returns: Zero on success, failure otherwise                    */
/*...................................................................*/
int LanDeviceSendFrame(const void *buffer, u32 length)
{
  Lan95xxDevice *lan = Eth0;
  assert (lan != 0);

  if (length >= FRAME_BUFFER_SIZE-8)
  {
    return FALSE;
  }

  assert(lan->txBuffer != 0);
  assert(buffer != 0);
  memcpy(lan->txBuffer+8, buffer, length);

  *(u32 *)(&lan->txBuffer[0]) = TX_CTRL_0_FIRST_SEG |
                                TX_CTRL_0_LAST_SEG | length;
  *(u32 *)(&lan->txBuffer[4]) = length;

  assert (lan->endpointBulkOut != 0);
  return HostEndpointTransfer(lan->device.host, lan->endpointBulkOut,
                              lan->txBuffer, length + 8, NULL) >= 0;
}

/*...................................................................*/
/*  LanGetMAC: Retrieve pointer to unmodifiable MAC address          */
/*                                                                   */
/*    Returns: Constant (unmodifiable) pointer to the MAC Id         */
/*...................................................................*/
const u8 *LanGetMAC(void)
{
  if (Eth0)
    return (const u8 *)&Eth0->address;
  else
    return NULL;
}

/*...................................................................*/
/* LanDeviceAttach: Attach an emumerated device with the LAN device  */
/*                                                                   */
/*      Input: device is the USB device that hub enumerated          */
/*                                                                   */
/*    Returns: Pointer to the Lan78xxDevice                          */
/*...................................................................*/
void *LanDeviceAttach(Device *device)
{
  Lan95xxDevice *lan = &EtherDevice;
  assert(lan != 0);

  lan->configurationState = 0;
  DeviceCopy (&lan->device, device);
  lan->device.configure = configure;

  lan->endpointBulkIn = 0;
  lan->endpointBulkOut = 0;
  lan->txBuffer = 0;
  lan->configurationState = 0;

  lan->txBuffer = lan->TxBuffer;
  assert (lan->txBuffer != 0);
  Eth0 = NULL; // Do not assign until configured

  return lan;
}

/*...................................................................*/
/* LanDeviceRelease: Release the LAN device                          */
/*                                                                   */
/*      Input: device is the LAN USB device                          */
/*...................................................................*/
void LanDeviceRelease(Device *device)
{
  Lan95xxDevice *lan = (void *)device;
  assert (lan != 0);

  if (lan->txBuffer != 0)
    lan->txBuffer = 0;

  if (lan->endpointBulkOut != 0)
  {
    EndpointRelease(lan->endpointBulkOut);
    lan->endpointBulkOut = 0;
  }

  if (lan->endpointBulkIn != 0)
  {
    EndpointRelease(lan->endpointBulkIn);
    lan->endpointBulkIn = 0;
  }

  DeviceRelease(&lan->device);
}

#endif /* ENABLE_ETHER && (RPI < 3) */
