//
// Copyright (C) 2007 Jaroslav Libak
//
// Copyright (C) 2005-2007 SIPez LLC.
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

#ifndef CpCallIdGenerator_h__
#define CpCallIdGenerator_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsMutex.h>
#include <utl/UtlRandom.h>
#include <utl/UtlString.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
// STRUCTS
// TYPEDEFS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/**
 * Generates pseudorandom call-ids. Takes prefix, IP address, process id,
 * 2 internal counters, time, random number into account. Threadsafe.
 */
class CpCallIdGenerator
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */
   ///@name Creators
   //@{

   /**
   * Constructor.
   */
   CpCallIdGenerator(const UtlString& prefix);

   /**
   * Destructor.
   */
   ~CpCallIdGenerator(void);

   //@}

   /* ============================ MANIPULATORS ============================== */
   ///@name Manipulators
   //@{

   /**
    * Returns new pseudorandom call-id. Threadsafe.
    */
   UtlString getNewCallId();

   //@}

   /* ============================ ACCESSORS ================================= */
   ///@name Accessors
   //@{
   //@}
   /* ============================ INQUIRY =================================== */
   ///@name Inquiry
   //@{
   //@}

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   int m_processId;  /// < pid of process that created this object
   unsigned int m_instanceId; ///< object instance id
   Int64 m_creationTime;  ///< time of creation in msecs
   UtlString m_sHostIp; ///< ip address of host
   Int64 m_callIdCounter; ///< internal callid counter
   UtlString m_sPrefix; ///< prefix for callids
   UtlRandom m_RandomNumGenerator; ///< random number generator

   OsMutex m_mutex;
   char m_callIdSuffix[255]; ///< internal buffer for speedup
   static unsigned int ms_instanceCounter; ///< global instance counter
};

#endif // CpCallIdGenerator_h__
