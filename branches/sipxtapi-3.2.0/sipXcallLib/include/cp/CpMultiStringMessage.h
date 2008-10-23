//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _CpMultiStringMessage_h_
#define _CpMultiStringMessage_h_

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsDefs.h>
#include <os/OsMsg.h>
#include <cp/CallManager.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

//:Class short description which may consist of multiple lines (note the ':')
// Class detailed description which may extend to multiple lines
class CpMultiStringMessage : public OsMsg
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   /* ============================ CREATORS ================================== */

   CpMultiStringMessage(unsigned char messageSubtype = CallManager::CP_UNSPECIFIED,
      const char* str1 = NULL, const char* str2 = NULL,
      const char* str3 = NULL, const char* str4 = NULL, const char* str5 = NULL,
      intptr_t int1 = 0, intptr_t int2 = 0, intptr_t int3 = 0, intptr_t int4 = 0,
      intptr_t int5 = 0, intptr_t int6 = 0, intptr_t int7 = 0, intptr_t int8 = 0);
   //:Default constructor


   virtual
      ~CpMultiStringMessage();
   //:Destructor

   virtual OsMsg* createCopy(void) const;

   /* ============================ MANIPULATORS ============================== */


   /* ============================ ACCESSORS ================================= */
   void getString1Data(UtlString& str1) const;
   void getString2Data(UtlString& str2) const;
   void getString3Data(UtlString& str3) const;
   void getString4Data(UtlString& str4) const;
   void getString5Data(UtlString& str5) const;
   void getString6Data(UtlString& str6) const;
   void setString6Data(const UtlString& str6);
   intptr_t getInt1Data() const;
   intptr_t getInt2Data() const;
   intptr_t getInt3Data() const;
   intptr_t getInt4Data() const;
   intptr_t getInt5Data() const;
   intptr_t getInt6Data() const;
   intptr_t getInt7Data() const;
   intptr_t getInt8Data() const;

   void toString(UtlString& dumpString, const char* terminator = "\n") const;


   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   intptr_t mInt1;
   intptr_t mInt2;
   intptr_t mInt3;
   intptr_t mInt4;
   intptr_t mInt5;
   intptr_t mInt6;
   intptr_t mInt7;
   intptr_t mInt8;
   UtlString mString1Data;
   UtlString mString2Data;
   UtlString mString3Data;
   UtlString mString4Data;
   UtlString mString5Data;
   UtlString mString6Data;

   CpMultiStringMessage(const CpMultiStringMessage& rCpMultiStringMessage);
   //:disable Copy constructor

   CpMultiStringMessage& operator=(const CpMultiStringMessage& rhs);
   //:disable Assignment operator

};

/* ============================ INLINE METHODS ============================ */

#endif  // _CpMultiStringMessage_h_
