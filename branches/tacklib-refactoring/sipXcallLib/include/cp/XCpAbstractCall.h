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

#ifndef XCpAbstractCall_h__
#define XCpAbstractCall_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsDefs.h>
#include <os/OsMutex.h>
#include <os/OsSyncBase.h>
#include <os/OsServerTask.h>
#include <utl/UtlContainable.h>
#include <cp/CpDefs.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS
class SipDialog;

/**
 * XCpAbstractCall is the top class for XCpConference and XCpCall providing
 * common functionality. This class can be stored in Utl containers.
 * Inherits from OsSyncBase, and can be locked externally. Locking the object ensures
 * that its state doesn't change.
 *
 * Most public methods must acquire the object mutex first.
 */
class XCpAbstractCall : public OsServerTask, public UtlContainable, public OsSyncBase
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   static const UtlContainableType TYPE; /** < Class type used for runtime checking */ 

   /* ============================ CREATORS ================================== */

   XCpAbstractCall(const UtlString& sId);

   virtual ~XCpAbstractCall();

   /* ============================ MANIPULATORS ============================== */

   virtual UtlBoolean handleMessage(OsMsg& rRawMsg);

   /** Connects call to given address. Uses supplied sip call-id. */
   virtual OsStatus connect(const UtlString& sSipCallId,
                            const UtlString& toAddress,
                            const UtlString& lineURI,
                            const UtlString& locationHeader,
                            CP_CONTACT_ID contactId) = 0;

   /** 
   * Accepts inbound call connection. Inbound connections can only be part of XCpCall
   *
   * Progress the connection from the OFFERING state to the
   * RINGING state. This causes a SIP 180 Ringing provisional
   * response to be sent.
   */
   virtual OsStatus acceptConnection(const UtlString& locationHeader,
                                     CP_CONTACT_ID contactId) = 0;

   /**
   * Reject the incoming connection.
   *
   * Progress the connection from the OFFERING state to
   * the FAILED state with the cause of busy. With SIP this
   * causes a 486 Busy Here response to be sent.
   */
   virtual OsStatus rejectConnection() = 0;

   /**
   * Redirect the incoming connection.
   *
   * Progress the connection from the OFFERING state to
   * the FAILED state. This causes a SIP 302 Moved
   * Temporarily response to be sent with the specified
   * contact URI.
   */
   virtual OsStatus redirectConnection(const UtlString& sRedirectSipUri) = 0;

   /**
   * Answer the incoming terminal connection.
   *
   * Progress the connection from the OFFERING or RINGING state
   * to the ESTABLISHED state and also creating the terminal
   * connection (with SIP a 200 OK response is sent).
   */
   virtual OsStatus answerConnection() = 0;

   /**
    * Disconnects given call with given sip call-id
    *
    * The appropriate disconnect signal is sent (e.g. with SIP BYE or CANCEL).  The connection state
    * progresses to disconnected and the connection is removed.
    */
   virtual OsStatus dropConnection(const UtlString& sSipCallId,
                                   const UtlString& sLocalTag,
                                   const UtlString& sRemoteTag) = 0;

   /** Sends an INFO message to the other party(s) on the call */
   virtual OsStatus sendInfo(const UtlString& sSipCallId,
                             const UtlString& sLocalTag,
                             const UtlString& sRemoteTag,
                             const UtlString& sContentType,
                             const UtlString& sContentEncoding,
                             const UtlString& sContent) = 0;

   /** Block until the sync object is acquired or the timeout expires */
   virtual OsStatus acquire(const OsTime& rTimeout = OsTime::OS_INFINITY);

   /** Conditionally acquire the semaphore (i.e., don't block) */
   virtual OsStatus tryAcquire();

   /** Release the sync object */
   virtual OsStatus release();

   /* ============================ ACCESSORS ================================= */

   /**
   * Calculate a unique hash code for this object.  If the equals
   * operator returns true for another object, then both of those
   * objects must return the same hashcode.
   */    
   virtual unsigned hash() const;

   /**
   * Get the ContainableType for a UtlContainable derived class.
   */
   virtual UtlContainableType getContainableType() const;

   /**
    * Gets Id of the abstract call.
    */
   UtlString getId() const;

   /* ============================ INQUIRY =================================== */

   /**
   * Compare the this object to another like-objects.  Results for 
   * designating a non-like object are undefined.
   *
   * @returns 0 if equal, < 0 if less then and >0 if greater.
   */
   virtual int compareTo(UtlContainable const* inVal) const;

   /**
    * Checks if this abstract call has given sip dialog.
    */
   virtual UtlBoolean hasSipDialog(const UtlString& sSipCallId,
                                   const UtlString& sLocalTag = NULL,
                                   const UtlString& sRemoteTag = NULL) const = 0;

   /** Gets the number of sip connections in this call */
   virtual int getCallCount() const = 0;

   /** Gets audio energy levels for call */
   virtual OsStatus getAudioEnergyLevels(int& iInputEnergyLevel,
                                         int& iOutputEnergyLevel) const = 0;

   /** gets remote user agent for call or conference */
   virtual OsStatus getRemoteUserAgent(const UtlString& sSipCallId,
                                       const UtlString& sLocalTag,
                                       const UtlString& sRemoteTag,
                                       UtlString& userAgent) const = 0;

   /** Gets internal id of media connection for given call or conference. Only for unit tests */
   virtual OsStatus getMediaConnectionId(int& mediaConnID) const = 0;

   /** Gets copy of SipDialog for given call */
   virtual OsStatus getSipDialog(const UtlString& sSipCallId,
                                 const UtlString& sLocalTag,
                                 const UtlString& sRemoteTag,
                                 SipDialog& dialog) const = 0;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   XCpAbstractCall(const XCpAbstractCall& rhs);

   XCpAbstractCall& operator=(const XCpAbstractCall& rhs);

   static const int CALL_MAX_REQUEST_MSGS;

   mutable OsMutex m_memberMutex; ///< mutex for member synchronization

   const UtlString m_sId; ///< unique identifier of the abstract call
};

#endif // XCpAbstractCall_h__
