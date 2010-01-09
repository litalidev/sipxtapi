//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


#ifndef _OsTime_h_
#define _OsTime_h_

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include "os/OsDefs.h"
#include "utl/UtlDefs.h"

// DEFINES
#define T1_PERIOD_MSEC 500 // 0.5s
#define T2_PERIOD_MSEC 4000 // 4s

// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

//:Time or time interval
// If necessary, this class will adjust the seconds and microseconds values
// that it reports such that 0 <= microseconds < USECS_PER_SEC.


class OsTime
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   /// Time quantity enum for special time values
   typedef enum
   {
      OS_INFINITY = -1,
      NO_WAIT_TIME = 0
   } TimeQuantity;

   static const long MSECS_PER_SEC;
   static const long USECS_PER_MSEC;
   static const long USECS_PER_SEC;

/* ============================ CREATORS ================================== */

   OsTime();
     //:Default constructor (creates a zero duration interval)

   OsTime(const long msecs);
     //:Constructor specifying time/duration in terms of milliseconds

   OsTime(TimeQuantity quantity);
     //:Constructor specifying time/duration in terms of TimeQuantity enum

   OsTime(const long seconds, const long usecs);
     //:Constructor specifying time/duration in terms of seconds and microseconds

   OsTime(const OsTime& rOsTime);
     //:Copy constructor

   virtual
   ~OsTime();
     //:Destructor

/* ============================ MANIPULATORS ============================== */

   OsTime& operator=(TimeQuantity rhs);
     //:Assignment operator

   OsTime& operator=(const OsTime& rhs);
     //:Assignment operator

   OsTime operator+(const OsTime& rhs);
     //:Addition operator

   OsTime operator-(const OsTime& rhs);
     //:Subtraction operator

   OsTime operator+=(const OsTime& rhs);
     //:Increment operator

   OsTime operator-=(const OsTime& rhs);
     //:Decrement operator

   bool operator==(const OsTime& rhs) const;
     //:Test for equality operator

   bool operator!=(const OsTime& rhs) const;
     //:Test for inequality operator

   bool operator>(const OsTime& rhs) const;
     //:Test for greater than

   bool operator>=(const OsTime& rhs) const;
     //:Test for greater than or equal

   bool operator<(const OsTime& rhs) const;
     //:Test for less than

   bool operator<=(const OsTime& rhs) const;
     //:Test for less than or equal

/* ============================ ACCESSORS ================================= */

   virtual long seconds(void) const
   {
      return mSeconds;
   }
     //:Return the seconds portion of the time interval

   virtual long usecs(void) const
   {
      return mUsecs;
   }
     //:Return the microseconds portion of the time interval

   virtual long cvtToMsecs(void) const;
     //:Convert the time interval to milliseconds

/* ============================ INQUIRY =================================== */

   virtual UtlBoolean isInfinite(void) const;
     //:Return TRUE if the time interval is infinite

   virtual UtlBoolean isNoWait(void) const;
     //:Return TRUE if the time interval is zero (no wait)

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   long mSeconds;
   long mUsecs;

   void init(void);
     //:Initialize the instance variables for a newly constructed object

};

/* ============================ INLINE METHODS ============================ */

#endif  // _OsTime_h_

