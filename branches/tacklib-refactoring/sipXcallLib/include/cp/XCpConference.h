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

#ifndef XCpConference_h__
#define XCpConference_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <cp/XCpAbstractCall.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
 * XCpConference wraps several XSipConnections realizing conference functionality.
 */
class XCpConference : public XCpAbstractCall
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   XCpConference(const UtlString& sId);

   virtual ~XCpConference();

   /* ============================ MANIPULATORS ============================== */

   /** Connects call to given address. Uses supplied sip call-id. */
   virtual OsStatus connect(const UtlString& sSipCallId,
                            const UtlString& toAddress,
                            const UtlString& lineURI,
                            const UtlString& locationHeader,
                            CP_CONTACT_ID contactId);

   /**
    * Always fails, as conference cannot accept inbound call. Instead, add calls
    * to conference.
    */
   virtual OsStatus acceptConnection(const UtlString& locationHeader,
                                     CP_CONTACT_ID contactId);

   /**
    * Always fails, as conference cannot reject inbound connections.
    */
   virtual OsStatus rejectConnection();

   /**
   * Always fails, as conference cannot redirect inbound connections.
   */
   virtual OsStatus redirectConnection(const UtlString& sRedirectSipUri);

   /**
   * Always fails, as conference cannot answer inbound connections.
   */
   virtual OsStatus answerConnection();

   /**
   * Disconnects given call with given sip call-id
   *
   * The appropriate disconnect signal is sent (e.g. with SIP BYE or CANCEL).  The connection state
   * progresses to disconnected and the connection is removed.
   */
   virtual OsStatus dropConnection(const UtlString& sSipCallId,
                                   const UtlString& sLocalTag,
                                   const UtlString& sRemoteTag);

   /** Disconnects all calls */
   OsStatus dropAllConnections();

   /** Sends an INFO message to the other party(s) on the call */
   virtual OsStatus sendInfo(const UtlString& sSipCallId,
                             const UtlString& sLocalTag,
                             const UtlString& sRemoteTag,
                             const UtlString& sContentType,
                             const UtlString& sContentEncoding,
                             const UtlString& sContent);

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /**
   * Checks if this conference has given sip dialog.
   */
   virtual UtlBoolean hasSipDialog(const UtlString& sSipCallId,
                                   const UtlString& sLocalTag = NULL,
                                   const UtlString& sRemoteTag = NULL) const;

   /** Gets the number of sip connections in this call */
   virtual int getCallCount() const;

   /** Gets sip call-id of conference if its available */
   OsStatus getConferenceSipCallIds(UtlSList& sipCallIdList) const;

   /** Gets audio energy levels for call */
   virtual OsStatus getAudioEnergyLevels(int& iInputEnergyLevel,
                                         int& iOutputEnergyLevel) const;

   /** Gets remote user agent for call or conference */
   virtual OsStatus getRemoteUserAgent(const UtlString& sSipCallId,
                                       const UtlString& sLocalTag,
                                       const UtlString& sRemoteTag,
                                       UtlString& userAgent) const;

   /** Gets internal id of media connection for given call or conference. Only for unit tests */
   virtual OsStatus getMediaConnectionId(int& mediaConnID) const;

   /** Gets copy of SipDialog for given call */
   virtual OsStatus getSipDialog(const UtlString& sSipCallId,
                                 const UtlString& sLocalTag,
                                 const UtlString& sRemoteTag,
                                 SipDialog& dialog) const;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   XCpConference(const XCpConference& rhs);

   XCpConference& operator=(const XCpConference& rhs);

};

#endif // XCpConference_h__
