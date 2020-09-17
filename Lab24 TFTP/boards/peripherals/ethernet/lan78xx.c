/*...................................................................*/
/*                                                                   */
/*   Module:  lan78xx.c                                              */
/*   Version: 2019.0                                                 */
/*   Purpose: Microchip LAN78XX USB driver                           */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2019, Sean Lawless                    */
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
#include <assert.h>
#include <usb/request.h>
#include <usb/device.h>
#include <stdio.h>

#if ENABLE_ETHER && (RPI >= 3)

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define STATIC_MAC_ADDRESS FALSE

// This driver has been written based on FreeBSD lan78xx driver
//  Copyright (C) 2015 Microchip Technology
// Register map derived from if_lan78xxreg.h
// Remainder derived from if_lan78xx.c

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
#define FRAME_BUFFER_SIZE   1600
#define MAC_ADDRESS_SIZE    6
#define HS_USB_PKT_SIZE     512

#define DEFAULT_BURST_CAP_SIZE MAX_TX_FIFO_SIZE
#define DEFAULT_BULK_IN_DELAY 0x800

#define RX_HEADER_SIZE      (4 + 4 + 2)
#define TX_HEADER_SIZE      (4 + 4)

#define MAX_RX_FRAME_SIZE   (2 * 6 + 2 + 1500 + 4)

// USB vendor requests
#define WRITE_REGISTER      0xA0
#define READ_REGISTER       0xA1

// LAN 78xx Register Map
#define ID_REV              0x000
#define   ID_REV_CHIP_ID_MASK 0xFFFF0000
#define ID_REV_CHIP_ID_7800   0x7800
#define INT_STS             0x00C
#define INT_STS_CLEAR_ALL     0xFFFFFFFF
#define HW_CFG              0x010 // Hardware config
#define   HW_CFG_SRST         (1 << 0) /* soft reset */
#define   HW_CFG_LRST         (1 << 1) /* lite reset */
#define   HW_CFG_ETC          (1 << 3)
#define   HW_CFG_MEF          (1 << 4)
#define   HW_CFG_LED0_EN      (1 << 20)
#define   HW_CFG_LED1_EN      (1 << 21)
#define   HW_CFG_LED2_EN      (1 << 22)
#define   HW_CFG_LED3_EN      (1 << 22)
#define PMT_CTL             0x014
#define   PMT_CTL_PHY_WAKE_EN (1 << 2)
#define   PMT_CTL_WOL_EN      (1 << 3)
#define   PMT_CTL_PHY_RST     (1 << 4)
#define USB_CFG0            0x080
#define   USB_CFG_BCE         (1 << 5)
#define   USB_CFG_BIR         (1 << 6)
#define BURST_CAP           0x090
  #define BURST_CAP_SIZE_MASK 0x000000FF
#define BULK_IN_DLY         0x094
  #define BULK_IN_DLY_MASK    0x0000FFFF
#define INT_EP_CTL          0x098
  #define INT_EP_PHY_INT      (1 >> 17) /* PHY enable */
#define RFE_CTL             0x0B0
#define   RFE_CTL_IGMP_COE    (1 << 14)
#define   RFE_CTL_ICMP_COE    (1 << 13)
#define   RFE_CTL_TCPUDP_COE  (1 << 12)
#define   RFE_CTL_IP_COE      (1 << 11)
#define   RFE_CTL_BCAST_EN    (1 << 10)
#define   RFE_CTL_MCAST_EN    (1 << 9)
#define   RFE_CTL_UCAST_EN    (1 << 8)
#define   RFE_CTL_VLAN_FILTER (1 << 5)
#define   RFE_CTL_MCAST_HASH  (1 << 3)
#define RFE_CTL_DA_PERFECT    (1 << 1)
#define FCT_RX_CTL          0x0C0
#define   FCT_RX_CTL_EN       (1 << 31)
#define FCT_TX_CTL          0x0C4
#define   FCT_TX_CTL_EN       (1 << 31)

#define FCT_RX_FIFO_END     0x0C8
#define   FCT_RX_FIFO_END_MASK 0x0000007F
#define   MAX_RX_FIFO_SIZE    (12 * 1024)
#define FCT_TX_FIFO_END     0x0CC
#define   FCT_TX_FIFO_END_MASK 0x0000003F
#define   MAX_TX_FIFO_SIZE    (12 * 1024)

#define FCT_FLOW            0x0D0
#define MAC_CR              0x100
#define MAC_CR_AUTO_DUPLEX    (1 << 12)
 #define MAC_CR_AUTO_SPEED    (1 << 11)
#define MAC_RX              0x104
#define   MAC_RX_MAX_SIZE_MASK  0x3FFF0000
#define   MAC_RX_MAX_SIZE_SHIFT 16
#define   MAC_RX_RX_EN        (1 << 0)
#define MAC_TX              0x108
#define   MAC_TX_TXEN         (1 << 0)
#define FLOW                0x10C
#define RX_ADDRH            0x118
  #define RX_ADDRH_MASK_      0x0000FFFF
#define RX_ADDRL            0x11C
  #define RX_ADDRL_MASK_      0xFFFFFFFF
#define MII_ACCESS             0x120
#define   MII_ACC_MII_READ    (0 << 0)
#define   MII_ACC_MII_WRITE   (1 << 1)
#define   MII_ACC_MII_BUSY    (1 << 0)
#define MII_DATA            0x124
#define   MII_DATA_MASK       0x0000FFFF
#define PFILTER_BASE        0x400
#define   PFILTER_HIX         0x00
#define   PFILTER_LOX         0x04
#define   NUM_PFILTER_ADDRS   33
#define   PFILTER_HI(index)   (PFILTER_BASE + (8 * (index)) + (PFILTER_HIX))
#define   PFILTER_LO(index)   (PFILTER_BASE + (8 * (index)) + (PFILTER_LOX))
#define   PFILTER_ADDR_VALID  (1 << 31)
#define   PFILTER_HI_TYPE_SRC (1 << 30)
#define   PFILTER_HI_TYPE_DST (0 << 0)

// TX command A
#define TX_CMD_A_FCS        0x00400000
#define TX_CMD_A_LEN_MASK     0x000FFFFF

// RX command A
#define RX_CMD_A_RED        0x00400000
#define RX_CMD_A_LEN_MASK     0x00003FFF

// Lan78xx configuration state machine
#define STATE_READ_HW_RESET   0
#define STATE_WRITE_HW_RESET  1
#define STATE_WAIT_FOR_RESET  2
#define STATE_LOW_ADDRESS     3
#define STATE_HIGH_ADDRESS    4
#define STATE_BURST_CAP       5
#define STATE_IN_DELAY        6
#define STATE_READ_HW_CFG     7
#define STATE_WRITE_HW_CFG    8
#define STATE_READ_CFG0       9
#define STATE_WRITE_CFG0      10
#define STATE_RX_FIFO         11
#define STATE_TX_FIFO         12
#define STATE_INT_SYS         13
#define STATE_FLOW            14
#define STATE_FLOW_CONTROL    15
#define STATE_READ_RFE_CTL    16
#define STATE_WRITE_RFE_CTL   17
#define STATE_PHY_RESET       18
#define STATE_CHECK_PHY_RESET 19
#define STATE_READ_MAC_CR     20
#define STATE_WRITE_MAC_CR    21
#define STATE_PFILTER_LOW     22
#define STATE_PFILTER_HIGH    23
#define STATE_EP_CTL_CLEAR    24
#define STATE_READ_MAC_TX     25
#define STATE_WRITE_MAC_TX    26
#define STATE_READ_TX_CTL     27
#define STATE_WRITE_TX_CTL    28
#define STATE_READ_MAC_RX     29
#define STATE_WRITE_MAC_RX    30
#define STATE_READ_RX_CTL     31
#define STATE_WRITE_RX_CTL    32
#define STATE_FINISHED        33

/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
typedef struct Lan78xxDevice
{
  Device device;

  Endpoint endpointBulkIn;
  Endpoint endpointBulkOut;

  int configurationState;
  u8 address[MAC_ADDRESS_SIZE];

  u8 *txBuffer;
  u8 *rxBuffer;
  Request *rxURB;
  u8 TxBuffer[FRAME_BUFFER_SIZE];
  u8 RxBuffer[FRAME_BUFFER_SIZE];

}
Lan78xxDevice;

/*...................................................................*/
/* Global variables                                                  */
/*...................................................................*/
#if ENABLE_NETWORK
int NetStart(char *command);
int NetIn(u8 *frame, int frameLength);
#endif
Lan78xxDevice EtherDevice;
Lan78xxDevice *Eth0 = NULL;

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
static void write_reg(Lan78xxDevice *lan, u32 index,
      u32 value, void(*complete)(void *urb, void *param, void *context))
{
  static u32 Value;

  assert(lan != 0);

  Endpoint *endpoint = lan->device.endpoint0;
  Value = value; //Copy from stack to persistent (static) memory

  HostEndpointControlMessage(lan->device.host, endpoint,
            REQUEST_OUT | REQUEST_VENDOR, WRITE_REGISTER, 0, index,
            &Value, sizeof(u32), complete, complete ? lan : NULL);
}

/*...................................................................*/
/*   read_reg: Read data from a LAN register as USB request          */
/*                                                                   */
/*      Input: lan is the USB device                                 */
/*             index is the register number                          */
/*     Output: value is pointer to value modified upon success       */
/*             complete() is the function invoked upon completion    */
/*...................................................................*/
static void read_reg(Lan78xxDevice *lan, u32 index,
    u32 *value, void(*complete)(void *urb, void *param, void *context))
{
  assert(lan != 0);

  Endpoint *endpoint = lan->device.endpoint0;

  HostEndpointControlMessage(lan->device.host, endpoint,
            REQUEST_IN | REQUEST_VENDOR, READ_REGISTER, 0, index,
            value, sizeof(u32), complete, complete ? lan : NULL);
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
  Lan78xxDevice *lan = param;
  static u32 reg;

  if (urb)
    RequestRelease(urb);
  urb = NULL;

//  printf("lan78xx configure_complete - %d\n", lan->configurationState);
  if (lan->configurationState == STATE_READ_HW_RESET)
  {
    // Read HW config before HW reset
    read_reg(lan, HW_CFG, &reg, configure_complete);
  }
  else if (lan->configurationState == STATE_WRITE_HW_RESET)
  {
    // HW reset
    reg |= HW_CFG_LRST;
    write_reg(lan, HW_CFG, reg, configure_complete);
  }
  else if (lan->configurationState == STATE_WAIT_FOR_RESET)
  {
    // Wait for HW reset to complete
    read_reg(lan, HW_CFG, &reg, configure_complete);
  }
  else if (lan->configurationState == STATE_LOW_ADDRESS)
  {
    // If HW still resetting, reread without updating state
    if (reg & HW_CFG_LRST)
    {
      read_reg(lan, HW_CFG, &reg, configure_complete);
      return;
    }

    // Otherwise advance state and write low address of MAC
    else
    {
      u32 addressLow = ((u32)lan->address[0]) |
                       ((u32)lan->address[1] << 8) |
                       ((u32)lan->address[2] << 16) |
                       ((u32)lan->address[3] << 24);

      write_reg(lan, RX_ADDRL, addressLow, configure_complete);
    }
  }
  else if (lan->configurationState == STATE_HIGH_ADDRESS)
  {
    u32 addressHigh = ((u32)lan->address[4]) |
                      ((u32)lan->address[5] << 8);

    // Set high MAC address for Ethernet controller
    write_reg(lan, RX_ADDRH, addressHigh, configure_complete);
  }
  else if (lan->configurationState == STATE_BURST_CAP)
  {
    // USB high speed, enable burst capabilities
    write_reg(lan, BURST_CAP, DEFAULT_BURST_CAP_SIZE / HS_USB_PKT_SIZE, configure_complete);
  }
  else if (lan->configurationState == STATE_IN_DELAY)
  {
    // USB high speed, set bulk in delay to default
    write_reg(lan, BULK_IN_DLY, DEFAULT_BULK_IN_DELAY, configure_complete);
  }
  else if (lan->configurationState == STATE_READ_HW_CFG)
  {
    // Read HW_CFG before enabling
    read_reg(lan, HW_CFG, &reg, configure_complete);
  }
  else if (lan->configurationState == STATE_WRITE_HW_CFG)
  {
    // Disable Multiple Ethernet Frames (MEF) per USB packet.
    reg &= ~HW_CFG_MEF;

    // Enable both LEDs.
    reg |= HW_CFG_LED0_EN | HW_CFG_LED1_EN;
    write_reg(lan, HW_CFG, reg, configure_complete);
  }
  else if (lan->configurationState == STATE_READ_CFG0)
  {
    // Read USB_CFG0 before changing
    read_reg(lan, USB_CFG0, &reg, configure_complete);
  }
  else if (lan->configurationState == STATE_WRITE_CFG0)
  {
    // Enable burst CAP, disable NAK on RX FIFO empty
    reg &= ~USB_CFG_BIR;
    reg |= USB_CFG_BCE;
    write_reg(lan, USB_CFG0, reg, configure_complete);
  }
  else if (lan->configurationState == STATE_RX_FIFO)
  {
    // Set Rx FIFO size
    write_reg(lan, FCT_RX_FIFO_END, (MAX_RX_FIFO_SIZE - 512) / 512, configure_complete);
  }
  else if (lan->configurationState == STATE_TX_FIFO)
    // Set Tx FIFO size
    write_reg(lan, FCT_TX_FIFO_END, (MAX_TX_FIFO_SIZE - 512) / 512, configure_complete);
  else if (lan->configurationState == STATE_INT_SYS)
  {
    // Clear all the interrupt status bits
    write_reg(lan, INT_STS, INT_STS_CLEAR_ALL, configure_complete);
  }
  else if (lan->configurationState == STATE_FLOW)
  {
    // Disable flow control
    write_reg(lan, FLOW, 0, configure_complete);
  }
  else if (lan->configurationState == STATE_FLOW_CONTROL)
  {
    // Disable flow control
    write_reg(lan, FCT_FLOW, 0, configure_complete);
  }
  else if (lan->configurationState == STATE_READ_RFE_CTL)
  {
    // Read receive filter before enabling
    read_reg(lan, RFE_CTL, &reg, configure_complete);
  }
  else if (lan->configurationState == STATE_WRITE_RFE_CTL)
  {
    // Enable broadcast and perfect filter
    reg |= RFE_CTL_BCAST_EN | RFE_CTL_DA_PERFECT;
    write_reg(lan, RFE_CTL, reg, configure_complete);
  }
  else if (lan->configurationState == STATE_PHY_RESET)
  {
    // PHY reset
    write_reg(lan, PMT_CTL, PMT_CTL_PHY_RST, configure_complete);
  }
  else if (lan->configurationState == STATE_CHECK_PHY_RESET)
  {
    // Wait for HW reset to complete
    read_reg(lan, PMT_CTL, &reg, configure_complete);
  }
  else if (lan->configurationState == STATE_READ_MAC_CR)
  {
    // If HW still resetting, reread without updating state
    if (!(reg & PMT_CTL_PHY_RST))
    {
      read_reg(lan, PMT_CTL, &reg, configure_complete);
      return;
    }

    // Read the MAC configuration register
    read_reg(lan, MAC_CR, &reg, configure_complete);
  }
  else if (lan->configurationState == STATE_WRITE_MAC_CR)
  {
    // enable AUTO negotiation
    reg |= MAC_CR_AUTO_DUPLEX | MAC_CR_AUTO_SPEED;
    write_reg(lan, MAC_CR, reg, configure_complete);
  }
  else if (lan->configurationState == STATE_PFILTER_LOW)
  {
    u32 addressLow = ((u32)lan->address[0]) |
                     ((u32)lan->address[1] << 8) |
                     ((u32)lan->address[2] << 16) |
                     ((u32)lan->address[3] << 24);

    // Set the low filter MAC address
    write_reg(lan, PFILTER_LO(0), addressLow, configure_complete);
  }
  else if (lan->configurationState == STATE_PFILTER_HIGH)
  {
    u32 addressHigh = ((u32)lan->address[4]) |
                      ((u32)lan->address[5] << 8);

    // Set the high filter MAC address
    write_reg(lan, PFILTER_HI(0), addressHigh | PFILTER_ADDR_VALID,
              configure_complete);
  }
  else if (lan->configurationState == STATE_EP_CTL_CLEAR)
  {
    // Disable PHY interrupts
    write_reg(lan, INT_EP_CTL, 0, configure_complete);
  }
  else if (lan->configurationState == STATE_READ_MAC_TX)
  {
    // Read TX before enabling
    read_reg(lan, MAC_TX, &reg, configure_complete);
  }
  else if (lan->configurationState == STATE_WRITE_MAC_TX)
  {
    // Enable TX
    reg |= MAC_TX_TXEN;
    write_reg(lan, MAC_TX, reg, configure_complete);
  }
  else if (lan->configurationState == STATE_READ_TX_CTL)
    read_reg(lan, FCT_TX_CTL, &reg, configure_complete);
  else if (lan->configurationState == STATE_WRITE_TX_CTL)
  {
    reg |= FCT_TX_CTL_EN;
    write_reg(lan, FCT_TX_CTL, reg, configure_complete);
  }
  else if (lan->configurationState == STATE_READ_MAC_RX)
  {
    read_reg(lan, MAC_RX, &reg, configure_complete);
  }
  else if (lan->configurationState == STATE_WRITE_MAC_RX)
  {
    reg &= MAC_RX_MAX_SIZE_MASK;
    reg |= (MAX_RX_FRAME_SIZE << MAC_RX_MAX_SIZE_SHIFT) | MAC_RX_RX_EN;
    write_reg(lan, MAC_RX, reg, configure_complete);
  }
  else if (lan->configurationState == STATE_READ_RX_CTL)
  {
    read_reg(lan, FCT_RX_CTL, &reg, configure_complete);
  }
  else if (lan->configurationState == STATE_WRITE_RX_CTL)
  {
    reg |= FCT_RX_CTL_EN;
    write_reg(lan, FCT_RX_CTL, reg, configure_complete);
  }
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
    puts("Lan78xx configuration complete");

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
int configure(Device *device, void (complete)
              (void *urb, void *param, void *context), void *param)
{
  Lan78xxDevice *lan = (Lan78xxDevice *)device;
  const ConfigurationDescriptor *configDesc;
  const InterfaceDescriptor *interfaceDesc;
  const EndpointDescriptor *endpointDesc;

  assert (lan != 0);

  // Assign the Ethernet MAC address
#if STATIC_MAC_ADDRESS
  lan->address[0] = 0xB8;
  lan->address[1] = 0x27;
  lan->address[2] = 0xEB;
  lan->address[3] = 0xFE;
  lan->address[4] = 0xD9;
  lan->address[5] = 0x64;
  puts("MAC address is B8:27:EB:43:f5:07");
#else
  if (GetMACAddress(lan->address))
  {
    printf("Get MAC failed. Rebuild with STATIC_MAC_ADDRESS?\n");
    return FALSE;
  }
  putbyte(lan->address[0]); putbyte(lan->address[1]);
  putbyte(lan->address[2]); putbyte(lan->address[3]);
  putbyte(lan->address[4]); putbyte(lan->address[5]);
  puts(" MAC ID");
#endif

  // check USB configuration

  configDesc =
    (ConfigurationDescriptor *)DeviceGetDescriptor(
               lan->device.configParser, DESCRIPTOR_CONFIGURATION);
  if ((configDesc == 0) || (configDesc->numInterfaces != 1))
  {
    puts("Lan78xx configuration descriptor invalid");
    return FALSE;
  }

  interfaceDesc =
    (InterfaceDescriptor *)DeviceGetDescriptor(
                   lan->device.configParser, DESCRIPTOR_INTERFACE);
  if ((interfaceDesc == 0) ||
      (interfaceDesc->interfaceNumber != 0x00) ||
      (interfaceDesc->alternateSetting != 0x00) ||
      (interfaceDesc->numEndpoints != 3))
  {
    puts("Lan78xx interface descriptor invalid");
    return FALSE;
  }

  // Loop on all endpoint descriptors
  while ((endpointDesc = (EndpointDescriptor *)
      DeviceGetDescriptor(lan->device.configParser,
                                          DESCRIPTOR_ENDPOINT)) != 0)
  {
    // Examine further if endpoint is bulk type
    if ((endpointDesc->attributes & 0x3F) == 0x02)
    {
      // Assign the bulk in endpoint is input
      if ((endpointDesc->endpointAddress & 0x80) == 0x80)
        EndpointFromDescr(&lan->endpointBulkIn, &lan->device,
                          endpointDesc);

      // Otherwise assign bulk out endpoint
      else
        EndpointFromDescr(&lan->endpointBulkOut, &lan->device,
                          endpointDesc);
    }
  }

  if ((lan->endpointBulkIn.type != EndpointTypeBulk) ||
      (lan->endpointBulkOut.type != EndpointTypeBulk))
  {
    puts("Lan78xx bulk endpoints not configured");
    return FALSE;
  }

  // Configure the USB device
  if (!DeviceConfigure(&lan->device, configure_complete, lan))
  {
    puts("Cannot configure lan78xx");
    return FALSE;
  }

  // Return configuration success
  return TRUE;
}

/*...................................................................*/
/* receive_complete: Callback for Rx inbound Ethernet frame          */
/*                                                                   */
/*      Input: request is the USB request or URB                     */
/*             param is a void pointer to the LAN USB device         */
/*             context is not used                                   */
/*...................................................................*/
static void receive_complete(void *request, void *param, void *context)
{
  Request *urb = request;
  u32 resultLength, rxStatus, frameLength;
  Lan78xxDevice *lan = param;
  u8 *buffer;

  assert (lan != 0);
  assert (urb != 0);

  resultLength = urb->resultLen;
  if (resultLength < RX_HEADER_SIZE)
    goto finished;

  buffer = lan->rxBuffer;
  rxStatus = *(u32 *)buffer;
  if (rxStatus & RX_CMD_A_RED)
  {
    printf("RX error (status 0x%X)", rxStatus);
    goto finished;
  }

  // Read the frame buffer from the URB Rx status
  frameLength = rxStatus & RX_CMD_A_LEN_MASK;
  if (frameLength <= 4)
    goto finished;

//  putbyte(frameLength); puts(" IN frame bytes");

#if ENABLE_NETWORK
  // Remove RX status, empty VLAN and padding from the frame
  frameLength -= 4;
  buffer += 10;

  // Inform the network stack of the new frame
  NetIn(buffer, frameLength);
#endif /* ENABLE_NETWORK */

finished:

  //Reuse urb and start another async request
  RequestRelease(urb);
  RequestAttach(urb, &lan->endpointBulkIn, lan->rxBuffer,
                FRAME_BUFFER_SIZE, 0);

  //Register the callback and submit the request asynchronously
  RequestSetCompletionRoutine(urb, receive_complete, lan, NULL);
  HostSubmitAsyncRequest(urb, lan->device.host, NULL);

  return;
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
  Lan78xxDevice *lan = Eth0;

  if (lan == NULL)
    return -1;

  assert(lan->rxURB == 0);
  lan->rxURB = NewRequest();
  assert(lan->rxURB != 0);
  bzero(lan->rxURB, sizeof(Request));

  lan->rxBuffer = lan->RxBuffer;

  // Create and submit request to the bulk in endpoint
  RequestAttach(lan->rxURB, &lan->endpointBulkIn, lan->rxBuffer,
                FRAME_BUFFER_SIZE, 0);
  RequestSetCompletionRoutine(lan->rxURB, receive_complete, lan, NULL);
  HostSubmitAsyncRequest(lan->rxURB, lan->device.host, NULL);
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
  Lan78xxDevice *lan = Eth0;;
  assert(lan != 0);

  if (length > FRAME_BUFFER_SIZE - TX_HEADER_SIZE)
  {
    return FALSE;
  }

  assert(lan->txBuffer != 0);
  assert (buffer != 0);
  memcpy(lan->txBuffer+TX_HEADER_SIZE, buffer, length);

  *(u32 *)&lan->txBuffer[0] = (length & TX_CMD_A_LEN_MASK) |
                              TX_CMD_A_FCS;
  *(u32 *)&lan->txBuffer[4] = 0;

  return HostEndpointTransfer(lan->device.host, &lan->endpointBulkOut,
                    lan->txBuffer, length + TX_HEADER_SIZE, NULL) >= 0;
}

/*...................................................................*/
/*  LanGetMAC: Retrieve pointer to unmodifiable MAC address          */
/*                                                                   */
/*    Returns: Constant (unmodifiable) pointer to the MAC Id         */
/*...................................................................*/
const u8 *LanGetMAC(void)
{
  if (Eth0)
    return (const u8 *)Eth0->address;
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
  Lan78xxDevice *lan = &EtherDevice;

  assert(lan != 0);

  DeviceCopy(&lan->device, device);
  lan->device.configure = configure;

  lan->configurationState = 0;

  bzero(&lan->endpointBulkIn, sizeof(Endpoint));
  bzero(&lan->endpointBulkOut, sizeof(Endpoint));
  lan->configurationState = 0;
  lan->txBuffer = lan->TxBuffer;

  assert(lan->txBuffer != 0);
  return lan;
}

/*...................................................................*/
/* LanDeviceRelease: Release the LAN device                          */
/*                                                                   */
/*      Input: device is the LAN USB device                          */
/*...................................................................*/
void LanDeviceRelease(Device *device)
{
  Lan78xxDevice *lan = (void *)device;

  assert(lan != 0);

  if (lan->txBuffer != 0)
    lan->txBuffer = 0;

  if (lan->endpointBulkOut.type)
  {
    EndpointRelease(&lan->endpointBulkOut);
    lan->endpointBulkOut.type = 0;
  }

  if (lan->endpointBulkIn.type)
  {
    EndpointRelease(&lan->endpointBulkIn);
    lan->endpointBulkIn.type = 0;
  }

  DeviceRelease(&lan->device);
}

#endif /* ENABLE_ETHER && (RPI >= 3) */
