//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef XCpGenericCallStack_h__
#define XCpGenericCallStack_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsMutex.h>
#include <os/OsDefs.h>
#include <utl/UtlString.h>
#include <utl/UtlHashMap.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS
template <class T>
class OsPtrLock; // forward template class declaration
class XCpAbstractCall;
class XCpCall;
class XCpConference;
class SipDialog;
class SipMessage;

/**
 * Class XCpCallStack is a container for XCpCall and XCpConference instances.
 * It supports fast lookup by id, sip call-id, focused call tracking 
 */
class XCpCallStack
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   XCpCallStack();

   ~XCpCallStack();

   /* ============================ MANIPULATORS ============================== */

   /**
   * Finds and returns a call or conference as XCpAbstractCall according to given id.
   * Returned OsPtrLock unlocks XCpAbstractCall automatically, and the object should not
   * be used outside its scope.
   * @param sID Identifier of call or conference. Must not be sip call-id.
   *
   * @return TRUE if a call or conference was found, FALSE otherwise.
   */
   UtlBoolean findAbstractCall(const UtlString& sAbstractCallId,
                               OsPtrLock<XCpAbstractCall>& ptrLock) const;

   /**
   * Gets some abstract call, which is different from supplied id. Useful when some call
   * is getting shut down, and some resource needs to move to other call, different from
   * the old one.
   * @param sID Identifier of call or conference to avoid. Must not be sip call-id.
   *
   * @return TRUE if a call or conference was found, FALSE otherwise.
   */
   UtlBoolean findSomeAbstractCall(const UtlString& sAvoidAbstractCallId,
                                   OsPtrLock<XCpAbstractCall>& ptrLock) const;

   /**
   * Finds and returns a call or conference as XCpAbstractCall according to given sip call-id.
   * Returned OsPtrLock unlocks XCpAbstractCall automatically, and the object should not
   * be used outside its scope.
   * @param sSipCallId Sip call-id of the call to find.
   * @param sLocalTag Tag of From SIP message field if known
   * @param sRemoteTag Tag of To SIP message field if known
   *
   * @return TRUE if a call or conference was found, FALSE otherwise.
   */
   UtlBoolean findAbstractCall(const SipDialog& sSipDialog,
                               OsPtrLock<XCpAbstractCall>& ptrLock) const;

   /**
   * Finds and returns a XCpCall according to given id.
   * Returned OsPtrLock unlocks XCpCall automatically, and the object should not
   * be used outside its scope.
   *
   * @return TRUE if a call was found, FALSE otherwise.
   */
   UtlBoolean findCall(const UtlString& sId, OsPtrLock<XCpCall>& ptrLock) const;

   /**
   * Finds and returns a XCpCall according to given SipDialog.
   * Returned OsPtrLock unlocks XCpCall automatically, and the object should not
   * be used outside its scope.
   *
   * @return TRUE if a call was found, FALSE otherwise.
   */
   UtlBoolean findCall(const SipDialog& sSipDialog, OsPtrLock<XCpCall>& ptrLock) const;

   /**
   * Finds and returns a XCpConference according to given id.
   * Returned OsPtrLock unlocks XCpConference automatically, and the object should not
   * be used outside its scope.
   *
   * @return TRUE if a conference was found, FALSE otherwise.
   */
   UtlBoolean findConference(const UtlString& sId, OsPtrLock<XCpConference>& ptrLock) const;

   /**
   * Finds and returns a XCpConference according to given SipDialog.
   * Returned OsPtrLock unlocks XCpConference automatically, and the object should not
   * be used outside its scope.
   *
   * @return TRUE if a conference was found, FALSE otherwise.
   */
   UtlBoolean findConference(const SipDialog& sSipDialog, OsPtrLock<XCpConference>& ptrLock) const;

   /**
   * Finds and returns XCpAbstractCall capable of handling given SipMessage.
   *
   * @return TRUE if an XCpAbstractCall was found, FALSE otherwise.
   */
   UtlBoolean findHandlingAbstractCall(const SipMessage& rSipMessage, OsPtrLock<XCpAbstractCall>& ptrLock) const;

   /**
   * Pushes given XCpCall on the call stack. Call must not be locked to avoid deadlocks.
   * Only push newly created calls.
   */
   UtlBoolean push(XCpCall& call);

   /**
   * Pushes given XCpCall on the conference stack. Conference must not be locked to avoid deadlocks.
   * Only push newly created conferences.
   */
   UtlBoolean push(XCpConference& conference);

   /**
   * Deletes call identified by Id from stack. Doesn't hang up the call, just shuts
   * media resources and deletes the call.
   */
   UtlBoolean deleteCall(const UtlString& sCallId);

   /**
   * Deletes conference identified by Id from stack. Doesn't hang up the conference, just shuts
   * media resources and deletes the conference.
   */
   UtlBoolean deleteConference(const UtlString& sConferenceId);

   /**
   * Deletes abstract call identified by Id from stack. Doesn't hang up the call, just shuts
   * media resources and deletes the call. Works for both calls and conferences.
   */
   UtlBoolean deleteAbstractCall(const UtlString& sAbstractCallId);

   /**
   * Deletes all calls on the stack, freeing any call resources. Doesn't properly terminate
   * the calls.
   */
   void deleteAllCalls();

   /**
   * Deletes all conferences on the stack, freeing any call resources. Doesn't properly terminate
   * the conferences.
   */
   void deleteAllConferences();

   /** Gains focus for given call, defocusing old focused call. */
   OsStatus doGainFocus(const UtlString& sAbstractCallId,
                        UtlBoolean bGainOnlyIfNoFocusedCall = FALSE);

   /**
   * Gains focus for next call, avoiding sAvoidAbstractCallId when looking for next call to focus.
   * If there is no other call than sAvoidAbstractCallId, then no focus is gained. Meant to be used
   * from doYieldFocus to gain next focus. Works only if no call has currently focus.
   */
   OsStatus doGainNextFocus(const UtlString& sAvoidAbstractCallId);

   /**
   * Defocuses given call if its focused. Shifts focus to next call if requested.
   * Has no effect if given call is not focused anymore.
   */
   OsStatus doYieldFocus(const UtlString& sAbstractCallId,
                         UtlBoolean bShiftFocus = TRUE);

   /** Defocuses current call in focus, and lets other call gain focus if requested */
   OsStatus doYieldFocus(UtlBoolean bShiftFocus = TRUE);

   /** Shuts down threads of all calls */
   void shutdownAllCallThreads();

   /** Shuts down threads of all conferences*/
   void shutdownAllConferenceThreads();

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /** gets total amount of calls. Also calls in conference are counted */
   int getCallCount() const;

   /** Gets ids of all calls. Ids are appended into list. */
   OsStatus getCallIds(UtlSList& callIdList) const;

   /** Gets ids of all conferences. Ids are appended into list. */
   OsStatus getConferenceIds(UtlSList& conferenceIdList) const;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   mutable OsMutex m_memberMutex; ///< mutex for member synchronization, delete guard.

   // not thread safe fields
   UtlHashMap m_callMap; ///< hashmap with calls
   UtlHashMap m_conferenceMap; ///< hashmap with conferences

   // focus
   mutable OsMutex m_focusMutex; ///< required for access to m_sAbstractCallInFocus
   UtlString m_sAbstractCallInFocus; ///< holds id of call currently in focus.
};

#endif // XCpGenericCallStack_h__
