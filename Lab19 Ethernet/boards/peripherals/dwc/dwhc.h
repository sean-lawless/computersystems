/*...................................................................*/
/*                                                                   */
/*   Module:  dwhc.h                                                 */
/*   Version: 2020.0                                                 */
/*   Purpose: DesignWare Host Channel register map                   */
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
#ifndef _DWHC_H
#define _DWHC_H

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
//  Translated from FreeBSD dwc_otgreg.h
//    BSD-2-Clause-FreeBSD, Copyright 2010,2011 Aleksander Rybalko
#define USB_BASE                 (PERIPHERAL_BASE | 0x980000)

// Host Channels 0 through 15
#define HC_CHAR(chan)            (USB_BASE + 0x500 + (chan)*0x20)
#define   HC_CHAR_MPS_MASK         0x7FF // Maximium Packet Size (mask)
#define   HC_CHAR_EP_NUM_OFFSET    11
#define   HC_CHAR_EP_NUM_MASK      (0xF << 11)
#define   HC_CHAR_EP_DIR_IN        (1 << 15)
#define   HC_CHAR_LSPD_DEV         (1 << 17) // low speed device
#define   HC_CHAR_EP_TYPE_OFFSET   18
#define   HC_CHAR_EP_TYPE_MASK     (3 << 18)
#define     HC_CHAR_EP_TYPE_CONTROL 0
#define     HC_CHAR_EP_TYPE_ISO    1
#define     HC_CHAR_EP_TYPE_BULK   2
#define     HC_CHAR_EP_TYPE_INT    3
#define   HC_CHAR_MULTI_CNT_OFFSET 20
#define   HC_CHAR_MULTI_CNT_MASK   (3 << 20)
#define   HC_CHAR_DEV_ADDR_OFFSET  22 // device address
#define   HC_CHAR_DEV_ADDR_MASK    (0x7F << 22)
#define   HC_CHAR_ODD_FRM          (1 << 29) // odd frame
#define   HC_CHAR_DIS              (1 << 30) // disable
#define   HC_CHAR_EN               (1 << 31) // enable
#define HC_SPLT(chan)            (USB_BASE + 0x504 + (chan) * 0x20)
  #define HC_SPLT_PRT_ADDR_MASK    0x7F
  #define HC_SPLT_HUB_ADDR_OFFSET  7
  #define HC_SPLT_HUB_ADDR_MASK    (0x7F << 7)
  #define HC_SPLT_XACT_POS_OFFSET  14
  #define HC_SPLT_XACT_POS_MASK    (3 << 14)
    #define HC_SPLT_XACT_POS_ALL   3
  #define HC_SPLT_COMP_SPLT        (1 << 16) // complete split
  #define HC_SPLT_SPLT_ENA         (1 << 31) // split enable
#define HC_INT(chan)             (USB_BASE + 0x508 + (chan) * 0x20)
  #define HC_INT_XFER_COMP         (1 << 0) // transfer complete
  #define HC_INT_CH_HLTD           (1 << 1) // channel halted
  #define HC_INT_AHB_ERR           (1 << 2) // AHB error
  #define HC_INT_STALL             (1 << 3)
  #define HC_INT_NAK               (1 << 4)
  #define HC_INT_ACK               (1 << 5)
  #define HC_INT_NYET              (1 << 6)
  #define HC_INT_XACT_ERR          (1 << 7)
  #define HC_INT_BBL_ERR           (1 << 8) // Babble error
  #define HC_INT_FRM_OVRUN         (1 << 9) // frame overrun
  #define HC_INT_DATA_TGL_ERR      (1 << 10)// data toggle error
  #define HC_INT_ERROR_MASK        (HC_INT_AHB_ERR | HC_INT_STALL | \
              HC_INT_XACT_ERR | HC_INT_BBL_ERR | HC_INT_FRM_OVRUN | \
                                                 HC_INT_DATA_TGL_ERR)
#define HC_INT_MSK(chan)         (USB_BASE + 0x50C + (chan) * 0x20)
#define HC_T_SIZ(chan)           (USB_BASE + 0x510 + (chan) * 0x20)
#define HC_T_SIZ_XFER_SIZE_MASK    0x7FFFF
#define HC_T_SIZ_PKT_CNT_OFFSET    19
#define HC_T_SIZ_PKT_CNT_MASK      (0x3FF << 19)
#define HC_T_SIZ_PKT_CNT(reg)      (((reg) >> 19) & 0x3FF)
#define HC_T_SIZ_PID_OFFSET        29
#define HC_T_SIZ_PID_MASK          (3 << 29)
#define HC_T_SIZ_PID(reg)          (((reg) >> 29) & 3)
#define HC_T_SIZ_PID_DATA0           0
#define HC_T_SIZ_PID_DATA1           2
#define HC_T_SIZ_PID_DATA2           1
#define HC_T_SIZ_PID_MDATA           3 // non-control transfer
#define HC_T_SIZ_PID_SETUP           3
#define HC_DMA_ADDR(chan)        (USB_BASE + 0x514 + (chan)*0x20)

#endif /* _DWHC_H */
