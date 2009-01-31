//  
// Copyright (C) 2006 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _OsSyncBase_h_
#define _OsSyncBase_h_

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "os/OsDefs.h"
#include "os/OsStatus.h"
#include "os/OsTime.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

/**
 * @brief Base class for the synchronization mechanisms in the OS abstraction layer
 */
class OsSyncBase
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

     /// Destructor
   virtual ~OsSyncBase() { };

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */

     /// Assignment operator
   OsSyncBase& operator=(const OsSyncBase& rhs);

     /// Block until the sync object is acquired or the timeout expires
   virtual OsStatus acquire(const OsTime& rTimeout = OsTime::OS_INFINITY) = 0;

     /// Conditionally acquire the semaphore (i.e., don't block)
   virtual OsStatus tryAcquire(void) = 0;
     /**
      * @return OS_BUSY if the sync object is held by some other task.
      */

     /// Release the sync object
   virtual OsStatus release(void) = 0;

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:
	 /// Default constructor
   OsSyncBase() { };

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
     /// Copy constructor
   OsSyncBase(const OsSyncBase& rOsSyncBase);
};

/* ============================ INLINE METHODS ============================ */

#endif  // _OsSyncBase_h_

