/*...................................................................*/
/*                                                                   */
/*   Module:  timers.c                                               */
/*   Version: 2019.0                                                 */
/*   Purpose: Timer interface                                        */
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
#include <board.h>
#include <stdio.h>
#include <string.h>

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
#define MAX_TIMER_TASKS 32

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
struct timer_task *TimerStart;
struct timer_task TimerTasks[MAX_TIMER_TASKS];

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* TimerInit: Initialize the system timers                           */
/*                                                                   */
/*...................................................................*/
void TimerInit(void)
{
  TimerStart = NULL;
  bzero(&TimerTasks, sizeof(struct timer_task) * MAX_TIMER_TASKS);
}

/*...................................................................*/
/* TimerCancel: Cancel a scheduled timer                             */
/*                                                                   */
/*     Inputs: cancel is pointer to the timer to cancel              */
/*                                                                   */
/*    Returns: The created timer                                     */
/*...................................................................*/
void TimerCancel(struct timer_task *cancel)
{
  // Return on invalid parameter
  if (cancel == NULL)
    return;

  // Special handling to maintain top of the list
  if (cancel == TimerStart)
    TimerStart = (void *)cancel->list.next;

  // Remove the timer item from the list
  ListRemove(cancel->list);

  /* initialize the completed timer task for reuse */
  bzero(cancel, sizeof(struct timer_task));
}

/*...................................................................*/
/* TimerSchedule: Check if a registered timer has expired            */
/*                                                                   */
/*     Inputs: usec - expiration time in microseconds                */
/*             poll - function to call after expiration              */
/*             data - pointer to data structure passed to function   */
/*             context - pointer to context structure of NULL        */
/*                                                                   */
/*    Returns: Pointer to the created timer task                     */
/*...................................................................*/
struct timer_task *TimerSchedule(u32 usec,
                       int (*poll) (u32 id, void *data, void *context),
                       void *data, void *context)
{
  int i;
  struct timer tw;
  u64 now;
  struct timer_task *new_timer, *tt;

  /* Retrieve the current time from the hardware clock. */
  now = TimerNow();

  /* Assign the timer timeout value. */
  tw.expire = now + usec;
  tw.last = now;

  /* Find a free timer task. */
  for (i = 0; i < MAX_TIMER_TASKS; ++i)
  {
    if (TimerTasks[i].poll == NULL)
      break;
  }

  /* If no timer task available, return NULL failure. */
  if (i >= MAX_TIMER_TASKS)
    return NULL;

  /* Initialize the new timer task. */
  new_timer = &TimerTasks[i];
  new_timer->expire = tw;
  new_timer->poll = poll;
  new_timer->data = data;
  new_timer->context = context;
  new_timer->list.next = NULL;
  new_timer->list.previous = NULL;

  // If task list is empty start it with this node
  if (TimerStart == NULL)
  {
    TimerStart = new_timer;
    return new_timer;
  }

  // Search list of tasks in priority order
  for (tt = (void *)TimerStart; tt;
       tt = (void *)tt->list.next)
  {
    /* If priority is less for this entry, insert before here. */
    if (new_timer->expire.expire < tt->expire.expire)
    {
      if (tt == TimerStart)
        TimerStart = new_timer;
      ListInsertBefore(new_timer, tt);
      break;
    }

    // Otherwise if this is the last node insert after here
    else if (tt->list.next == NULL)
    {
      ListInsertAfter(new_timer, tt);
      break;
    }
  }

  /* Return the expiration time. */
  return new_timer;
}

/*...................................................................*/
/* TimerServiceCancel: Cancel perviously scheduled timer by callback */
/*                                                                   */
/*     Inputs: poll - function to call after expiration              */
/*             data - pointer to data structure passed to function   */
/*                                                                   */
/*    Returns: Zero (0) on success, failure otherwise                */
/*...................................................................*/
int TimerServiceCancel(void *poll, void *data)
{
  struct timer_task *current;

  // Search for the registered timer and cancel it with success or fail
  for (current = TimerStart; current;
       current = (void *)current->list.next)
  {
    // If found cancel the timer.
    if ((current->poll == poll) && (current->data == data))
    {
      TimerCancel(current);
      return 0;
    }
  }
  return -1;
}

/*...................................................................*/
/*  TimerPoll: Poll scheduled timers and execute any expired         */
/*                                                                   */
/*     Inputs: data - pointer to data structure of timer state       */
/*                                                                   */
/*    Returns: TASK_IDLE                                             */
/*...................................................................*/
int TimerPoll(void *data)
{
  struct timer_task **start = data; // Pointer to a pointer
  struct timer_task *current;
  int status;

  // Loop through all the timers
  for (current = *start; current; current = (void *)current->list.next)
  {
    // Check if the timer expired
    if (TimerRemaining(&current->expire) == 0)
    {
      // Execute the timer callback function
      status = current->poll((u32)TimerPoll, current->data,
                             current->context);

      //Cancel the timer if poll returned finished
      if (status == TASK_FINISHED)
         TimerCancel(current);

      /* After doing work return idle */
      return TASK_IDLE;
    }

    /* otherwise no more expired timers in ordered list so break out */
    else
      break;
  }
  return TASK_IDLE;
}

/*...................................................................*/
/*     usleep: Wait or sleep for an amount of microseconds           */
/*                                                                   */
/*      Input: microseconds to sleep                                 */
/*...................................................................*/
void usleep(u64 microseconds)
{
  struct timer tw;

  /* Create a timer that expires 'microseconds' from now. */
  tw = TimerRegister(microseconds);

  /* Loop checking the timer until it expires. */
  for (;TimerRemaining(&tw) > 0;)
    ; // Do nothing
}

