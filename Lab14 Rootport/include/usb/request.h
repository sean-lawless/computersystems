/*...................................................................*/
/*                                                                   */
/*   Module:  request.h                                              */
/*   Version: 2020.0                                                 */
/*   Purpose: USB host request definitions                           */
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
#ifndef _REQUEST_H
#define _REQUEST_H

#include <usb/usb.h>
#include <usb/endpoint.h>
#include <system.h>

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define MAX_USB_REQUESTS 10

/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
typedef void URBCompletion(void *urb, void *param, void *context);

// Usb Request Buffer (URB)
typedef struct Request
{
  Endpoint *endpoint;

  SetupData *setupData;
  SetupData Data;
  void *buffer;
  u32 bufLen;

  int status;
  u32 resultLen;
  int zlb;

  URBCompletion *savedRoutine;
  void *savedParam;
  void *savedContext;

  URBCompletion *completionRoutine;
  void *completionParam;
  void *completionContext;
}
Request;

/*...................................................................*/
/* Function Prototypes                                               */
/*...................................................................*/
// Initialization
void RequestInit(void);

// URB allocation routines
Request *NewRequest(void);
void FreeRequest(Request *req);

// URB assignment routines
void RequestAttach(Request *request, Endpoint *endpoint, void *buffer,
                   u32 bufLen, SetupData *setupData);
void RequestRelease(Request *request);

// URB action routines
void RequestSetCompletionRoutine(Request *request,
                   URBCompletion *complete, void *param, void *context);
void RequestCallCompletionRoutine(Request *request);

#endif /* _REQUEST_H */
