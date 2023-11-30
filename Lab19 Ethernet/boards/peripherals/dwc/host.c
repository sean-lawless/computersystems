/*...................................................................*/
/*                                                                   */
/*   Module:  host.c                                                 */
/*   Version: 2020.0                                                 */
/*   Purpose: Host Controller for DesignWare                         */
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
#include <system.h>
#if ENABLE_USB
#include <board.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <usb/request.h>
#include <usb/device.h>
#include "dwhc.h"
#include "transfer.h"

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define RX_FIFO_SIZE             1024
#define NONPERIODIC_TX_FIFO_SIZE 1024
#define PERIODIC_TX_FIFO_SIZE    1024
#define MAX_CHANNELS             16

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
//
// Core Registers
//  Translated from FreeBSD dwc_otgreg.h
//    BSD-2-Clause-FreeBSD, Copyright 2010,2011 Aleksander Rybalko
//
#define OTG_CTL                (USB_BASE + 0x00) // OTG control
#define OTG_INT                (USB_BASE + 0x04) // OTG interrupt
#define AHB_CFG                (USB_BASE + 0x08) // AHB configuration
#define   GLOBAL_INT_ENABLE      (1 << 0)
#define   BURST_INCR             (1 << 1)
#define   BURST_INCR4            (3 << 1)
#define   BURST_INCR8            (5 << 1)
#define   BURST_INCR16           (7 << 1)
#define   AHB_CFG_DMAENABLE      (1 << 5)
#define USB_CFG                (USB_BASE + 0x00C) // USB configuration
#define   USB_CFG_PHYIF          (1 << 3)
#define   USB_CFG_ULPI_UTMI_SEL  (1 << 4)
#define   USB_CFG_SRP_CAPABLE    (1 << 8)
#define   USB_CFG_HNP_CAPABLE    (1 << 9)
#define   USB_CFG_ULPI_FSLS      (1 << 17)
#define   USB_CFG_ULPI_CLK_SUS_M (1 << 19)
#define   USB_CFG_ULPI_EXT_VBUS_DRV (1 << 20)
#define   USB_CFG_TERM_SEL_DL_PULSE (1 << 22)
#define RST_CTL                (USB_BASE + 0x010) // Reset control
#define   RST_CTL_CSFT_RST       (1 << 0)
#define   RST_CTL_HSFT_RST       (1 << 1)
#define   RST_CTL_RX_FFLSH       (1 << 4) // Rx FIFO flush
#define   RST_CTL_TX_FFLSH       (1 << 5) // Tx FIFO flush
#define   RST_CTL_TX_FNUM_OFFSET 6
#define   RST_CTL_TX_FNUM_MASK   (0x1F << 6)
#define   RST_CTL_AHB_IDLE       (1 << 31)
#define INT_STS                (USB_BASE + 0x014) // Interrupt status
#define   INT_STS_SOF_INTR       (1 << 3)  // soft interrupt
#define   INT_STS_PORT_INTR      (1 << 24) // port interrupt
#define   INT_STS_HC_INTR        (1 << 25) // Host interrupt
#define INT_MSK                (USB_BASE + 0x018) // Interrupt Mask
#define   INT_MSK_MODE_MISMATCH  (1 << 1)
#define   INT_MSK_SOF_INTR       (1 << 3)
#define   INT_MSK_RX_STS_QLVL    (1 << 4)
#define   INT_MSK_USB_SUSPEND    (1 << 11)
#define   INT_MSK_USB_RESET      (1 << 12)
#define   INT_MSK_PORT_INTR      (1 << 24)
#define   INT_MSK_HC_INTR        (1 << 25)
#define   INT_MSK_DISCONNECT     (1 << 29)
#define   INT_MSK_SESS_REQ_INTR  (1 << 30)
#define   INT_MSK_WK_UP_INTR     (1 << 31)
#define FIFO_SIZE              (USB_BASE + 0x024)
#define HW_CFG2                (USB_BASE + 0x048)
#define   HW_CFG2_ARCHITECTURE(reg) (((reg) >> 3) & 3)
#define   HW_CFG2_HS_PHY_TYPE(reg)  (((reg) >> 6) & 3)
#define     HW_CFG2_HS_PHY_TYPE_NOT_SUPPORTED 0
#define     HW_CFG2_HS_PHY_TYPE_UTMI 1
#define     HW_CFG2_HS_PHY_TYPE_ULPI 2
#define     HW_CFG2_HS_PHY_TYPE_UTMI_ULPI 3
#define   HW_CFG2_FS_PHY_TYPE(reg)  (((reg) >> 8) & 3)
#define     HW_CFG2_FS_PHY_TYPE_DEDICATED 1
#define     HW_CFG2_NUM_HOST_CHANNELS(reg) ((((reg) >> 14) & 0xF) + 1)

//
// Host Registers
//  Translated from FreeBSD dwc_otgreg.h
//    BSD-2-Clause-FreeBSD, Copyright 2010,2011 Aleksander Rybalko
//
#define HCFG                   (USB_BASE + 0x400)
#define   HCFG_FSLS_PCLK_SEL_OFFSET 0
#define   HCFG_FSLS_PCLK_SEL_MASK (3 << 0)
#define     DWC_HCFG_30_60_MHZ     0
#define     DWC_HCFG_48_MHZ        1
#define     DWC_HCFG_6_MHZ         2
#define HOST_FRM_NUM           (USB_BASE + 0x408)
#define   HOST_FRM_NUM_NUMBER(reg)      ((reg) & 0xFFFF)
#define     MAX_FRAME_NUMBER      0x3FFF
#define   HOST_FRM_NUM_REMAINING(reg)   (((reg) >> 16) & 0xFFFF)
#define ACH_INT                (USB_BASE + 0x414) // All chan interrupt
#define ACH_INT_MSK            (USB_BASE + 0x418) // interrupt mask
#define HPRT                   (USB_BASE + 0x440) // Host port
#define   PRT_CONN_STS           (1 << 0) // connection status
#define   PRT_CONN_DET           (1 << 1) // connection detected
#define   PRT_ENA                (1 << 2) // port enable
#define   PRT_ENA_CHNG           (1 << 3) // port changed
#define   PRT_OVR_CURR_ACT       (1 << 4) // over current
#define   PRT_OVR_CURR_CHNG      (1 << 5) // over current changed
#define   PRT_RST                (1 << 8) // port reset
#define   PRT_PWR                (1 << 12)// port power
#define   PRT_SPD(reg)           (((reg) >> 17) & 3) // port speed
#define     DWC_HPRT0_PTRSPD_HIGH  0
#define     DWC_HPRT0_PTRSPD_FULL  1
#define     DWC_HPRT0_PTRSPD_LOW   2

/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
typedef enum
{
  StageStateNoSplitTransfer,
  StageStateStartSplit,
  StageStateCompleteSplit,
  StageStatePeriodicDelay,
  StageStateUnknown
}
StageState;

typedef enum
{
  StageSubStateWaitForChannelDisable,
  StageSubStateWaitForTransactionComplete,
  StageSubStateUnknown
}
StageSubState;

typedef struct Host
{
  u32 channels, channelAllocated;
  void *stageData[MAX_CHANNELS];
  void *rootPort;
} Host;

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
Host HostController;
TransferStageData StageData[MAX_USB_REQUESTS];

/*...................................................................*/
/* External Function Prototypes                                      */
/*...................................................................*/
extern void *RootPortAttach(struct Host *host);
extern void RootPortRelease(Device *rp);
extern int RootPortInitialize(void *root);

/*...................................................................*/
/* Static Local Functions                                            */
/*...................................................................*/

/*...................................................................*/
/* new_stage_data: Find a free TransferStageData structure           */
/*                                                                   */
/*    Returns: pointer to the new structure                          */
/*...................................................................*/
static TransferStageData *new_stage_data(void)
{
  int i;

  for (i = 0; i < MAX_USB_REQUESTS; ++i)
  {
    if ((StageData[i].endpoint == NULL) &&
        (StageData[i].urb == NULL))
    {
      StageData[i].endpoint = (void *)-1;
      return &StageData[i];
    }
  }
  return NULL;
}

/*...................................................................*/
/* free_stage_data: Move stage data structure to free list           */
/*                                                                   */
/*      Input: stage is the pointer to the structure to free         */
/*...................................................................*/
static void free_stage_data(TransferStageData *stage)
{
  int i;

  stage->endpoint = NULL;
  stage->urb = NULL;

  for (i = 0; i < MAX_USB_REQUESTS; ++i)
  {
    if (&StageData[i] == stage)
    {
      return;
    }
  }
  putchar('e');
}

/*...................................................................*/
/* host_attach: Attach host to the root port                         */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*...................................................................*/
static void host_attach(Host *host)
{
  u32 channel = 0;

  assert(host != 0);

  host->channels = 0;
  host->channelAllocated = 0;
  host->rootPort = RootPortAttach(host);

  for (channel = 0; channel < MAX_CHANNELS; channel++)
  {
    host->stageData[channel] = 0;
  }
}

/*...................................................................*/
/* host_release: Release hosts' root port                            */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*...................................................................*/
static void host_release(Host *host)
{
  RootPortRelease(host->rootPort);
}

/*...................................................................*/
/* transfer_complete: Callback for configure state machine           */
/*                                                                   */
/*      Input: request is the USB Request Buffer (URB)               */
/*             param is a pointer to the LAN USB device              */
/*             context is not used                                   */
/*...................................................................*/
static void transfer_complete(void *request, void *param,
                              void *context)
{
  Request *urb = request;

  RequestRelease(urb);
  FreeRequest(urb);
}

/*...................................................................*/
/* new_channel: Callback for configure state machine                 */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*                                                                   */
/*    Returns: the new channel number                                */
/*...................................................................*/
static u32 new_channel(Host *host)
{
  u32 channelMask = 1, channel = 0;

  assert(host != 0);

  for (channel = 0; channel < host->channels; channel++)
  {
    if (!(host->channelAllocated & channelMask))
    {
      host->channelAllocated |= channelMask;
      return channel;
    }

    channelMask <<= 1;
  }

  return MAX_CHANNELS;
}

/*...................................................................*/
/* free_channel: Free up use of a host channel                       */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*             channel is the channel to free up                     */
/*...................................................................*/
static void free_channel(Host *host, u32 channel)
{
  assert(host != 0);

  assert(channel < host->channels);
  u32 channelMask = 1 << channel;

  assert(host->channelAllocated & channelMask);
  host->channelAllocated &= ~channelMask;
}

/*...................................................................*/
/* wait_for_bit: Wait for a bit to be set on a register              */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*             reg is the hardware register to read                  */
/*             mask is the bitmask to apply to the register read     */
/*             set is to wait for bit to be set (1) or cleared (0)   */
/*             timeout is the timeout in milliseconds                */
/*                                                                   */
/*    Returns: TRUE if the bit is set/cleared, FALSE on timeout      */
/*...................................................................*/
static int wait_for_bit(Host *host, u32 reg, u32 mask,
                        int set, u32 timeout)
{
  assert(host != 0);
  assert(reg != 0);
  assert(mask != 0);
  assert(timeout > 0);

  while ((REG32(reg) & mask) ? !set : set)
  {
    usleep(MICROS_PER_MILLISECOND);

    if (--timeout == 0)
      return FALSE;
  }

  return TRUE;
}

/*...................................................................*/
/* enable_root_port: Enable the host software root port device       */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*                                                                   */
/*    Returns: TRUE on success, FALSE if error                       */
/*...................................................................*/
static int enable_root_port(Host *host)
{
  assert(host != 0);
  u32 port;

  if (!wait_for_bit(host, HPRT, PRT_CONN_STS, TRUE, 20))
    return FALSE;

  // Reset the port
  port = REG32(HPRT);
  port &= ~(PRT_CONN_DET | PRT_ENA | PRT_ENA_CHNG | PRT_OVR_CURR_CHNG);
  port |= PRT_RST;
  REG32(HPRT) = port;

  usleep(100 * MICROS_PER_MILLISECOND); // USB 2.0 tDRSTR

  // Clear the reset for the port
  port = REG32(HPRT);
  port &= ~(PRT_CONN_DET | PRT_ENA | PRT_ENA_CHNG | PRT_OVR_CURR_CHNG);
  port &= ~PRT_RST;
  REG32(HPRT) = port;

  usleep(60 * MICROS_PER_MILLISECOND); // USB 2.0 tRSTRCY

  return TRUE;
}

/*...................................................................*/
/* enable_global_interrupt: Enable USB controller global interrupts  */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*...................................................................*/
static void enable_global_interrupt(Host *host)
{
  u32 config;

  assert (host != 0);

  config = REG32(AHB_CFG) | GLOBAL_INT_ENABLE;
  REG32(AHB_CFG) = config;
}

/*...................................................................*/
/* clear_global_interrupt: Clear all global interrupts for USB host  */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*...................................................................*/
static void clear_host_interrupt(Host *host)
{
  /* Clear all pending interrupts. */
  REG32(INT_STS) = 0xFFFFFFFF;
}

/*...................................................................*/
/* enable_host_interrupt: Enable interrupts for USB host             */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*...................................................................*/
static void enable_host_interrupt(Host *host)
{
  u32 mask;
  assert(host != 0);

  // Disable core interrupts by clearing the mask. */
  REG32(INT_MSK) = 0;

  clear_host_interrupt(host);

  mask = REG32(INT_MSK);
  mask |= INT_MSK_HC_INTR | INT_MSK_PORT_INTR | INT_MSK_DISCONNECT;

  REG32(INT_MSK) = mask;
}

/*...................................................................*/
/* enable_channel_interrupt: Enable interrupts for a USB channel     */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*             channel is the channel to enable interrupts           */
/*...................................................................*/
static void enable_channel_interrupt(Host *host, u32 channel)
{
  u32 mask;

  mask = REG32(ACH_INT_MSK);
  mask |= 1 << channel;
  REG32(ACH_INT_MSK) = mask;
}

/*...................................................................*/
/* disable_channel_interrupt: Disable interrupts for a USB channel   */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*             channel is the channel to disable interrupts          */
/*...................................................................*/
static void disable_channel_interrupt(Host *host, u32 channel)
{
  u32 mask;
  assert(host != 0);

  /* Clear this channels mask bit all channels mask. */
  mask = REG32(ACH_INT_MSK);
  mask &= ~(1 << channel);
  REG32(ACH_INT_MSK) = mask;
}

/*...................................................................*/
/* flush_tx_fifo: Flush the hardware transmit FIFO queue             */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*             fifo is the FIFO queue to flush                       */
/*...................................................................*/
static void flush_tx_fifo(Host *host, u32 fifo)
{
  u32 reset;

  assert(host != 0);
  reset = RST_CTL_TX_FFLSH | fifo << RST_CTL_TX_FNUM_OFFSET;
  reset &= ~RST_CTL_TX_FNUM_MASK;
  REG32(RST_CTL) = reset;

  if (wait_for_bit(host, RST_CTL, RST_CTL_TX_FFLSH, FALSE, 10))
    usleep(1);
}

/*...................................................................*/
/* flush_rx_fifo: Flush the hardware receive FIFO queue              */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*             fifo is the FIFO queue to flush                       */
/*...................................................................*/
static void flush_rx_fifo(Host *host)
{
  u32 reset;
  assert(host != 0);

  reset = RST_CTL_RX_FFLSH;
  REG32(RST_CTL) = reset;

  if (wait_for_bit(host, RST_CTL, RST_CTL_RX_FFLSH,
                        FALSE, 10))
    usleep(1);
}

/*...................................................................*/
/* flush_tx_fifo: Reset the USB host controller                      */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*                                                                   */
/*    Returns: TRUE on success, FALSE otherwise                      */
/*...................................................................*/
static int reset(Host *host)
{
  u32 reset;
  assert(host != 0);

  reset = 0;

  // Wait for AHB to be in the IDLE state
  if (!wait_for_bit(host, RST_CTL, RST_CTL_AHB_IDLE, TRUE, 100))
    return FALSE;

  // Perform a soft reset of the controller
  reset |= RST_CTL_CSFT_RST;
  REG32(RST_CTL) = reset;

  if (!wait_for_bit(host, RST_CTL, RST_CTL_CSFT_RST,
                        FALSE, 10))
  {
    return FALSE;
  }

  usleep(200 * MICROS_PER_MILLISECOND); // USB 2.0

  return TRUE;
}

/*...................................................................*/
/* initialize_core: Initialize the core USB controller               */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*                                                                   */
/*    Returns: TRUE on success, FALSE otherwise                      */
/*...................................................................*/
static int initialize_core(Host *host)
{
  u32 config;
  assert(host != 0);

  /* Turn off the core voltage and pulse. */
  config = REG32(USB_CFG);
  config &= ~(USB_CFG_ULPI_EXT_VBUS_DRV |
              USB_CFG_TERM_SEL_DL_PULSE);
  REG32(USB_CFG) = config;

  /* Reset the host controller. */
  if (!reset(host))
  {
    puts("Reset failed");
    return FALSE;
  }

  /* Turn off the PHY and UTMI. */
  config = REG32(USB_CFG);
  config &= ~(USB_CFG_ULPI_UTMI_SEL |
              USB_CFG_PHYIF);
  REG32(USB_CFG) = config;

  // Internal DMA mode only
  assert(HW_CFG2_ARCHITECTURE(REG32(HW_CFG2)) == 2);

  config = REG32(USB_CFG);
  if (HW_CFG2_HS_PHY_TYPE(REG32(HW_CFG2) == HW_CFG2_HS_PHY_TYPE_ULPI) &&
   HW_CFG2_FS_PHY_TYPE(REG32(HW_CFG2) == HW_CFG2_FS_PHY_TYPE_DEDICATED))
  {
    config |= USB_CFG_ULPI_FSLS |
              USB_CFG_ULPI_CLK_SUS_M;
  }
  else
  {
    config &= ~(USB_CFG_ULPI_FSLS |
                USB_CFG_ULPI_CLK_SUS_M);
  }

  assert (host->channels == 0);
  host->channels = HW_CFG2_NUM_HOST_CHANNELS (REG32(HW_CFG2));
  assert(4 <= host->channels && host->channels <= MAX_CHANNELS);

  // Enable DMA through AHB
  config = REG32(AHB_CFG);
  config |= AHB_CFG_DMAENABLE;
  REG32(AHB_CFG) = config;

  // HNP and SRP are not used
  config = REG32(USB_CFG);
  config &= ~(USB_CFG_HNP_CAPABLE |
              USB_CFG_SRP_CAPABLE);
  REG32(USB_CFG) = config;

  return TRUE;
}

/*...................................................................*/
/* initialize_host: Initialize the USB host                          */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*                                                                   */
/*    Returns: TRUE on success, FALSE otherwise                      */
/*...................................................................*/
static int initialize_host(Host *hostController)
{
  u32 size, host, config, core_config;
  assert(hostController != 0);

  // First clear all the host interrupts
  clear_host_interrupt(hostController);

  // Disable the clock before configuring
  host = REG32(HCFG);
  host &= ~HCFG_FSLS_PCLK_SEL_MASK;
  REG32(HCFG) = host;

  // Read the USB and HW configuration registers
  config = REG32(USB_CFG);
  core_config = REG32(HW_CFG2);

  // If ULPI and dedicated with FSLS, enable 48 MHz
  if ((HW_CFG2_HS_PHY_TYPE(core_config) == HW_CFG2_HS_PHY_TYPE_ULPI) &&
      (HW_CFG2_FS_PHY_TYPE(core_config) ==
       HW_CFG2_FS_PHY_TYPE_DEDICATED) && (config & USB_CFG_ULPI_FSLS))
    host |= DWC_HCFG_48_MHZ;

  // Otherwise 30/60 MHz
  else
    host |= DWC_HCFG_30_60_MHZ;

  // Commit host configuration clock changes
  REG32(HCFG) = host;

  // Configure HW FIFO Rx and Tx queues
  REG32(FIFO_SIZE) = RX_FIFO_SIZE;

  size = 0;
  size |= (RX_FIFO_SIZE | (NONPERIODIC_TX_FIFO_SIZE << 16));
  REG32(NONPERIODIC_TX_FIFO_SIZE) = size;

  size = 0;
  size |= PERIODIC_TX_FIFO_SIZE << 16;
  REG32(PERIODIC_TX_FIFO_SIZE) = size;

  // Flush Rx and Tx FIFOs
  flush_rx_fifo(hostController);
  flush_tx_fifo(hostController, 0x10);

  // Read host port register
  host = REG32(HPRT);

  // If not powered, power the host port
  if (!(REG32(HPRT) & PRT_PWR))
  {
    // Clear host port changes
    host &= ~(PRT_CONN_DET | PRT_ENA | PRT_ENA_CHNG |PRT_OVR_CURR_CHNG);
    host |= PRT_PWR;
    REG32(HPRT) = host;
  }

  // Enbale the host interrupts
  enable_host_interrupt(hostController);

  return TRUE;
}

/*...................................................................*/
/* start_transaction: start a USB request transaction                */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*             stageData is the transfer to start                    */
/*...................................................................*/
static void start_transaction(Host *host, TransferStageData *stageData)
{
  u32 character, size, channel;
  FrameScheduler *frameScheduler;

  assert(host != 0);
  assert(stageData != 0);

  channel = stageData->channel;
  assert(channel < host->channels);

  // Check if channel is already enabled
  character = REG32(HC_CHAR(channel));
  if (character & HC_CHAR_EN)
  {
    /* If enabled then set state to wait for disable. */
    stageData->subState = StageSubStateWaitForChannelDisable;

    /* Disable the channel. */
    character &= ~HC_CHAR_EN;
    character |= HC_CHAR_DIS;
    REG32(HC_CHAR(channel)) = character;

    /* Halt interrupts for the channel. */
    REG32(HC_INT_MSK(channel)) = HC_INT_CH_HLTD;
    printf("Channel %d disabled\n", channel);
    return;
  }

  stageData->subState = StageSubStateWaitForTransactionComplete;

  /* Clear all pending host channel interrupts. */
  REG32(HC_INT(channel)) = 0xFFFFFFFF;

  // set transfer size, packet count and pid
  size = 0;
  size |= stageData->bytesPerTransaction & HC_T_SIZ_XFER_SIZE_MASK;
  size |=(stageData->packetsPerTransaction <<HC_T_SIZ_PKT_CNT_OFFSET) &
          HC_T_SIZ_PKT_CNT_MASK;
  size |= TransferStageDataGetPID(stageData) << HC_T_SIZ_PID_OFFSET;
  REG32(HC_T_SIZ(channel)) = size;

  // set DMA address
  REG32(HC_DMA_ADDR(channel)) =
                          (u32)stageData->bufferPointer | GPU_MEM_BASE;

  // Set split control
  size = 0;
  if (stageData->splitTransaction)
  {
    size |= ((Device *)stageData->device)->hubPortNumber;
    size |= ((Device *)stageData->device)->hubAddress <<
                                HC_SPLT_HUB_ADDR_OFFSET;
    size |= HC_SPLT_XACT_POS_ALL << HC_SPLT_XACT_POS_OFFSET;
    if (stageData->splitComplete)
      size |= HC_SPLT_COMP_SPLT;
    size |= HC_SPLT_SPLT_ENA;
  }
  REG32(HC_SPLT(channel)) = size;

  // set channel parameters
  size = HC_CHAR(channel);
  size &= ~HC_CHAR_MPS_MASK;
  size |= stageData->maxPacketSize &
                        HC_CHAR_MPS_MASK;
  size &= ~HC_CHAR_MULTI_CNT_MASK;
  size |= 1 << HC_CHAR_MULTI_CNT_OFFSET;

  if (stageData->in)
    size |= HC_CHAR_EP_DIR_IN;
  else
    size &= ~HC_CHAR_EP_DIR_IN;

  if (stageData->speed == SpeedLow)
    size |= HC_CHAR_LSPD_DEV;
  else
    size &= ~HC_CHAR_LSPD_DEV;

  size &= ~HC_CHAR_DEV_ADDR_MASK;
  size |= ((Device *)stageData->device)->address <<
                                               HC_CHAR_DEV_ADDR_OFFSET;

  size &= ~HC_CHAR_EP_TYPE_MASK;
  size |= TransferStageDataGetEndpointType(stageData) <<
                                                HC_CHAR_EP_TYPE_OFFSET;

  size &= ~HC_CHAR_EP_NUM_MASK;
  size |= ((Endpoint *)stageData->endpoint)->number <<
                                                 HC_CHAR_EP_NUM_OFFSET;

  frameScheduler = stageData->frameScheduler;
  if (frameScheduler != 0)
  {
    frameScheduler->waitForFrame(frameScheduler);

    if (frameScheduler->isOddFrame(frameScheduler))
    {
      size |= HC_CHAR_ODD_FRM;
    }
    else
    {
      size &= ~HC_CHAR_ODD_FRM;
    }
  }

  REG32(HC_INT_MSK(channel))=TransferStageDataGetStatusMask(stageData);

  size |= HC_CHAR_EN;
  size &= ~HC_CHAR_DIS;
  REG32(HC_CHAR(channel)) = size;
}

/*...................................................................*/
/* transfer_stage_async: transfer a stage of data asynchonously      */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*             urb is the USB Request Buffer (URB)                   */
/*             in is TRUE if inbound, FALSE if outbound              */
/*             statusStage is the stage of the transaction           */
/*                                                                   */
/*    Returns: TRUE on success, FALSE if error                       */
/*...................................................................*/
static int transfer_stage_async(Host *host, void *urb, int in,
                                int statusStage)
{
  TransferStageData *stageData;
  u32 channel;

  assert(host != 0);
  assert(urb != 0);

  // Find an unused channel
  channel = new_channel(host);
  if (channel >= host->channels)
  {
    puts("new_channel() failed");
    return FALSE;
  }

  // Find an unused transfer stage data structure and attach URB to it
  stageData = new_stage_data();
  assert (stageData != 0);
  TransferStageDataAttach(stageData, channel, urb, in, statusStage);

  // Enable the channel transfer
  assert(host->stageData[channel] == 0);
  host->stageData[channel] = stageData;
  enable_channel_interrupt(host, channel);

  // If no split then start at no split stage of transfer
  if (!stageData->splitTransaction)
  {
    stageData->state = StageStateNoSplitTransfer;
  }

  // Otherwise start at the split transaction stage
  else
  {
    stageData->state = StageStateStartSplit;
    stageData->splitComplete = FALSE;

    assert(stageData->frameScheduler != 0);
    stageData->frameScheduler->startSplit(stageData->frameScheduler);
  }

  // Begin the host transaction
  start_transaction(host, stageData);

  return TRUE;
}

/*...................................................................*/
/* stage_interval: start a periodic transfer                         */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*             urb is the USB Request Buffer (URB)                   */
/*             in is TRUE if inbound, FALSE if outbound              */
/*             statusStage is the stage of the transaction           */
/*                                                                   */
/*    Returns: TASK_FINISHED to indicate completion                  */
/*...................................................................*/
static int stage_interval(u32 tmr, void *param, void *context)
{
  Host *host = (Host *)context;
  TransferStageData *stageData = (TransferStageData *)param;

  assert(host != 0);
  assert(stageData != 0);
  assert(stageData->state == StageStatePeriodicDelay);

  if (stageData->splitTransaction)
  {
    stageData->state = StageStateStartSplit;

    stageData->splitComplete = FALSE;
    assert(stageData->frameScheduler != 0);
    stageData->frameScheduler->startSplit(stageData->frameScheduler);
  }
  else
    stageData->state = StageStateNoSplitTransfer;

  start_transaction(host, stageData);
  return TASK_FINISHED;
}

/*...................................................................*/
/* process_channel_interrupt: Process interrupts for a channel       */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*             channel is the to process interrupts for              */
/*...................................................................*/
static void process_channel_interrupt(Host *host, u32 channel)
{
  u32 interval, status;
  assert(host != 0);

  TransferStageData *stageData = host->stageData[channel];
  assert(stageData != 0);
  FrameScheduler *frameScheduler = stageData->frameScheduler;
  Request *urb = stageData->urb;
  assert (urb != 0);
  assert(channel < MAX_CHANNELS);
  switch (stageData->subState)
  {
    case StageSubStateWaitForChannelDisable:
      start_transaction/*channel*/(host, stageData);
      return;

    case StageSubStateWaitForTransactionComplete:
      // restart halted transaction
      if (REG32(HC_INT(channel)) == HC_INT_CH_HLTD)
      {
        puts("Transaction halted, restarting.");
        start_transaction(host, stageData);
        return;
      }

      assert(!TransferStageDataIsPeriodic(stageData) ||
        (HC_T_SIZ_PID(REG32(HC_T_SIZ(channel)))
         != HC_T_SIZ_PID_MDATA));

      TransferStageDataTransactionComplete (stageData,
        REG32(HC_INT(channel)),
        HC_T_SIZ_PKT_CNT(REG32(HC_T_SIZ(channel))),
        REG32(HC_T_SIZ(channel)) & HC_T_SIZ_XFER_SIZE_MASK);
      break;

    default:
      putbyte(stageData->subState);
      assert (0);
      break;
  }

  switch (stageData->state)
  {
    case StageStateNoSplitTransfer:
      status = stageData->transactionStatus;
      if (status & HC_INT_ERROR_MASK)
      {
        printf("No split Transaction failed (status 0x%X)\n", status);
        urb->status = status;
      }
      else if ((status & (HC_INT_NAK | HC_INT_NYET))
         && TransferStageDataIsPeriodic(stageData))
      {
        stageData->state = StageStatePeriodicDelay;

        interval = urb->endpoint->interval;
        TimerSchedule(interval * MICROS_PER_MILLISECOND, stage_interval,
                      stageData, host);
        break;
      }
      else
      {
        // Read data during data stage
        if (!stageData->statusStage)
          urb->resultLen = TransferStageDataGetResultLen(stageData);
        urb->status = 1;
      }

      disable_channel_interrupt(host, channel);

      TransferStageDataRelease(stageData);
      free_stage_data(stageData);
      host->stageData[channel] = 0;
      free_channel(host, channel);

      if (!(status & HC_INT_ERROR_MASK))
        RequestCallCompletionRoutine(urb);
      break;

    case StageStateStartSplit:
      status = stageData->transactionStatus;
      if ((status & HC_INT_ERROR_MASK) ||
          (status & HC_INT_NAK) ||
          (status & HC_INT_NYET))
      {
        puts("StartSplit Transaction failed");

        urb->status = 0;

        disable_channel_interrupt(host, channel);

        TransferStageDataRelease(stageData);
        free_stage_data(stageData);
        host->stageData[channel] = 0;

        free_channel(host, channel);

        RequestCallCompletionRoutine(urb);
        break;
      }

      frameScheduler->transactionComplete(frameScheduler, status);

      stageData->state = StageStateCompleteSplit;
      stageData->splitComplete = TRUE;

      if (!frameScheduler->completeSplit(frameScheduler))
      {
        goto LeaveCompleteSplit;
      }

      start_transaction(host, stageData);
      break;

    case StageStateCompleteSplit:
      status = stageData->transactionStatus;
      if (status & HC_INT_ERROR_MASK)
      {
        puts("Complete split Transaction failed");

        urb->status = 0;

        disable_channel_interrupt(host, channel);

        TransferStageDataRelease(stageData);
        free_stage_data(stageData);
        host->stageData[channel] = 0;

        free_channel(host, channel);

        RequestCallCompletionRoutine(urb);
        break;
      }

      frameScheduler->transactionComplete(frameScheduler, status);

      if (frameScheduler->completeSplit(frameScheduler))
      {
        start_transaction(host, stageData);
        break;
      }

    LeaveCompleteSplit:
      if (stageData->packets != 0)
      {
        if (!TransferStageDataIsPeriodic(stageData))
        {
          stageData->state = StageStateStartSplit;
          stageData->splitComplete = FALSE;

          frameScheduler->startSplit(frameScheduler);

          start_transaction(host, stageData);
        }
        else
        {
          stageData->state = StageStatePeriodicDelay;

          interval = urb->endpoint->interval;
          TimerSchedule(interval * MICROS_PER_MILLISECOND,
                        stage_interval, stageData, host);
        }
        break;
      }

      disable_channel_interrupt(host, channel);

      if (!stageData->statusStage)
        urb->resultLen = TransferStageDataGetResultLen(stageData);

      urb->status = 1;

      TransferStageDataRelease(stageData);
      free_stage_data(stageData);
      host->stageData[channel] = 0;

      free_channel(host, channel);

      RequestCallCompletionRoutine(urb);
      break;

    default:
      assert (0);
      break;
  }
}

/*...................................................................*/
/* process_interrupt: process the host controller interrupts         */
/*                                                                   */
/*      Input: param is the USB host                                 */
/*                                                                   */
/*    Returns: TASK_FINISHED if rescheduled or TASK_IDLE if a task   */
/*...................................................................*/
#if ENABLE_USB_TASK
static int process_interrupt(void *param)
#else
static int process_interrupt(u32 unused, void *param, void *context)
#endif
{
  Host *host = (Host *)param;
  unsigned channel = 0;
  u32 status;

  assert(host != 0);

  // Read current interrupt status
  status = REG32(INT_STS);

  // Process only host controller interrupts
  if (REG32(INT_STS) & INT_STS_HC_INTR)
  {
    u32 channelMask = 1;

    for (channel = 0; channel < host->channels; channel++)
    {
      if (REG32(ACH_INT) & channelMask)
      {
        REG32(HC_INT_MSK(channel)) = 0;
        process_channel_interrupt(host, channel);
      }

      channelMask <<= 1;
    }

  }

  // Acknowledge all previously processed interrupts
  REG32(INT_STS) = status;

#if !ENABLE_USB_TASK
  /* Schedule interrupt handler for next USB frame (125us). */
  TimerSchedule(125, process_interrupt, (void *)host, 0);
  return TASK_FINISHED;
#else
  return TASK_IDLE;
#endif
}

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* HostGetPortSpeed: process the host controller interrupts          */
/*                                                                   */
/*    Returns: The USB port speed, high, full, low or unknown        */
/*...................................................................*/
int HostGetPortSpeed(void)
{
  Speed result = SpeedUnknown;

  switch (PRT_SPD(REG32(HPRT)))
  {
    case DWC_HPRT0_PTRSPD_HIGH:
      result = SpeedHigh;
      break;

    case DWC_HPRT0_PTRSPD_FULL:
      result = SpeedFull;
      break;

    case DWC_HPRT0_PTRSPD_LOW:
      result = SpeedLow;
      break;

    default:
      break;
  }

  return result;
}

/*...................................................................*/
/* HostEnable: Enable the USB host controller and root port          */
/*                                                                   */
/*    Returns: TRUE on success, FALSE otherwise                      */
/*...................................................................*/
int HostEnable(void)
{
  u32 config;
  Host *host;

  bzero(&HostController, sizeof(Host));
  bzero(StageData, sizeof(TransferStageData) * MAX_USB_REQUESTS);

  /* Initialize the DesignWare Host Controller () device. */
  host_attach(&HostController);
  host = &HostController;
  assert(host != 0);

  puts("Turn on USB power...");
  if (SetUsbPowerStateOn())
  {
    puts("Set of USB HCD power bit failed");
    host_release(host);
    return FALSE;
  }

  /* Disable all interrupts by clearing the global interrupt mask. */
  config = REG32(AHB_CFG);
  config &= ~GLOBAL_INT_ENABLE;
  REG32(AHB_CFG) = config;

//  puts("Initialize the DesignWare Host Controller () Host.");
//  puts("  Initialize core.");
  if (!initialize_core(host))
  {
    puts("ERROR: Cannot initialize core.");
    host_release(host);
    return FALSE;
  }

//  puts("  Initialize interrupts.");
  enable_global_interrupt(host);

//  puts("  Initialize host.");
  if (!initialize_host(host))
  {
    puts("  ERROR: Cannot initialize host");
    host_release(&HostController);
    return FALSE;
  }

//  puts("  Create root port.");
  if (!enable_root_port(host))
  {
    puts("ERROR: No device connected to root port");
    host_release(&HostController);
    return TRUE;
  }

//  puts("  Begin root port enumeration.");
  if (!RootPortInitialize(host->rootPort))
  {
    puts("ERROR: Cannot initialize root port");
    host_release(&HostController);
    return TRUE;
  }

  // Create task or timer to monitor the interrupts
//  puts("  Begin interrupt handling.");
#if ENABLE_USB_TASK
  TaskNew(2, process_interrupt, (void *)host);
#else
  TimerSchedule(MICROS_PER_MILLISECOND, process_interrupt,
                (void *)host, 0);
#endif

  return TRUE;
}

/*...................................................................*/
/* HostDisable: disable the USB host controller                      */
/*                                                                   */
/*...................................................................*/
void HostDisable(void)
{
  Host *host = &HostController;
  u32 host_port;
  assert (host != 0);

  host_port = REG32(HPRT);
  host_port &= ~PRT_PWR;
  REG32(HPRT) = host_port;
}

/*...................................................................*/
/* HostGetEndpointDescriptor: retrieve an endpoint descriptor        */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*             endpoint is the device endpoint                       */
/*             type is the descriptor type                           */
/*             index is the descriptor index                         */
/*     Output: buffer to resulting descriptor                        */
/*             bufSize is the size of the resulting buffer           */
/*             requestType is the type of request (REQUEST_IN, etc.) */
/*             complete is callback function invoked upon completion */
/*             param is the pointer to pass to the complete callback */
/*                                                                   */
/*    Returns: One (1) on success, negative value if error           */
/*...................................................................*/
int HostGetEndpointDescriptor(void *host, void *endpoint, u8 type,
            u8 index, void *buffer, u32 bufSize, u8 requestType,
            void (complete)(void *urb, void *param, void *context),
            void *param)
{
  assert(host != 0);

  return HostEndpointControlMessage(host, endpoint,
          requestType, GET_DESCRIPTOR,
          (type << 8) | index, 0,
          buffer, bufSize, complete, param);
}

/*...................................................................*/
/* HostSetEndpointAddress: set the endpoint address to use           */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*             endpoint is the device endpoint                       */
/*             deviceAddress is the address to set                   */
/*             complete is callback function invoked upon completion */
/*             param is the pointer to pass to the complete callback */
/*                                                                   */
/*    Returns: TRUE on success, FALSE if error                       */
/*...................................................................*/
int HostSetEndpointAddress(void *host, void *endpoint,
      u8 deviceAddress, void (complete)(void *urb, void *param,
                                        void *context), void *param)
{
  assert(host != 0);

  if (HostEndpointControlMessage(host, endpoint, REQUEST_OUT,
             SET_ADDRESS, deviceAddress, 0, 0, 0, complete, param) < 0)
    return FALSE;

  //USB 2.0 set address max delay is defined as 150 ms
  usleep(150 * MICROS_PER_MILLISECOND);
  return TRUE;
}

/*...................................................................*/
/* HostSetEndpointConfiguration: set the endpoint configuration      */
/*                                                                   */
/*      Input: host is the USB host                                  */
/*             endpoint is the device endpoint                       */
/*             configurationValue is the configuration to set/use    */
/*             complete is callback function invoked upon completion */
/*             param is the pointer to pass to the complete callback */
/*                                                                   */
/*    Returns: TRUE on success, FALSE if error                       */
/*...................................................................*/
int HostSetEndpointConfiguration(void *host, void *endpoint,
         u8 configurationValue, void (complete)(void *urb, void *param,
                                          void *context), void *param)
{
  assert(host != 0);

  if (HostEndpointControlMessage(host, endpoint, REQUEST_OUT,
      SET_CONFIGURATION, configurationValue, 0, 0, 0, complete,
      param) < 0)
    return FALSE;

  //USB 2.0 max set delay is defined as 150 ms
  //  Alternatively use a complete callback and wait for it
  usleep(150 * MICROS_PER_MILLISECOND);

  return TRUE;
}

/*...................................................................*/
/* HostEndpointControlMessage: send endpoint control message         */
/*                                                                   */
/*      Input: vhost is the USB host                                 */
/*             endpoint is the device endpoint                       */
/*             requestType is the type of request (REQUEST_IN, etc.) */
/*             request is the request identifier (SET_ADDRESS, etc.) */
/*             value is the value of the request                     */
/*             index is the index of the request                     */
/*             data is the optional data of the request              */
/*             dataSize is the size of the optional data             */
/*             complete is the callback to invoke upon completion    */
/*             param is the pointer to pass to the complete callback */
/*                                                                   */
/*    Returns: One (1) on success, negative if error                 */
/*...................................................................*/
int HostEndpointControlMessage(void *vhost, void *endpoint,
      u8 requestType, u8 request, u16 value, u16 index, void *data,
      u16 dataSize, void (complete)(void *urb, void *param,
                                    void *context), void *param)
{
  Host *host = vhost;
  Request *urb = NewRequest();
  SetupData *setup = &urb->Data;
  assert(host != 0);
  assert(setup != 0);
  if ((urb == NULL) || (setup == NULL))
  {
    puts("HostEndpointControlMessage - URB or Setup invalid");
    return -1;
  }

  setup->requestType  = requestType;
  setup->request      = request;
  setup->value        = value;
  setup->index        = index;
  setup->length       = dataSize;
  RequestAttach(urb, endpoint, data, dataSize, setup);


  if (complete)
    RequestSetCompletionRoutine(urb, complete, param, host);
  else
    RequestSetCompletionRoutine(urb, transfer_complete, param, host);
  HostSubmitAsyncRequest(urb, host, NULL);
  return 1;
}

/*...................................................................*/
/* HostEndpointTransfer: transfer URB with an endpoint               */
/*                                                                   */
/*      Input: vhost is the USB host                                 */
/*             endpoint is the device endpoint                       */
/*             buffer is the data to transfer in the URB             */
/*             bufSize is the size of the URB data                   */
/*             complete is the callback to invoke upon completion    */
/*                                                                   */
/*    Returns: One (1) on success, negative if error                 */
/*...................................................................*/
int HostEndpointTransfer(void *vhost, void *endpoint, void *buffer,
                   u32 bufSize, void (complete)(void *urb, void *param,
                                                void *context))
{
  Host *host = vhost;
  Request *urb;

  assert(host != 0);

  urb = NewRequest();
  if (urb == NULL)
  {
    puts("USBNewReqest failed!");
    return -1;
  }
  RequestAttach(urb, endpoint, buffer, bufSize, 0);

  if (complete)
    RequestSetCompletionRoutine(urb, complete, 0, host);
  else
    RequestSetCompletionRoutine(urb, transfer_complete, 0, host);

  HostSubmitAsyncRequest(urb, host, (void *)0);
  return 0;
}

/*...................................................................*/
/* HostSubmitAsyncRequest: transfer URB with an endpoint             */
/*                                                                   */
/*      Input: request is the USB Request Buffer (URB)               */
/*             dev is the USB host                                   */
/*             context is the state, or stage, of request transfer   */
/*...................................................................*/
void HostSubmitAsyncRequest(void *request, void *dev, void *context)
{
  Host *host = dev;
  Request *urb = request;
  int state = (u32)context;
  assert(host != 0);

  assert(urb != 0);
  assert(urb->endpoint->type == EndpointTypeBulk
    || urb->endpoint->type == EndpointTypeInterrupt
    || urb->endpoint->type == EndpointTypeControl);
  assert(urb->bufLen >= 0);

  urb->status = 0;

  if (urb->endpoint->type == EndpointTypeControl)
  {
    SetupData *setup = urb->setupData;

    assert(setup != 0);

    if (setup->requestType & REQUEST_IN)
    {
      assert(urb->bufLen > 0);

      if (state == 0)
      {
        /* save the previous registered completion callback */
        urb->savedRoutine = urb->completionRoutine;
        urb->savedParam   = urb->completionParam;
        urb->savedContext = urb->completionContext;

        RequestSetCompletionRoutine(urb,
                       HostSubmitAsyncRequest, host, (void *)1);

        transfer_stage_async(host, urb, FALSE, FALSE);
      }
      else if (state == 1)
      {
        RequestSetCompletionRoutine(urb,
                       HostSubmitAsyncRequest, host, (void *)2);
        transfer_stage_async(host, urb, TRUE, FALSE);
      }
      else if (state == 2)
      {
        RequestSetCompletionRoutine(urb,
          urb->savedRoutine, urb->savedParam,
          urb->savedContext);
        transfer_stage_async(host, urb, FALSE, TRUE);
      }
    }
    else
    {
      if (urb->zlb || ((state == 0) &&
                        (urb->bufLen == 0)))
      {
        if (state == 0)
        {
          /* save the previous registered completion callback */
          urb->savedRoutine = urb->completionRoutine;
          urb->savedParam   = urb->completionParam;
          urb->savedContext = urb->completionContext;
          urb->zlb = 1;

          RequestSetCompletionRoutine(urb,
                      HostSubmitAsyncRequest, host, (void *)1);

          transfer_stage_async(host, urb, FALSE, FALSE);
        }
        else if (state == 1)
        {
          RequestSetCompletionRoutine(urb,
            urb->savedRoutine, urb->savedParam,
            urb->savedContext);
          transfer_stage_async(host, urb, TRUE, TRUE);
        }
      }
      else
      {
        if (state == 0)
        {
          /* save the previous registered completion callback */
          urb->savedRoutine = urb->completionRoutine;
          urb->savedParam   = urb->completionParam;
          urb->savedContext = urb->completionContext;
          urb->zlb = 0;

          RequestSetCompletionRoutine(urb,
                      HostSubmitAsyncRequest, host, (void *)1);

          transfer_stage_async(host, urb, FALSE, FALSE);
        }
        else if (state == 1)
        {
          RequestSetCompletionRoutine(urb,
                       HostSubmitAsyncRequest, host, (void *)2);
          transfer_stage_async(host, urb, FALSE, FALSE);
        }
        else if (state == 2)
        {
          RequestSetCompletionRoutine(urb,
            urb->savedRoutine, urb->savedParam,
            urb->savedContext);
          transfer_stage_async(host, urb, TRUE, TRUE);
        }
      }
    }
  }
  else
  {
    if (!transfer_stage_async(host, urb, urb->endpoint->directionIn,
                              FALSE))
    {
      puts("HostSubmitAsyncRequest TransferStageAsync failed");
      return;
    }
  }
  return;
}

#endif /* ENABLE_USB */

