//  
// Copyright (C) 2006-2007 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


/* There used to be #ifdef's here to do the same thing on WIN32/VXWORKS, but I
* took them out because we do the same thing on every OS. -Mike */
#define PRINTF Zprintf

// SYSTEM INCLUDES
#include <assert.h>

// APPLICATION INCLUDES
#include "os/OsLock.h"
#include "os/OsMsgPool.h"
#include "utl/UtlHashBagIterator.h"
#include "utl/UtlPtr.h"
#include "mp/MpFlowGraphBase.h"
#include "mp/MpMisc.h"
#include "mp/MpMediaTask.h"
#include "mp/MpMediaTaskMsg.h"
#include "mp/MpBufferMsg.h"
#include "mp/MpCodecFactory.h"
#include "mp/MpCodec.h"

#ifdef RTL_ENABLED
#   include <rtl_macro.h>
#endif

// Include sys/time.h if _PROFILE is set.
// This #include has to be after the #include of mp/MpMediaTask.h, because
// that is where we normally set _PROFILE.
#ifdef _PROFILE /* [ */
#include <sys/time.h>
#endif /* _PROFILE ] */

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#ifdef __pingtel_on_posix__ /* [ */
#define MPMEDIA_DEF_MAX_MSGS 1000
#else
#define MPMEDIA_DEF_MAX_MSGS OsServerTask::DEF_MAX_MSGS
#endif /* __pingtel_on_posix__ ] */

// STATIC VARIABLE INITIALIZATIONS
MpMediaTask* MpMediaTask::spInstance = NULL;
OsBSem       MpMediaTask::sLock(OsBSem::Q_PRIORITY, OsBSem::FULL);


#ifdef _PROFILE /* [ */
   static long long sSignalTicks; // Time (in microseconds) for the current
                                  // frame start signal
   static unsigned long long sMinTicks;
   static unsigned long long sMaxTicks;
#endif /* _PROFILE ] */

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Return a pointer to the media task, creating it if necessary
MpMediaTask* MpMediaTask::getMediaTask()
{
   UtlBoolean isStarted;

   // If the task object already exists, and the corresponding low-level task
   // has been started, then use it
   if (spInstance != NULL && spInstance->isStarted())
      return spInstance;

   // If the task does not yet exist or hasn't been started, then acquire
   // the lock to ensure that only one instance of the task is started
   OsLock lock(sLock);
   if (spInstance == NULL)
       spInstance = new MpMediaTask();

   isStarted = spInstance->isStarted();
   if (!isStarted)
   {
      isStarted = spInstance->start();
      assert(isStarted);
   }

   return spInstance;
}

// Destructor
MpMediaTask::~MpMediaTask()
{
   // $$$ need to figure out how to cleanly shut down this task after
   // $$$ unmanaging and destroying all of its flow graphs

   // there shouldn't be any flowgraphs here at this point
   assert(mManagedFlowGraphs.entries() == 0);

   mManagedFlowGraphs.destroyAll();

   if (mpBufferMsgPool != NULL)
      delete mpBufferMsgPool;

   if (mpSignalMsgPool != NULL)
      delete mpSignalMsgPool;

   spInstance = NULL;
}

/* ============================ MANIPULATORS ============================== */

// Directs the media processing task to add the flow graph to its 
// set of managed flow graphs.  The flow graph must be in the 
// MpFlowGraphBase::STOPPED state when this method is invoked.
// Returns OS_INVALID_ARGUMENT if the flow graph is not in the STOPPED state.
// Otherwise returns OS_SUCCESS to indicate that the flow graph will be added
// to the set of managed flow graphs at the start of the next frame
// processing interval.
OsStatus MpMediaTask::manageFlowGraph(MpFlowGraphBase& rFlowGraph)
{
   MpMediaTaskMsg msg(MpMediaTaskMsg::MANAGE, &rFlowGraph);
   OsStatus       res;

   if (rFlowGraph.getState() != MpFlowGraphBase::STOPPED) {
      // PRINTF("MpMediaTask::manageFlowGraph: error!\n", 0,0,0,0,0,0);
      return OS_INVALID_ARGUMENT;
   }

   res = postMessage(msg, OsTime::NO_WAIT_TIME);
   assert(res == OS_SUCCESS);

   return OS_SUCCESS;
}

// Directs the media processing task to remove the flow graph from its 
// set of managed flow graphs.
// If the flow graph is not already in the MpFlowGraphBase::STOPPED state,
// then the flow graph will be stopped before it is removed from the set
// of managed flow graphs.
// Returns OS_SUCCESS to indicate that the media task will stop managing
// the indicated flow graph at the start of the next frame processing
// interval.
OsStatus MpMediaTask::unmanageFlowGraph(MpFlowGraphBase& rFlowGraph)
{
   MpMediaTaskMsg msg(MpMediaTaskMsg::UNMANAGE, &rFlowGraph);
   OsStatus       res;

   res = postMessage(msg, OsTime::NO_WAIT_TIME);
   assert(res == OS_SUCCESS);

   return OS_SUCCESS;
}

// When "debug" mode is enabled, the "time limit" checking is 
// disabled and the wait for "frame start" timeout is set to "OS_INFINITY".
// For now, this method always returns OS_SUCCESS.
OsStatus MpMediaTask::setDebug(UtlBoolean enableFlag)
{
   // osPrintf("\nMpMediaTask::setDebug(%s)\n", enableFlag?"TRUE":"FALSE");
   mDebugEnabled = enableFlag;

   return OS_SUCCESS;
}

// Changes the focus to the indicated flow graph.
// At most one flow graph at a time can have focus.  Only the flow
// graph that has focus is allowed to access the audio resources
// (speaker and microphone) of the phone.
// The affected flow graphs will be modified to reflect the change of
// focus at the beginning of the next frame interval. For now, this method
// always returns OS_SUCCESS.
OsStatus MpMediaTask::setFocus(MpFlowGraphBase* pFlowGraph)
{
   MpMediaTaskMsg msg(MpMediaTaskMsg::SET_FOCUS, pFlowGraph);
   OsStatus       res;

   res = postMessage(msg, OsTime::NO_WAIT_TIME);
   assert(res == OS_SUCCESS);

   return OS_SUCCESS;
}

// Sets the amount of time (in microseconds) allotted to the media 
// processing task for processing a frame's worth of media.
// If this time limit is exceeded, the media processing task increments
// an internal statistic.  The value of this statistic can be retrieved
// by calling the getLimitExceededCnt() method. For now, this method
// always returns OS_SUCCESS.
OsStatus MpMediaTask::setTimeLimit(int usecs)
{
#ifdef _PROFILE /* [ */
   // Convert to nanoseconds.
   mLimitTicks = usecs * 1000;
#endif /* _PROFILE ] */
   mLimitUsecs = usecs;

   return OS_SUCCESS;
}

// Sets the maximum time (in milliseconds) that the media processing 
// task will wait for a "frame start" signal. A value of -1 indicates 
// that the task should wait "forever".
// The new timeout will take effect at the beginning of the next frame
// interval. For now, this method always returns OS_SUCCESS.
OsStatus MpMediaTask::setWaitTimeout(int msecs)
{
   assert(msecs >= -1);

   if (msecs == -1)
      mSemTimeout = OsTime::OS_INFINITY;
   else
   {
      OsTime tmpTime(msecs/1000, (msecs % 1000) * 1000);
      mSemTimeout = tmpTime;
   }

   return OS_SUCCESS;
}

// (static) Release the "frame start" semaphore.  This signals the media 
// processing task that it should begin processing the next frame.
// Returns the result of releasing the binary semaphore that is used to send
// the signal.
OsStatus MpMediaTask::signalFrameStart(void)
{
   OsStatus ret = OS_TASK_NOT_STARTED;
   MpMediaTaskMsg* pMsg;

   if (spInstance && spInstance->getFrameStartMsgs() >= 2)
   {
      // don't post message, as we already have 2
      return OS_SUCCESS;
   }
   
   // If the Media Task has been started
   if (spInstance != NULL) {
      pMsg = (MpMediaTaskMsg*) spInstance->mpSignalMsgPool->findFreeMsg();
      if (NULL == pMsg) {
         ret = OS_LIMIT_REACHED;
      } else {
#ifdef _PROFILE /* [ */
         timeval t;
         gettimeofday(&t, NULL);
         long long now = (t.tv_sec * 1000000) + t.tv_usec;
         // Record the time into the message
         pMsg->setInt1(t.tv_sec);
         pMsg->setInt2(t.tv_usec);
         // Record the tick interval into the histogram.
         if (spInstance->mSignalTime.tally(now - sSignalTicks) >= 1000)
         {
            UtlString* print = spInstance->mSignalTime.show();
            OsSysLog::add(FAC_MP, PRI_NOTICE,
                          "MpMediaTask::signalFrameStart %-18s %d%s",
                          "mSignalTime",
                          spInstance->mSignalTime.getBinSize(),
                          print->data());
            delete print;
            spInstance->mSignalTime.clear();
         }
         sSignalTicks = now; // record the time
#endif /* _PROFILE, __pingtel_on_posix__ ] */

         ret = spInstance->postMessage(*pMsg, OsTime::NO_WAIT_TIME);
         spInstance->nFrameStartMsgs++;
      }
   }
   return ret;
}

// Directs the media processing task to start the specified flow 
// graph.  A flow graph must be started in order for it to process 
// the media stream.
// The flow graph state change will take effect at the beginning of the
// next frame interval. For now, this method always returns OS_SUCCESS.
OsStatus MpMediaTask::startFlowGraph(MpFlowGraphBase& rFlowGraph)
{
   MpMediaTaskMsg msg(MpMediaTaskMsg::START, &rFlowGraph);
   OsStatus       res;

   res = postMessage(msg, OsTime::NO_WAIT_TIME);
   if (res != OS_SUCCESS)
   {
      OsSysLog::add(FAC_MP, PRI_DEBUG, " MpMediaTask::startFlowGraph - post"
         " returned %d, try again, will block", res);
         res = postMessage(msg);
      OsSysLog::add(FAC_MP, PRI_DEBUG, " MpMediaTask::startFlowGraph -"
         " re-post returned %d", res);
   }
   assert(res == OS_SUCCESS);

   return OS_SUCCESS;
}

// Directs the media processing task to stop the specified flow 
// graph.  When a flow graph is stopped it no longer processes the 
// media stream.
// The flow graph state change will take effect at the beginning of the
// next frame interval. For now, this method always returns OS_SUCCESS.
OsStatus MpMediaTask::stopFlowGraph(MpFlowGraphBase& rFlowGraph)
{
   MpMediaTaskMsg msg(MpMediaTaskMsg::STOP, &rFlowGraph);
   OsStatus       res;

   res = postMessage(msg, OsTime::NO_WAIT_TIME);
   assert(res == OS_SUCCESS);

   return OS_SUCCESS;
}

/* ============================ ACCESSORS ================================= */

// Debug aid for tracking state. See MpMediaTaskTest
int MpMediaTask::numHandledMsgErrs()
{
    int ret = mHandleMsgErrs;
    // reset the handleMessage error count
    mHandleMsgErrs = 0;
    return ret;
}

// Returns TRUE if debug mode is enabled, FALSE otherwise.
UtlBoolean MpMediaTask::getDebugMode(void) const
{
   return mDebugEnabled;
}

// Returns the flow graph that currently has focus (access to the audio 
// apparatus) or NULL if there is no flow graph with focus.
MpFlowGraphBase* MpMediaTask::getFocus(void) const
{
   return mpFocus;
}

// Returns the number of times that the frame processing time limit 
// has been exceeded.
int MpMediaTask::getLimitExceededCnt(void) const
{
   return mTimeLimitCnt;
}

// Returns an array of MpFlowGraphBase pointers that are presently managed 
// by the media processing task.
// The caller is responsible for allocating the flowGraphs array
// containing room for "size" pointers.  The number of items
// actually filled in is passed back via the "nItems" argument.
OsStatus MpMediaTask::getManagedFlowGraphs(MpFlowGraphBase* flowGraphs[],
                                           const int size, int& numItems)
{
   OsLock lock(mMutex);

   UtlHashBagIterator itor(mManagedFlowGraphs);
   size_t nManagedFlowGraphs = mManagedFlowGraphs.entries();
   UtlPtr<MpFlowGraphBase>* ptr;
   
   numItems = (nManagedFlowGraphs > (size_t)size) ? size : nManagedFlowGraphs;
   for (int i = 0; i < numItems; i++)
   {
      ptr = (UtlPtr<MpFlowGraphBase>*)itor();
      flowGraphs[i] = ptr->getValue();
   }

   return OS_SUCCESS;
}

// Returns the amount of time (in microseconds) allotted to the media 
// processing task for processing a frame's worth of media.
int MpMediaTask::getTimeLimit(void) const
{
   return mLimitUsecs;
}

// Returns the maximum time (in milliseconds) that the media processing 
// task will wait for the "frame start" signal. A value of -1 indicates 
// that the task will wait "forever".
int MpMediaTask::getWaitTimeout(void) const
{
   if (mSemTimeout.isInfinite())
      return -1;
   else
      return mSemTimeout.cvtToMsecs();
}

// Returns the number of times that the wait timeout associated with 
// "frame start" signal has been exceeded.
int MpMediaTask::getWaitTimeoutCnt(void) const
{
   return mSemTimeoutCnt;
}

// (static) Displays information on the console about the media processing
// task.
MpFlowGraphBase* MpMediaTask::mediaInfo(void)
{
   MpFlowGraphBase* flowGraphs[20];
   int              i;
   int              numItems;
   MpMediaTask*     pMediaTask;
   MpFlowGraphBase* pFlowGraph;
   OsStatus         res;

   pMediaTask = MpMediaTask::getMediaTask();

   osPrintf("\nMedia processing task information\n");
   osPrintf("  Debug mode:                      %s\n",
             pMediaTask->getDebugMode() ? "TRUE" : "FALSE");

   osPrintf("  Processed Frame Count:           %d\n",
             pMediaTask->numProcessedFrames());

   osPrintf("  Processing Time Limit:           %d usecs\n",
             pMediaTask->getTimeLimit());

   osPrintf("  Processing Limit Exceeded Count: %d\n",
             pMediaTask->getLimitExceededCnt());

   i = pMediaTask->getWaitTimeout();
   if (i < 0)
      osPrintf("  Frame Start Wait Timeout:        INFINITE\n");
   else
      osPrintf("  Frame Start Wait Timeout:        %d\n", i);

   osPrintf("  Wait Timeout Exceeded Count:     %d\n",
             pMediaTask->getWaitTimeoutCnt());

   osPrintf("\n  Flow Graph Information\n");
   osPrintf("    Managed:      %d\n", pMediaTask->numManagedFlowGraphs());
   osPrintf("    Started:      %d\n", pMediaTask->numStartedFlowGraphs());

   pFlowGraph = pMediaTask->getFocus();
   if (pFlowGraph == NULL)
      osPrintf("    Focus:        NULL\n");
   else
      osPrintf("    Focus:        %p\n", pFlowGraph);

   res = pMediaTask->getManagedFlowGraphs(flowGraphs, 20, numItems);
   for (i=0; i < numItems; i++)
      osPrintf("    FlowGraph[%d]: %p\n", i, flowGraphs[i]);
   return pFlowGraph;
}

// Returns the number of flow graphs currently being managed by the 
// media processing task.
int MpMediaTask::numManagedFlowGraphs(void) const
{
   // hashbag has a lock inside
   return mManagedFlowGraphs.entries();
}

// Returns the number of frames that the media processing task has 
// processed. This count is maintained as an unsigned, 32-bit value.
// Note: If the frame period is 10 msecs, then it will take
// 2^32 / (100 * 3600 * 24 * 365) = 1.36 years before this count wraps.
int MpMediaTask::numProcessedFrames(void) const
{
   return mProcessedCnt;
}

// Returns the number of flow graphs that have been started by the media 
// processing task.
// This value should always be <= the number of managed flow graphs.
int MpMediaTask::numStartedFlowGraphs(void) const
{
   return mStartedCnt;
}

// Returns pointer to pool of reusable buffer messages
OsMsgPool* MpMediaTask::getBufferMsgPool(void) const
{
   return mpBufferMsgPool;
}


/* ============================ INQUIRY =================================== */

void MpMediaTask::getQueueUsage(int& numMsgs)
{
   numMsgs = mIncomingQ.numMsgs();
   OsSysLog::add(FAC_MP, PRI_DEBUG,
                 "MpMediaTask::getQueueUsage "
                 "numMsgs = %d",
                 numMsgs);
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

// Default constructor (called only indirectly via getMediaTask())
MpMediaTask::MpMediaTask()
:  OsServerTask("MpMedia", NULL, MPMEDIA_DEF_MAX_MSGS,
                MEDIA_TASK_PRIORITY),
   mMutex(OsMutex::Q_PRIORITY),  // create mutex for protecting data
   mDebugEnabled(FALSE),
   mTimeLimitCnt(0),
   mProcessedCnt(0),
   mStartedCnt(0),
   mSemTimeout(DEF_SEM_WAIT_MSECS / 1000, (DEF_SEM_WAIT_MSECS % 1000) * 1000),
   mSemTimeoutCnt(0),
   mWaitForSignal(TRUE),
   mpFocus(NULL),
   mHandleMsgErrs(0),
   mpBufferMsgPool(NULL),
   mManagedFlowGraphs(),
   // numQueuedMsgs(0),
   mpSignalMsgPool(NULL),
   nFrameStartMsgs(0)
#ifdef _PROFILE /* [ */
   ,
   mStartToEndTime(20, 0, 1000, " %4d", 5),
   mStartToStartTime(20, 0, 1000, " %4d", 5),
   mEndToStartTime(20, 0, 1000, " %4d", 5),
   mSignalTime(20, 0, 1000, " %4d", 5),
   mSignalToStartTime(20, 0, 1000, " %4d", 5),
   mOtherMessages(20, 0, 1000, " %4d", 5)
#endif /* _PROFILE ] */
{
   OsStatus res;

   res = setTimeLimit(DEF_TIME_LIMIT_USECS);
   assert(res == OS_SUCCESS);

   int totalNumBufs = MpMisc.AudioHeadersPool->getNumBlocks()
      + MpMisc.RtpHeadersPool->getNumBlocks()
#ifdef REAL_RTCP // [
      + MpMisc.RtcpHeadersPool->getNumBlocks()
#endif // ] TINY_MEDIALIB
      ;
   int soft = totalNumBufs/20;
   if (soft < 8) soft = 8;
   {
      MpBufferMsg msg(MpBufferMsg::AUD_RECORDED);
      mpBufferMsgPool = new OsMsgPool("MediaBuffers", msg,
                          soft, soft,
                          OsMsgPool::MULTIPLE_CLIENTS);
   }

   {
      MpMediaTaskMsg msg(MpMediaTaskMsg::WAIT_FOR_SIGNAL);
      mpSignalMsgPool = new OsMsgPool("MediaSignals", msg,
                          soft, soft, 
                          OsMsgPool::MULTIPLE_CLIENTS);
   }

   mpCodecFactory = MpCodecFactory::getMpCodecFactory();
#ifdef _PROFILE /* [ */
   mStartTicks = 0;
   mStopTicks = 0;
   sSignalTicks = 0;
   sMinTicks = 0;
   sMaxTicks = 0;
#endif /* _PROFILE ] */
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

// Handle an incoming message
// Return TRUE if the message was handled, otherwise FALSE.
UtlBoolean MpMediaTask::handleMessage(OsMsg& rMsg)
{
   UtlBoolean           handled;
   MpFlowGraphBase*    pFlowGraph;
   MpMediaTaskMsg*     pMsg;

   if (rMsg.getMsgType() != OsMsg::MP_TASK_MSG)
      return FALSE;    // the method only handles MP_TASK_MSG messages

   pMsg = (MpMediaTaskMsg*) &rMsg;
   pFlowGraph = (MpFlowGraphBase*) pMsg->getPtr1();

   handled = TRUE;     // until proven otherwise, assume we'll handle the msg

#ifdef _PROFILE
   osPrintf("MpMediaTask queue size: %i, message type: %i\n", this->getMessageQueue()->numMsgs(), pMsg->getMsg());
   OsTime start_time;
   OsDateTime::getCurTime(start_time);
#endif

   switch (pMsg->getMsg())
   {
   case MpMediaTaskMsg::MANAGE:
      if (!handleManage(pFlowGraph))
         mHandleMsgErrs++;
      break;
   case MpMediaTaskMsg::SET_FOCUS:
      if (!handleSetFocus(pFlowGraph))
         mHandleMsgErrs++;
      break;
   case MpMediaTaskMsg::START:
      if (!handleStart(pFlowGraph))
         mHandleMsgErrs++;
      break;
   case MpMediaTaskMsg::STOP:
      if (!handleStop(pFlowGraph))
         mHandleMsgErrs++;
      break;
   case MpMediaTaskMsg::UNMANAGE:
      if (!handleUnmanage(pFlowGraph))
         mHandleMsgErrs++;
      break;
   case MpMediaTaskMsg::WAIT_FOR_SIGNAL:
      if (!handleWaitForSignal(pMsg))
         mHandleMsgErrs++;
      break;
   default:
      handled = FALSE; // we didn't handle the message after all
      break;
   }

#ifdef _PROFILE
   OsTime end_time;
   OsDateTime::getCurTime(end_time);
   OsTime difference = end_time - start_time;
   osPrintf("MpMediaTask: Execution took %is, %ius\n", difference.seconds(), difference.usecs());
#endif

   return handled;
}

// Handles the MANAGE message.
// Returns TRUE if the message was handled, otherwise FALSE.
UtlBoolean MpMediaTask::handleManage(MpFlowGraphBase* pFlowGraph)
{
   OsLock lock(mMutex);

   if (isManagedFlowGraph(pFlowGraph)) { // we are already managing
      // PRINTF("MpMediaTask::handleManage: ERROR: flow graph already managed!\n", 0,0,0,0,0,0);
      return FALSE;                      // the flow graph, return FALSE
   }

   // PRINTF("MpMediaTask::handleManage: Adding flow graph # %d!\n", mManagedCnt, 0,0,0,0,0);

   // add flowgraph to hashbag
   UtlContainable* pRes = mManagedFlowGraphs.insert(new UtlPtr<MpFlowGraphBase>(pFlowGraph));
   assert(pRes);

   return pRes != NULL;
}

// Handles the SET_FOCUS message.
// Returns TRUE if the message was handled, otherwise FALSE.
UtlBoolean MpMediaTask::handleSetFocus(MpFlowGraphBase* pFlowGraph)
{
   if (pFlowGraph != NULL)
   {
      if (!isManagedFlowGraph(pFlowGraph) ||
          !pFlowGraph->isStarted())
      {
         Nprintf("MpMT::handleSetFocus(0x%X) INVALID: %smanaged, %sstarted\n",
            (int) pFlowGraph,
            (int) (isManagedFlowGraph(pFlowGraph)? "" : "NOT "),
            (int) (pFlowGraph->isStarted() ? "" : "NOT "), 0,0,0);
         return FALSE; // we aren't managing this flow graph, return FALSE
      }
   }

   if (mpFocus != NULL)
   {
      // remove focus from the flow graph that currently has it
      Nprintf("MpMT::handleSetFocus(0x%X): removing old focus (0x%X)\n",
         (int) pFlowGraph, (int) mpFocus, 0,0,0,0);
      mpFocus->loseFocus();
   }

   mpFocus = pFlowGraph;
   if (mpFocus != NULL)
   {
      // try to give focus to the indicated flow graph
      if (OS_SUCCESS != mpFocus->gainFocus())
      {
         Nprintf("MpMT::handleSetFocus(0x%X): attempt to give focus FAILED\n",
            (int) pFlowGraph, 0,0,0,0,0);
         mpFocus = NULL;
         return FALSE; // the flow graph did not accept focus.
      }
      Nprintf("MpMT::handleSetFocus(0x%X): attempt to give focus SUCCEEDED\n",
         (int) pFlowGraph, 0,0,0,0,0);
   }

   return TRUE;
}

// Handles the START message.
// Returns TRUE if the message was handled, otherwise FALSE.
UtlBoolean MpMediaTask::handleStart(MpFlowGraphBase* pFlowGraph)
{
   OsStatus res;

   if (!isManagedFlowGraph(pFlowGraph))
      return FALSE;  // flow graph is not presently managed, return FALSE

   if (pFlowGraph->isStarted())   // if already started, return FALSE
      return FALSE;

   res = pFlowGraph->start();
   assert(res == OS_SUCCESS);

   mStartedCnt++;
   return TRUE;
}

// Handles the STOP message.
// Returns TRUE if the message was handled, otherwise FALSE.
UtlBoolean MpMediaTask::handleStop(MpFlowGraphBase* pFlowGraph)
{
   OsStatus res;

   if (pFlowGraph == mpFocus)
            handleSetFocus(NULL);

   if (!isManagedFlowGraph(pFlowGraph))
      return FALSE;  // flow graph is not presently managed, return FALSE

   if (pFlowGraph->getState() == MpFlowGraphBase::STOPPED)
      return FALSE;               // if already stopped, return FALSE

   res = pFlowGraph->stop();
   assert(res == OS_SUCCESS);

   mStartedCnt--;
   return TRUE;
}

// Handles the UNMANAGE message.
// Returns TRUE if the message was handled, otherwise FALSE.
UtlBoolean MpMediaTask::handleUnmanage(MpFlowGraphBase* pFlowGraph)
{
   OsLock lock(mMutex);
   UtlBoolean   found;
   OsStatus    res;

   if (pFlowGraph == mpFocus)
            handleSetFocus(NULL);

   if (!isManagedFlowGraph(pFlowGraph)) {
      return FALSE;  // flow graph is not presently managed, return FALSE
   }

   if (pFlowGraph->getState() != MpFlowGraphBase::STOPPED)
   {
      handleStop(pFlowGraph);

      // since we have "unmanaged" this flow graph, we need to coerce the
      // flow graph into processing its messages so that it gets the
      // indication that it has been stopped.
      res = pFlowGraph->processNextFrame();
      assert(res == OS_SUCCESS);
   }

   UtlPtr<MpFlowGraphBase> ptr(pFlowGraph);
   // deletes the old UtlPtr we allocated earlier
   found = mManagedFlowGraphs.destroy(&ptr);

   return found;
}

// Handles the WAIT_FOR_SIGNAL message.
// Performs the one-per-tick media processing as directed by the flow graph.
// Returns TRUE if the message was handled, otherwise FALSE.
UtlBoolean MpMediaTask::handleWaitForSignal(MpMediaTaskMsg* pMsg)
{
   OsStatus         res;

#ifdef MEDIA_VERBOSE /* [ */
   static int lastmMCnt = -1;
#endif /* MEDIA_VERBOSE ] */

#ifdef _PROFILE /* [ */
   {
      // Get the current time.
      timeval t;
      gettimeofday(&t, NULL);
      long long now = (t.tv_sec * 1000000) + t.tv_usec;
      // From the message, get the time that it was sent.
      long long signal_time =
         (pMsg->getInt1() * 1000000) + pMsg->getInt2();
      // Record intervals.
      mStartToStartTime.tally(now - mStartTicks);
      mEndToStartTime.tally(now - mStopTicks);
      mSignalToStartTime.tally(now - signal_time);
      // Record the processing start time.
      mStartTicks = now;
   }
#endif /* _PROFILE ] */

   // reset the handleMessage error count
   // mHandleMsgErrs = 0;

   mWaitForSignal = FALSE;

#ifdef MEDIA_VERBOSE /* [ */
   if (0 && (0 == (mProcessedCnt % 100))) {
      PRINTF("handleWaitForSignal: %d frames, %d managed flow graphs\n",
             mProcessedCnt, mManagedCnt, 0,0,0,0);
   }

   if (mManagedCnt != lastmMCnt)
   {
      // PRINTF("handleWaitForSignal: %d frames, %d managed flow graphs\n",
      mProcessedCnt, mManagedCnt, 0,0,0,0);
      lastmMCnt = mManagedCnt;
   }
#endif /* MEDIA_VERBOSE ] */

   // When this message is received we know that:
   // 1) We have received a frame start signal
   // 2) All of the messages that had been queued for this task at the
   //    time the frame start signal occurred have been processed.

   // Call processNextFrame() for each of the "started" flow graphs
   UtlHashBagIterator itor(mManagedFlowGraphs);
   UtlPtr<MpFlowGraphBase>* ptr = NULL;
   MpFlowGraphBase* pFlowGraph = NULL;

   while(ptr = (UtlPtr<MpFlowGraphBase>*)itor())
   {
      pFlowGraph = ptr->getValue();
      assert(pFlowGraph);

      if (pFlowGraph->isStarted())
      {
         res = pFlowGraph->processNextFrame();
         assert(res == OS_SUCCESS);
      }
      
   }

#ifdef RTL_ENABLED
    RTL_EVENT("MpMediaTask.handleWaitForSignal", 0);
#endif
#ifdef _PROFILE /* [ */
   {
      timeval t;
      gettimeofday(&t, NULL);
      // Record the processing stop time.
      mStopTicks = (t.tv_sec * 1000000) + t.tv_usec;
      // if not debugging, determine whether the processing limit was exceeded
      if (!mDebugEnabled)
      {
         if ((mStopTicks - mStartTicks) >= mLimitTicks) {
            mTimeLimitCnt++;
         }
      }
      if (mStartToEndTime.tally(mStopTicks - mStartTicks) >= 1000)
      {
         UtlString* print;
         // Print mSignalToStartTime.
         print = mSignalToStartTime.show();
         OsSysLog::add(FAC_MP, PRI_NOTICE,
                       "MpMediaTask::handleWaitForSignal %-18s %d%s",
                       "mSignalToStartTime",
                       mSignalToStartTime.getBinSize(),
                       print->data());
         delete print;
         mSignalToStartTime.clear();
         // Print mStartToEndTime.
         print = mStartToEndTime.show();
         OsSysLog::add(FAC_MP, PRI_NOTICE,
                       "MpMediaTask::handleWaitForSignal %-18s %d%s",
                       "mStartToEndTime",
                       mStartToEndTime.getBinSize(),
                       print->data());
         delete print;
         mStartToEndTime.clear();
         // Print mStartToStartTime.
         print = mStartToStartTime.show();
         OsSysLog::add(FAC_MP, PRI_NOTICE,
                       "MpMediaTask::handleWaitForSignal %-18s %d%s",
                       "mStartToStartTime",
                       mStartToStartTime.getBinSize(),
                       print->data());
         delete print;
         mStartToStartTime.clear();
         // Print mEndToStartTime.
         print = mEndToStartTime.show();
         OsSysLog::add(FAC_MP, PRI_NOTICE,
                       "MpMediaTask::handleWaitForSignal %-18s %d%s",
                       "mEndToStartTime",
                       mEndToStartTime.getBinSize(),
                       print->data());
         delete print;
         mEndToStartTime.clear();
         // Print mOtherMessages.
         print = mOtherMessages.show();
         OsSysLog::add(FAC_MP, PRI_NOTICE,
                       "MpMediaTask::handleWaitForSignal %-18s %d%s",
                       "mOtherMessages",
                       mOtherMessages.getBinSize(),
                       print->data());
         delete print;
         mOtherMessages.clear();
      }
   }
#endif /* _PROFILE ] */

   assert(!mWaitForSignal);
   mProcessedCnt++;
#ifdef MEDIA_VERBOSE /* [ */
   if (0 && (0 == (mProcessedCnt % 125)))
   {
      PRINTF("handleWaitForSignal: %d frames\n", mProcessedCnt, 0,0,0,0,0);
   }
#endif /* MEDIA_VERBOSE ] */
   mWaitForSignal = TRUE;

   if (nFrameStartMsgs > 0)
   {
      nFrameStartMsgs--;
   }
   
   return TRUE;
}

// Returns TRUE if the indicated flow graph is presently being managed 
// by the media processing task, otherwise FALSE.
UtlBoolean MpMediaTask::isManagedFlowGraph(MpFlowGraphBase* pFlowGraph)
{
   UtlPtr<MpFlowGraphBase> ptr(pFlowGraph);
   return mManagedFlowGraphs.contains(&ptr);
}


/* ============================ FUNCTIONS ================================= */
