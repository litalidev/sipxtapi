// 
// Copyright (C) 2005-2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
// 
// Copyright (C) 2004-2007 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// $$
//////////////////////////////////////////////////////////////////////////////


// SYSTEM INCLUDES
#include <assert.h>

#include "os/OsNameDbInit.h" 
// APPLICATION INCLUDES
#include "os/OsEvent.h"
#include "os/OsTimer.h"
#include "os/OsTimerTaskCommandMsg.h"
#include "os/OsTimerTask.h"
#include "os/OsLock.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES

// CONSTANTS

// STATIC VARIABLE INITIALIZATIONS
// spInstance can be tested and set asynchronously by different threads.
OsTimerTask OsTimerTask::sInstance;

const int OsTimerTask::TIMER_MAX_REQUEST_MSGS = 20000;

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Return a pointer to the timer task, creating it if necessary.
OsTimerTask* OsTimerTask::getTimerTask(void)
{
   // If the task object already exists, and the corresponding low-level task
   // has been started, then use it

   if (!sInstance.isStarted())
   {
      UtlBoolean isStarted = (&sInstance)->start();
   }

   return &sInstance;
}

// Destroy the singleton instance of the timer task.
void OsTimerTask::destroyTimerTask(void)
{
    OsSysLog::add(FAC_KERNEL, PRI_DEBUG,
                  "OsTimerTask::destroyTimerTask entered");

    if (sInstance.isStarted())
    {
       OsEvent event;
       OsTimerTaskCommandMsg msg(OsTimerTaskCommandMsg::OS_TIMER_SHUTDOWN, NULL, &event);
       // Send the OS_TIMER_SHUTDOWN message.
       OsStatus res = OsTimerTask::getTimerTask()->postMessage(msg);
       assert(res == OS_SUCCESS);
       // Wait for the response.
       event.wait();
    }
}

// Destructor
OsTimerTask::~OsTimerTask()
{
   if (sInstance.isStarted())
   {
      // Shut down the task.
      OsEvent event;
      OsTimerTaskCommandMsg msg(OsTimerTaskCommandMsg::OS_TIMER_SHUTDOWN, NULL, &event);
      // Send the OS_TIMER_SHUTDOWN message.
      OsStatus res = OsTimerTask::getTimerTask()->postMessage(msg);
      assert(res == OS_SUCCESS);
      // Wait for the response.
      event.wait();
   }
}

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

// Default constructor (called only indirectly via getTimerTask())
OsTimerTask::OsTimerTask(void)
:  OsServerTask("OsTimer-%d", NULL, TIMER_MAX_REQUEST_MSGS
                , 5 // high priority so that we get reasonable clock heartbeats for media
               ),
   mTimerQueue(0)
{
}

// The entry point for the task.
// This method executes a message processing loop until either
// requestShutdown(), deleteForce(), or the destructor for this object
// is called.
int OsTimerTask::run(void* pArg)
{
   UtlBoolean doShutdown;
   OsMsg*    pMsg = NULL;
   OsStatus     res;

   // "now" is the current time.
   OsTimer::Time now = OsTimer::now();
   doShutdown = FALSE;
   do
   {
      // Do not attempt to receive message if a timer has already fired.
      // (This also avoids an edge case if the timeout value is zero
      // or negative.)
      if (!(mTimerQueue &&
            OsTimer::compareTimes(now, mTimerQueue->mQueuedExpiresAt) >= 0))
      {
         // Set the timeout till the next timer fires.
         OsTime timeout;
         if (mTimerQueue)
         {
            OsTimer::cvtToOsTime(timeout,
                                 OsTimer::subtractTimes(mTimerQueue->mQueuedExpiresAt,
                                                        now));
         }
         else
         {
            timeout = OsTime::OS_INFINITY;
         }

         res = receiveMessage((OsMsg*&) pMsg, timeout); // wait for a message
         if (res == OS_SUCCESS)
         {
            // A message was received.  Process it.
            doShutdown = isShuttingDown();
            if (!doShutdown)
            {                                           // comply with shutdown
               if (!handleMessage(*pMsg))               // process the message
               {
                  OsServerTask::handleMessage(*pMsg);
               }
            }

            if (!pMsg->getSentFromISR())
            {
               pMsg->releaseMsg();                      // free the message
            }
         }
         else
         {
            assert(pMsg==NULL);
         }

         now = OsTimer::now();     // Update our record of the current time.
      }

      // Now check for timers that have expired.
      while (mTimerQueue &&
             OsTimer::compareTimes(now, mTimerQueue->mQueuedExpiresAt) >= 0)
      {
         // Fire the the timer (and remove it from the queue).
         OsTimer* timer = mTimerQueue;
         mTimerQueue = timer->mTimerQueueLink;
         // Clear the timer's mTimerQueueLink to indicate it is not
         // in the timer queue.
         timer->mTimerQueueLink = 0;
         fireTimer(timer);
      }

      if (OsSysLog::willLog(FAC_KERNEL, PRI_WARNING))
      {
         // Check to see if timer firing took too long.
         OsTimer::Time after = OsTimer::now();
         OsTimer::Time t = OsTimer::subtractTimes(after, now);
         if (t >= 1000000 /* 1 second */)
         {
            OsSysLog::add(FAC_KERNEL, PRI_WARNING,
                          "OsTimerTask::run firing took %"PRId64" usecs,"
                          " queue length = %d, before fire = %"PRId64", after fire = %"PRId64,
                          t, getMessageQueue()->numMsgs(), now, after);
         }
      }
   }
   while (!doShutdown);

   OsSysLog::add(FAC_KERNEL, PRI_INFO,
                 "OsTimerTask::run OsTimerTask shutting down");

   return 0;        // and then exit
}

// Handle a timer service request.
// Return TRUE if the request was handled, otherwise FALSE.
UtlBoolean OsTimerTask::handleMessage(OsMsg& rMsg)
{
   // Process a message.

   // If not an OS_TIMERTASK_COMMAND message, return FALSE to indicate it should be
   // passed to our superclass.
   if (rMsg.getMsgType() != OsMsg::OS_TIMERTASK_COMMAND)
   {
      return FALSE;
   }

   // Process an OS_TIMERTASK_COMMAND message.

   OsTimerTaskCommandMsg& message = dynamic_cast <OsTimerTaskCommandMsg&> (rMsg);

   // Process a OS_TIMER_SHUTDOWN message, which is special
   if (message.getMsgSubType() == OsTimerTaskCommandMsg::OS_TIMER_SHUTDOWN)
   {
      OsSysLog::add(FAC_KERNEL, PRI_INFO,
                    "OsTimerTask::handleMessage OS_TIMER_SHUTDOWN seen, mState = %d",
                    mState);
      // Verify that there are no other requests in the timer task's queue.
      assert(getMessageQueue()->isEmpty());

      // Stop all the timers in the timer queue.
      OsTimer* link;
      for (OsTimer* timer = mTimerQueue; timer; timer = link)
      {
         // This lock should never block, since the application should not
         // be accessing the timer.
         OsLock lock(timer->mBSem);

         // Check that the application and task states are the same.
         // If they aren't, the application is mucking with the timer.
         assert(timer->mTaskState == timer->mApplicationState);
         // Increment the state fields, to show the timer is stopped.
         timer->mTaskState =
            timer->mApplicationState = timer->mApplicationState + 1;

         // Get the link field.
         link = timer->mTimerQueueLink;

         // Clear the link field of the timer.
         timer->mTimerQueueLink = 0;
      }
      // Empty the timer queue.
      mTimerQueue = 0;

      // Change mState so the main loop will exit.
      requestShutdown();

      // Signal the event so our caller knows we're done.
      message.getEventP()->signal(0);
      OsSysLog::add(FAC_KERNEL, PRI_INFO,
                    "OsTimerTask::handleMessage OS_TIMER_SHUTDOWN seen, mState = %d",
                    mState);
      return TRUE;
   }

   OsTimer* timer = message.getTimerP();
#ifndef NDEBUG
   CHECK_VALIDITY(timer);
#endif
   unsigned int applicationState;
   OsTimer::Time expiresAt;
   UtlBoolean periodic;
   OsTimer::Interval period;
   {
      OsLock lock(timer->mBSem);

      // mDeleting may be true, if the destructor has started running.

      // Decrement the outstanding message count.
      timer->mOutstandingMessages--;

      // Get mApplicationState.
      applicationState = timer->mApplicationState;

      // Get the timing information.
      expiresAt = timer->mExpiresAt;
      periodic = timer->mPeriodic;
      period = timer->mPeriod;
   }	    

   // Determine whether the timer needs to be stopped.
   // (The comparison between applicationState and mTaskState is really
   // ">", taking into account wraparound.  But that is difficult to
   // implement, so given that mApplicationState is always >= mTaskState
   // by design, we can use "!=".)
   if (applicationState != timer->mTaskState &&
       OsTimer::isStarted(timer->mTaskState))
   {
      // Stop the timer.
      removeTimer(timer);
   }
   // Determine whether the timer needs to be started.
   if (applicationState != timer->mTaskState &&
       OsTimer::isStarted(applicationState))
   {
      // Start the timer.
      // Set the saved timing information.
      timer->mQueuedExpiresAt = expiresAt;
      timer->mQueuedPeriodic = periodic;
      timer->mQueuedPeriod = period;
      insertTimer(timer);
   }
	    
   // Update the task state.
   timer->mTaskState = applicationState;

   switch (message.getMsgSubType())
   {
   case OsTimerTaskCommandMsg::OS_TIMER_UPDATE:
      // No further processing is needed.
      break;

   case OsTimerTaskCommandMsg::OS_TIMER_UPDATE_SYNC:
      // If it is an UPDATE_SYNC message, signal the event.
      message.getEventP()->signal(0);
      break;

   case OsTimerTaskCommandMsg::OS_TIMER_UPDATE_DELETE:
      // If it is an UPDATE_DELETE, delete the timer.

      // Timer will not be accessed by any other thread, so we
      // can access it without locking.
#ifndef NDEBUG
      // Deletion in progress.
      assert(timer->mDeleting);
#endif
      // Timer should be stopped already.
      assert(OsTimer::isStopped(timer->mApplicationState));
      // No outstanding messages.
      assert(timer->mOutstandingMessages == 0);
#ifndef NDEBUG
      // Set mDeleting to FALSE to the destructor won't fail.
      timer->mDeleting = FALSE;
#endif

      // Use ordinary destructor to delete the timer.
      // Because of the state of the timer, it will not send a message to
      // the timer task.
      delete timer;
      break;

   default:
      // Catch invalid values.
      assert(FALSE);
   }

   return TRUE;
}

/** Fire a timer because it has expired.
 *  Calls the if notification routine, if the timer hasn't been stopped
 *  already.
 *  If the timer is periodic and hasn't been stopped, reinserts it into
 *  the queue.
 *  Advances the timer's state if it is one-shot or has been stopped.
 */
void OsTimerTask::fireTimer(OsTimer* timer)
{
   OsLock lock(timer->mBSem);
   UtlBoolean report;

   // mDeleting may be true, if the destructor has started running.

   // Determine if this firing should be reported, or whether the
   // timer has been stopped since we were informed that it started.
   report = timer->mTaskState == timer->mApplicationState;

   if (!report)
   {
      // If this firing is after the timer has been stopped by
      // the application, advance mTaskState to a stopped state
      // to recognize that the timer has been removed from the
      // timer queue.
      timer->mTaskState++;
   }
   else if (report && !timer->mQueuedPeriodic)
   {
      // If this firing should be reported, and this is a one-shot
      // timer, stop the timer:
      // advance both mTaskState and mApplicationState
      timer->mTaskState = timer->mApplicationState = timer->mTaskState + 1;
   }

   // If this firing should be reported, and this is a periodic
   // timer, re-set its firing time.
   if (report && timer->mQueuedPeriodic)
   {
      timer->mQueuedExpiresAt = OsTimer::addInterval(timer->mQueuedExpiresAt,
                                                     timer->mQueuedPeriod);
      // Insert the timer into the active timer queue.
      insertTimer(timer);
   }

   // Call the event routine if we are supposed to.
   if (report)
   {
      timer->mpNotifier->signal((int) timer);
      timer->mWasFired = TRUE;
   }
}

// Insert a timer into the timer queue.
void OsTimerTask::insertTimer(OsTimer* timer)
{
   OsTimer** previous_ptr;
   OsTimer* current;
   assert(timer->mTimerQueueLink == 0);
   // Check to see if the firing time is in the past.
   // This is not an error, but is unusual and probably indicates a backlog
   // in processing.
   if (OsSysLog::willLog(FAC_KERNEL, PRI_WARNING))
   {
      // Check to see if timer firing took too long.
      OsTimer::Time now = OsTimer::now();
      if (OsTimer::compareTimes(timer->mQueuedExpiresAt, now) < 0)
      {
         OsSysLog::add(FAC_KERNEL, PRI_WARNING,
                       "OsTimerTask::insertTimer timer to fire %"PRId64" in the past"
                       " (now=%"PRId64", fire=%"PRId64")",
                       OsTimer::subtractTimes(now, timer->mQueuedExpiresAt),
                       now, timer->mQueuedExpiresAt);
      }
   }

   // Scan through the timer queue, looking for the right place to
   // insert the timer.
   for (previous_ptr = &mTimerQueue, current = mTimerQueue;
        current &&
           OsTimer::compareTimes(timer->mQueuedExpiresAt,
                                 current->mQueuedExpiresAt) > 0;
        previous_ptr = &current->mTimerQueueLink,
           current = current->mTimerQueueLink)
   {
      /* null */
   }

   // Insert the timer.
   *previous_ptr = timer;
   timer->mTimerQueueLink = current;
}

// Remove a timer from the timer queue.
void OsTimerTask::removeTimer(OsTimer* timer)
{
   OsTimer** previous_ptr;
   OsTimer* current;

   // Scan through the timer queue, looking for this timer.
   for (previous_ptr = &mTimerQueue, current = mTimerQueue;
        current && current != timer;
        previous_ptr = &current->mTimerQueueLink,
           current = current->mTimerQueueLink)
   {
      /* null */
   }

   // Remove the timer, if we found it.
   if (!current)
   {
      OsSysLog::add(FAC_KERNEL, PRI_EMERG,
                    "OsTimerTask::removeTimer timer not found in queue");
      // mDeleting is not used if NDEBUG is defined, but we always initialize
      // it to FALSE in the constructors anyway.
      OsSysLog::add(FAC_KERNEL, PRI_EMERG,
                    "OsTimerTask::removeTimer timer = %p, mApplicationState = %d, mTaskState = %d, mDeleting = %d, mPeriodic = %d, mTimerQueueLink = %p",
                    timer, timer->mApplicationState, timer->mTaskState, timer->mDeleting,
                    timer->mPeriodic, timer->mTimerQueueLink);
      OsTimer* p;
      for (p = mTimerQueue; p; p = p->mTimerQueueLink)
      {
         OsSysLog::add(FAC_KERNEL, PRI_EMERG,
                       "OsTimerTask::removeTimer in queue %p", p);
      }
      OsSysLog::add(FAC_KERNEL, PRI_EMERG,
                    "OsTimerTask::removeTimer end of queue");
   }
   assert(current);

   *previous_ptr = timer->mTimerQueueLink;
   // Clear the timer's mTimerQueueLink to indicate it is not in the
   // timer queue.
   timer->mTimerQueueLink = 0;
}

/* ============================ FUNCTIONS ================================= */
