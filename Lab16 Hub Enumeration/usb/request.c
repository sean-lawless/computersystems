/*...................................................................*/
/*                                                                   */
/*   Module:  request.c                                              */
/*   Version: 2020.0                                                 */
/*   Purpose: USB Request attachment and completion callback         */
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
/*                                                                   */
/*...................................................................*/
#include <usb/request.h>
#include <assert.h>
#include <string.h>

#if ENABLE_USB

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
Request Requests[MAX_USB_REQUESTS];

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/*  NewRequest: Find and reserve an unused USB Request Buffer (URB)  */
/*                                                                   */
/*     Returns: pointer to resulting USB request or NULL             */
/*...................................................................*/
Request *NewRequest(void)
{
  int i;

  for (i = 0; i < MAX_USB_REQUESTS; ++i)
  {
    if ((Requests[i].endpoint == NULL) &&
        (Requests[i].buffer == NULL))
    {
      // Set endpoint to invalid; temporary to claim this request
      Requests[i].endpoint = (void *)-1;
      return &Requests[i];
    }
  }
  return NULL;
}

/*...................................................................*/
/* FreeRequest: Free up a USB Request Buffer (URB)                   */
/*                                                                   */
/*      Input: pointer to USB request to free up                     */
/*...................................................................*/
void FreeRequest(Request *request)
{
  request->endpoint = NULL;
  request->buffer = NULL;
}

/*...................................................................*/
/* RequestInit: Initialize the pool of USB requests                  */
/*                                                                   */
/*...................................................................*/
void RequestInit(void)
{
  int i;

  for (i = 0; i < MAX_USB_REQUESTS; ++i)
    bzero(&Requests[i], sizeof(Request));
}

/*...................................................................*/
/* RequestAttach: Attach a request to a device endpoint              */
/*                                                                   */
/*       Input: request is the USB Request Buffer (URB)              */
/*              endpoint is the endpoint to attach to URB            */
/*              buffer is the buffer to attach to URB                */
/*              bufLen is the length of the buffer                   */
/*              setupData is the USB setup data for this new request */
/*...................................................................*/
void RequestAttach(Request *request, Endpoint *endpoint,
                   void *buffer, u32 bufLen, SetupData *setupData)
{
  assert (request != 0);

  request->endpoint = endpoint;
  request->setupData = setupData;
  request->buffer = buffer;
  request->bufLen = bufLen;
  request->status = 0;
  request->resultLen = 0;
  request->completionRoutine = 0;
  request->completionParam = 0;
  request->completionContext = 0;
  request->zlb = 0;
  request->savedRoutine = 0;
  request->savedParam = 0;
  request->savedContext = 0;

  assert (request->endpoint != 0);
  assert ((request->buffer != 0) || (request->bufLen == 0));
}

/*...................................................................*/
/* RequestRelease: Release a USB Request Buffer (URB)                */
/*                                                                   */
/*       Input: request is the USB Request Buffer (URB)              */
/*...................................................................*/
void RequestRelease(Request *request)
{
  assert (request != 0);
  request->endpoint = 0;
  request->setupData = 0;
  request->buffer = 0;
  request->completionRoutine = 0;
}

/*...................................................................*/
/* RequestSetCompletionRoutine: Routine to invoke upon completion    */
/*                                                                   */
/*       Input: request is the USB Request Buffer (URB)              */
/*              routine is the completion callback function          */
/*              param is a pointer to pass to completion routine     */
/*              context is pointer to pass to completion routine     */
/*...................................................................*/
void RequestSetCompletionRoutine(Request *request,
              URBCompletion *routine, void *param, void *context)
{
  assert(request != 0);
  request->completionRoutine = routine;
  request->completionParam   = param;
  request->completionContext = context;

  assert (request->completionRoutine != 0);
}

/*...................................................................*/
/* RequestCallCompletionRoutine: Invoke completion routine for URB   */
/*                                                                   */
/*       Input: request is the USB Request Buffer (URB)              */
/*...................................................................*/
void RequestCallCompletionRoutine(Request *request)
{
  assert(request != 0);
  assert(request->completionRoutine != 0);

  (*request->completionRoutine)(request, request->completionParam,
                                request->completionContext);
}

#endif /* ENABLE_USB */
