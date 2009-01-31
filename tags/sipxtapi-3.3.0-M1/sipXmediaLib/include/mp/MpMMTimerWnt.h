//  
// Copyright (C) 2007 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// Author: Keith Kyzivat <kkyzivat AT SIPez DOT com>

#ifndef _MpMMTimerWnt_h_
#define _MpMMTimerWnt_h_

// SYSTEM INCLUDES
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <MMSystem.h>

// APPLICATION INCLUDES
#include "mp/MpMMTimer.h"
#include "os/OsMutex.h"

class MpMMTimerWnt : public MpMMTimer
{
private:
   static void CALLBACK timeProcCallback(UINT uID, UINT uMsg, DWORD dwUser, 
                                         DWORD dw1, DWORD dw2);
     /**< 
      *  @brief callback used by windows multimedia timers
      *
      *  This should only be called by the windows multimedia timer.
      */


public:
   typedef enum 
   {
      Multimedia = 0         ///< Microsoft Multimedia timers (W95+, CE)
      // Other possible choices, which have yet to be implemented: 
      // Microsoft Waitable Timers (W98/NT+) (Not Implemented)
      // Microsoft Queue Timers (W2k+) (Not Implemented)
   } MMTimerWntAlgorithms;

   MpMMTimerWnt(MpMMTimer::MMTimerType type);
   virtual ~MpMMTimerWnt();

     /// @copydoc MpMMTimer::setNotification()
   virtual inline OsStatus setNotification(OsNotification* notification);

   /// @copydoc MpMMTimer::run()
   virtual OsStatus run(unsigned usecPeriodic, 
                        unsigned uAlgorithm = MPMMTIMER_ALGORITHM_DEFAULT);

     /// @copydoc MpMMTimer::stop()
   virtual OsStatus stop();

     /// @copydoc MpMMTimer::waitForNextTick()
   OsStatus waitForNextTick();

     /// @copydoc MpMMTimer::getUSecSinceLastFire() const
   int getUSecSinceLastFire() const;

     /// @copydoc MpMMTimer::getUSecDeltaExpectedFire() const
   int getUSecDeltaExpectedFire() const;

     /// @copydoc MpMMTimer::getAbsFireTime() const
   OsTime getAbsFireTime() const;

protected:
   OsStatus runMultimedia(unsigned usecPeriodic);
     /**< 
      *  @brief start a multimedia timer.
      *
      *  This should only be used within the MpMMTimerWnt class.
      *  @see MpMMTimer::run(unsigned, unsigned)
      */

   OsStatus stopMultimedia();
     /**< 
      *  @brief stop a started multimedia timer.
      *
      *  This should only be used within the MpMMTimerWnt class.
      *  @see MpMMTimer::stop()
      */

private:
   MMTimerWntAlgorithms mAlgorithm; ///< The current or last algorithm used.
   BOOL mbInitialized; ///< Whether we're fully initialized or not, or are in some failure state.
   BOOL mbTimerStarted; ///< Indicator of timer started or not.
   unsigned mPeriodMSec; ///< The current millisecond period being used.  0 when no timer.
   unsigned mResolution; ///< Cached timer resolution in ms, queried for and stored at startup.
   HANDLE mEventHandle; ///< Only valid in Linear mode, holds handle to an event.
   OsNotification* mpNotification; ///< Notification object used to signal a tick of the timer.
   MMRESULT mTimerId; ///< The ID of the MM timer we're using.
};



// Inline Function Implementation

OsStatus MpMMTimerWnt::setNotification(OsNotification* notification)
{ 
   mpNotification = notification;
   return OS_SUCCESS;
}


#endif //_MpMMTimerWnt_h_
