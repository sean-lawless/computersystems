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
//#include <usb/request.h>
#include <usb/device.h>
#include "dwhc.h"
//#include "transfer.h"

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
#define   INT_STS_USB_SUSPEND    (1 << 11) // suspend interrupt
#define   INT_STS_USB_RESET      (1 << 12) // reset interrupt
#define   INT_STS_PORT_INTR      (1 << 24) // port interrupt
#define   INT_STS_HC_INTR        (1 << 25) // host interrupt
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

/*...................................................................*/
/* External Function Prototypes                                      */
/*...................................................................*/

/*...................................................................*/
/* Static Local Functions                                            */
/*...................................................................*/

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
  host->rootPort = 0;//RootPortAttach(host);

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
//  RootPortRelease(host->rootPort);
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
  mask |= INT_MSK_HC_INTR | INT_MSK_PORT_INTR | INT_MSK_DISCONNECT |
          INT_MSK_SOF_INTR | INT_MSK_USB_RESET;

  REG32(INT_MSK) = mask;
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
  u32 host, config, core_config;
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
/* process_interrupt: process the host controller interrupts         */
/*                                                                   */
/*      Input: param is the USB host                                 */
/*                                                                   */
/*    Returns: TASK_FINISHED if rescheduled or TASK_IDLE if a task   */
/*...................................................................*/
static int process_interrupt(u32 unused, void *param, void *context)
{
  Host *host = (Host *)param;
  unsigned channel = 0;
  u32 status;
  static u32 oldStatus;

  assert(host != 0);

  // Read current interrupt status
  status = REG32(INT_STS);

  // Process only host controller interrupts
  if (status & INT_STS_HC_INTR)
  {
    u32 channelMask = 1;

    printf("USB host interrupt %x\n", status);

    for (channel = 0; channel < host->channels; channel++)
    {
      if (REG32(ACH_INT) & channelMask)
      {
        REG32(HC_INT_MSK(channel)) = 0;
        printf("Channel interrupt %d\n", channel);
// todo:      process_channel_interrupt(host, channel);
      }

      channelMask <<= 1;
    }
  }
  if ((status & INT_STS_USB_RESET) && (oldStatus != status))
  {
    printf("USB reset interrupt %x\n", status);
  }
  if ((status & INT_STS_USB_SUSPEND) && (oldStatus != status))
  {
    printf("USB suspend interrupt %x\n", status);
  }
  if ((status & INT_STS_SOF_INTR) && (oldStatus != status))
  {
    printf("USB soft interrupt %x\n", status);
  }
  if ((status & INT_STS_PORT_INTR) && (oldStatus != status))
  {
    printf("USB port interrupt %x\n", status);
  }

  // Acknowledge all previously processed interrupts
  REG32(INT_STS) = status;
  oldStatus = status;

  /* Schedule interrupt handler for next USB frame (125us). */
  if (!unused)
    TimerSchedule(125, process_interrupt, (void *)host, 0);
  return TASK_FINISHED;
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
  Host *host;

  bzero(&HostController, sizeof(Host));

  /* Initialize the DesignWare Host Controller () device. */
  host_attach(&HostController);
  host = &HostController;
  assert(host != 0);

  puts("Initialize the DesignWare Host Controller () Host.");
  puts("  Turn on USB power...");
  if (SetUsbPowerStateOn())
  {
    puts("Set of USB HCD power bit failed");
    host_release(host);
    return FALSE;
  }

  puts("  Enable global interrupts.");
  enable_global_interrupt(host);

  process_interrupt(TRUE, host, 0);

  puts("  Initialize core.");
  if (!initialize_core(host))
  {
    puts("ERROR: Cannot initialize core.");
    host_release(host);
    return FALSE;
  }

  process_interrupt(TRUE, host, 0);

  puts("  Initialize host.");
  if (!initialize_host(host))
  {
    puts("  ERROR: Cannot initialize host");
    host_release(&HostController);
    return FALSE;
  }

  // Create task or timer to monitor the interrupts
  puts("  Schedule interrupt handler.");
  TimerSchedule(MICROS_PER_MILLISECOND, process_interrupt,
                (void *)host, 0);

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

#endif /* ENABLE_USB */

