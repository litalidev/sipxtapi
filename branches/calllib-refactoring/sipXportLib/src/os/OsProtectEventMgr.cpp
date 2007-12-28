//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


// SYSTEM INCLUDES
#include <assert.h>

// APPLICATION INCLUDES

// Keep OsProtectEventMgrInit.h as the first include!
// See OsProtectEventMgrInit class description for more information.
#include "os/OsProtectEventMgrInit.h"

#include "os/OsProtectEventMgr.h"
#include "os/OsSysLog.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
OsProtectEventMgr* OsProtectEventMgr::spInstance;

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

OsProtectEventMgr* OsProtectEventMgr::getEventMgr()
{
   return spInstance;
}

// Constructor
OsProtectEventMgr::OsProtectEventMgr(int userData,
                           int initialCount,
                                                                        int softLimit,
                                                                        int hardLimit,
                                                                        int increment)
:       mListSem(OsBSem::Q_PRIORITY, OsBSem::FULL)
{
        mCurrentCount = 0;
        mNext = 0;

        mInitialCount = (initialCount > 1) ? initialCount : 10;
        mSoftLimit = (initialCount > softLimit) ? initialCount : softLimit;
        mHardLimit = (softLimit > hardLimit) ? softLimit : hardLimit;
        mIncrement = increment;

        if (mHardLimit > initialCount) {
          assert(mIncrement>0);
          mIncrement = (mIncrement>0) ? mIncrement : 1;
        }

        mpEvents = new OsProtectedEvent*[mHardLimit];

        int i;
        for (i=0; i<mHardLimit; i++)
           mpEvents[i] = NULL;

        OsProtectedEvent* pEvent;
        for (i=0; i<mInitialCount; i++)
        {
          pEvent = new OsProtectedEvent(userData);
          if (NULL != pEvent)
          {
                 pEvent->setInUse(FALSE);
                 mpEvents[i] = pEvent;
                 mCurrentCount++;
          }
        }

        mAllocs = 0;
        mFrees = 0;
}

// Destructor
OsProtectEventMgr::~OsProtectEventMgr()
{
   OsProtectedEvent* pEvent;

   mListSem.acquire();
   for (int i=0; i<mCurrentCount; i++)
   {
      pEvent = mpEvents[i];
      if (NULL != pEvent)
          {
         if (!pEvent->isInUse())
                 {
            mpEvents[i] = NULL;
            delete pEvent;
         }
      }
   }
   delete[] mpEvents;
   mpEvents = NULL;

   mListSem.release();
}

/* ============================ MANIPULATORS ============================== */

   // Find and return an available element of the pool, creating more if
   // necessary and permitted.  Return NULL if failure.

OsProtectedEvent* OsProtectEventMgr::alloc(int userData)
{
        int i;
        OsProtectedEvent* pEvent = NULL;
        OsProtectedEvent* ret = NULL;

        mListSem.acquire();

        for (i=0; ((i<mCurrentCount)&&(NULL==ret)); i++)
        {
          pEvent = mpEvents[mNext++];
          if ((NULL != pEvent) && !pEvent->isInUse())
          {
                 pEvent->setInUse(TRUE);
                 ret = pEvent;
       break;
          }
          if (mNext >= mCurrentCount) mNext = 0;
        }

        if (NULL == ret)
        {
          if (mCurrentCount > mSoftLimit)
          {
          OsSysLog::add(FAC_KERNEL, PRI_INFO,
                     "*** OsProtectedEvent: pool exceeds soft limit (%d>%d) ***",
                                    mCurrentCount, mSoftLimit);
          }
          if (mCurrentCount < mHardLimit)
          {
                 int limit;

                 mNext = mCurrentCount;
                 limit = mCurrentCount + mIncrement;
                 if (limit > mHardLimit) limit = mHardLimit;
                 for (i=mCurrentCount; i<limit; i++)
                 {
                        pEvent = new OsProtectedEvent(userData);
                        if (NULL != pEvent)
                        {
                           pEvent->setInUse(FALSE);
                           mpEvents[i] = pEvent;
                           mCurrentCount++;
                        }
                 }
                 ret = mpEvents[mNext];
                 assert(NULL!=ret);
                 if ((NULL != ret) && !ret->isInUse())
                 {
                        ret->setInUse(TRUE);
                 }
                 mNext++;
                 if (mNext >= mCurrentCount) mNext = 0;
          }
          else
          {
        OsSysLog::add(FAC_KERNEL, PRI_CRIT,
                        "*** OsProtectEventMgr: pool exceeds hard limit (%d) *** ", mHardLimit);
          }
        }

        if (ret != NULL)
                mAllocs++;

    // If the number of outstanding events is a multiple of 10
    if(((mAllocs - mFrees) % (mIncrement/5)) == 0)
    {
        OsSysLog::add(FAC_KERNEL, PRI_DEBUG, "OsProtectEventMgr::alloc in use: %d pool size: %d num. allocs:%d",
            mAllocs - mFrees, mCurrentCount, mAllocs);
    }

        mListSem.release();
        return ret;
}

OsStatus OsProtectEventMgr::release(OsProtectedEvent* pEvent)
{
        mListSem.acquire();
        pEvent->setInUse(FALSE);
        mFrees++;
        mListSem.release();

        return OS_SUCCESS;
}

/* ============================ ACCESSORS ================================= */

// Return the event data that was signaled by the notifier task.
// Return OS_NOT_SIGNALED if the event has not been signaled (or has
// already been cleared), otherwise return OS_SUCCESS.
int OsProtectEventMgr::allocCnt()
{
     return mAllocs;
}

// Return the user data specified when this object was constructed.
// Always returns OS_SUCCESS.
int OsProtectEventMgr::availCnt()
{
   return (mCurrentCount - mAllocs + mFrees);
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
