//
// Copyright (C) 2005, 2006 SIPez LLC
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2005, 2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2007 Pingtel Corp., certain elements licensed under a Contributor Agreement.  
// Contributors retain copyright to elements licensed under a Contributor Agreement.
// Licensed to the User under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

// Author: Dan Petrie (dpetrie AT SIPez DOT com)

#ifndef _SipMessage_h_
#define _SipMessage_h_

// SYSTEM INCLUDES
//#include <...>

// APPLICATION INCLUDES
#include <utl/UtlHashBag.h>
#include <os/OsSocket.h>
#include <net/HttpMessage.h>
#include <net/SdpBody.h>
#include <sdp/SdpCodec.h>
#include <net/Url.h>
#include <net/SmimeBody.h>

class UtlHashMap;
class SipUserAgent;
class SipRegInfoBody;        // for RFC 3680


// DEFINES

// SIP extensions
#define SIP_CALL_CONTROL_EXTENSION "sip-cc"
#define SIP_SESSION_TIMER_EXTENSION "timer"
#define SIP_REPLACES_EXTENSION "replaces"
#define SIP_JOIN_EXTENSION "join"
#define SIP_PRACK_EXTENSION "100rel" // rfc3262
#define SIP_FROM_CHANGE_EXTENSION "from-change" // must not appear in Require: header
#define SIP_NO_REFER_SUB_EXTENSION "norefersub" // rfc4488

// SIP Methods
#define SIP_INVITE_METHOD "INVITE"
#define SIP_UPDATE_METHOD "UPDATE"
#define SIP_ACK_METHOD "ACK"
#define SIP_BYE_METHOD "BYE"
#define SIP_CANCEL_METHOD "CANCEL"
#define SIP_INFO_METHOD "INFO"
#define SIP_NOTIFY_METHOD "NOTIFY"
#define SIP_OPTIONS_METHOD "OPTIONS"
#define SIP_REFER_METHOD "REFER"
#define SIP_REGISTER_METHOD "REGISTER"
#define SIP_SUBSCRIBE_METHOD "SUBSCRIBE"
#define SIP_PRACK_METHOD "PRACK"


//Simple Methods
#define SIP_MESSAGE_METHOD "MESSAGE"
#define SIP_DO_METHOD "DO"
#define SIP_PUBLISH_METHOD "PUBLISH"

// SIP Fields
#define SIP_ACCEPT_FIELD "ACCEPT"
#define SIP_ACCEPT_ENCODING_FIELD HTTP_ACCEPT_ENCODING_FIELD
#define SIP_ACCEPT_LANGUAGE_FIELD HTTP_ACCEPT_LANGUAGE_FIELD
#define SIP_ALLOW_FIELD "ALLOW"
#define SIP_ALSO_FIELD "ALSO"
#define SIP_CALLID_FIELD "CALL-ID"
#define SIP_CONFIG_ALLOW_FIELD "CONFIG_ALLOW"
#define SIP_CONFIG_REQUIRE_FIELD "CONFIG_REQUIRE"
#define SIP_SHORT_CALLID_FIELD "i"
#define SIP_CONTACT_FIELD "CONTACT"
#define SIP_SHORT_CONTACT_FIELD "m"
#define SIP_CONTENT_LENGTH_FIELD HTTP_CONTENT_LENGTH_FIELD
#define SIP_SHORT_CONTENT_LENGTH_FIELD "l"
#define SIP_CONTENT_TYPE_FIELD HTTP_CONTENT_TYPE_FIELD
#define SIP_SHORT_CONTENT_TYPE_FIELD "c"
#define SIP_CONTENT_ENCODING_FIELD "CONTENT-ENCODING"
#define SIP_SHORT_CONTENT_ENCODING_FIELD "e"
#define SIP_CSEQ_FIELD "CSEQ"
#define SIP_RSEQ_FIELD "RSEQ"
#define SIP_RACK_FIELD "RACK"
#define SIP_DIVERSION_FIELD "DIVERSION"   // draft-levy-sip-diversion-08 Diversion header
#define SIP_EVENT_FIELD "EVENT"
#define SIP_SHORT_EVENT_FIELD "o"
#define SIP_EXPIRES_FIELD "EXPIRES"
#define SIP_FROM_FIELD "FROM"
#define SIP_IF_MATCH_FIELD "SIP-IF-MATCH"
#define SIP_SHORT_FROM_FIELD "f"
#define SIP_MAX_FORWARDS_FIELD "MAX-FORWARDS"
#define SIP_P_ASSERTED_IDENTITY_FIELD "P-ASSERTED-IDENTITY"
#define SIP_Q_FIELD "Q"
#define SIP_REASON_FIELD "REASON"          //  RFC 3326 Reason header
#define SIP_JOIN_FIELD "JOIN"          //  RFC 3911 Join header
#define SIP_RECORD_ROUTE_FIELD "RECORD-ROUTE"
#define SIP_REFER_TO_FIELD "REFER-TO"
#define SIP_REFER_SUB_FIELD "REFER-SUB"
#define SIP_SHORT_REFER_TO_FIELD "r"
#define SIP_REFERRED_BY_FIELD "REFERRED-BY"
#define SIP_SHORT_REFERRED_BY_FIELD "b"
#define SIP_REPLACES_FIELD "REPLACES"
#define SIP_RETRY_AFTER_FIELD "RETRY-AFTER"
#define SIP_REQUEST_DISPOSITION_FIELD "REQUEST-DISPOSITION"
#define SIP_REQUESTED_BY_FIELD "REQUESTED-BY"
#define SIP_REQUIRE_FIELD "REQUIRE"
#define SIP_PROXY_REQUIRE_FIELD "PROXY-REQUIRE"
#define SIP_ROUTE_FIELD "ROUTE"
#define SIP_SERVER_FIELD "SERVER"
#define SIP_SESSION_EXPIRES_FIELD "SESSION-EXPIRES"
#define SIP_SHORT_SESSION_EXPIRES_FIELD "x"
#define SIP_MIN_SE_FIELD "MIN-SE"
#define SIP_IF_MATCH_FIELD "SIP-IF-MATCH"
#define SIP_ETAG_FIELD "SIP-ETAG"
#define SIP_SUBJECT_FIELD "SUBJECT"
#define SIP_SHORT_SUBJECT_FIELD "s"
#define SIP_SUBSCRIPTION_STATE_FIELD "SUBSCRIPTION-STATE"
#define SIP_SUPPORTED_FIELD "SUPPORTED"
#define SIP_SHORT_SUPPORTED_FIELD "k"
#define SIP_TO_FIELD "TO"
#define SIP_SHORT_TO_FIELD "t"
#define SIP_UNSUPPORTED_FIELD "UNSUPPORTED"
#define SIP_USER_AGENT_FIELD HTTP_USER_AGENT_FIELD
#define SIP_VIA_FIELD "VIA"
#define SIP_SHORT_VIA_FIELD "v"
#define SIP_WARNING_FIELD "WARNING"
#define SIP_MIN_EXPIRES_FIELD "MIN-EXPIRES"

///custom fields
#define SIPX_IMPLIED_SUB "sipx-implied" ///< integer expiration duration for subscription
// Response codes and text
#define SIP_1XX_CLASS_CODE 100

#define SIP_TRYING_CODE 100
#define SIP_TRYING_TEXT "Trying"

#define SIP_RINGING_CODE 180
#define SIP_RINGING_TEXT "Ringing"

#define SIP_CALL_BEING_FORWARDED_CODE 181
#define SIP_CALL_BEING_FORWARDED_TEXT "Call Is Being Forwarded"

#define SIP_QUEUED_CODE 182
#define SIP_QUEUED_TEXT "Queued"

#define SIP_SESSION_PROGRESS_CODE 183
#define SIP_SESSION_PROGRESS_TEXT "Session Progress"

#define SIP_2XX_CLASS_CODE 200

#define SIP_OK_CODE 200
#define SIP_OK_TEXT "OK"

#define SIP_ACCEPTED_CODE 202
#define SIP_ACCEPTED_TEXT "Accepted"

#define SIP_3XX_CLASS_CODE 300

#define SIP_MULTI_CHOICE_CODE 300
#define SIP_MULTI_CHOICE_TEXT "Multiple Choices"

#define SIP_PERMANENT_MOVE_CODE 301
#define SIP_PERMANENT_MOVE_TEXT "Moved Permanently"

#define SIP_TEMPORARY_MOVE_CODE 302
#define SIP_TEMPORARY_MOVE_TEXT "Moved Temporarily"

#define SIP_USE_PROXY_CODE 305
#define SIP_USE_PROXY_TXT "Use Proxy"

#define SIP_ALTERNATIVE_SERVICE_CODE 380
#define SIP_ALTERNATIVE_SERVICE_TXT "Alternative Service"

#define SIP_4XX_CLASS_CODE 400

#define SIP_BAD_REQUEST_CODE 400
#define SIP_BAD_REQUEST_TEXT "Bad Request"

#define SIP_UNAUTHORIZED_CODE 401
#define SIP_UNAUTHORIZED_TEXT "Unauthorized"

#define SIP_PAYMENT_REQUIRED_CODE 402
#define SIP_PAYMENT_REQUIRED_TEXT "Payment Required"

#define SIP_FORBIDDEN_CODE 403
#define SIP_FORBIDDEN_TEXT "Forbidden"

#define SIP_NOT_FOUND_CODE 404
#define SIP_NOT_FOUND_TEXT "Not Found"

#define SIP_BAD_METHOD_CODE 405
#define SIP_BAD_METHOD_TEXT "Method Not Allowed"

#define SIP_NOT_ACCEPTABLE_CODE 406
#define SIP_NOT_ACCEPTABLE_TEXT "Not Acceptable"

#define SIP_PROXY_AUTH_REQUIRED_CODE 407
#define SIP_PROXY_AUTH_REQUIRED_TEXT "Proxy Authentication Required"

#define SIP_REQUEST_TIMEOUT_CODE 408
#define SIP_REQUEST_TIMEOUT_TEXT "Request Timeout"

#define SIP_GONE_CODE 410
#define SIP_GONE_TEXT "Gone"

#define SIP_CONDITIONAL_REQUEST_FAILED_CODE 412
#define SIP_CONDITIONAL_REQUEST_FAILED_TEXT "Conditional Request Failed"

#define SIP_BAD_MEDIA_CODE 415
#define SIP_BAD_MEDIA_TEXT "Unsupported Media Type or Content Encoding"

#define SIP_UNSUPPORTED_URI_SCHEME_CODE 416
#define SIP_UNSUPPORTED_URI_SCHEME_TEXT "Unsupported URI Scheme"

#define SIP_BAD_EXTENSION_CODE 420
#define SIP_BAD_EXTENSION_TEXT "Extension Not Supported"

#define SIP_SMALL_SESSION_INTERVAL_CODE 422
#define SIP_SMALL_SESSION_INTERVAL_TEXT "Session Interval Too Small"

#define SIP_TOO_BRIEF_CODE 423
#define SIP_TOO_BRIEF_TEXT "Registration Too Brief"
#define SIP_SUB_TOO_BRIEF_TEXT "Subscription Too Brief"

#define SIP_TEMPORARILY_UNAVAILABLE_CODE 480
#define SIP_TEMPORARILY_UNAVAILABLE_TEXT "Temporarily Unavailable"

#define SIP_BAD_TRANSACTION_CODE 481
#define SIP_BAD_TRANSACTION_TEXT "Call/Transaction Does Not Exist"

#define SIP_LOOP_DETECTED_CODE 482
#define SIP_LOOP_DETECTED_TEXT "Loop Detected"

#define SIP_TOO_MANY_HOPS_CODE 483
#define SIP_TOO_MANY_HOPS_TEXT "Too many hops"

#define SIP_BAD_ADDRESS_CODE 484
#define SIP_BAD_ADDRESS_TEXT "Address Incomplete"

#define SIP_AMBIGUOUS_CODE 485
#define SIP_AMBIGUOUS_TEXT "Ambiguous"

#define SIP_BUSY_CODE 486
#define SIP_BUSY_TEXT "Busy Here"

#define SIP_REQUEST_TERMINATED_CODE 487
#define SIP_REQUEST_TERMINATED_TEXT "Request Terminated"

#define SIP_REQUEST_NOT_ACCEPTABLE_HERE_CODE 488
#define SIP_REQUEST_NOT_ACCEPTABLE_HERE_TEXT "Not Acceptable Here"

#define SIP_BAD_EVENT_CODE 489
#define SIP_BAD_EVENT_TEXT "Requested Event Type Is Not Supported"

#define SIP_REQUEST_PENDING_CODE 491
#define SIP_REQUEST_PENDING_TEXT "Request Pending"   

#define SIP_REQUEST_UNDECIPHERABLE_CODE 493
#define SIP_REQUEST_UNDECIPHERABLE_TEXT "Request Contained an Undecipherable S/MIME body"

#define SIP_5XX_CLASS_CODE 500

#define SIP_SERVER_INTERNAL_ERROR_CODE 500
#define SIP_SERVER_INTERNAL_ERROR_TEXT "Internal Server Error"

#define SIP_UNIMPLEMENTED_METHOD_CODE 501
#define SIP_UNIMPLEMENTED_METHOD_TEXT "Not Implemented"

#define SIP_BAD_GATEWAY_CODE 502
#define SIP_BAD_GATEWAY_TEXT "Bad Gateway"

#define SIP_SERVICE_UNAVAILABLE_CODE 503
#define SIP_SERVICE_UNAVAILABLE_TEXT "Service Unavailable"

#define SIP_SERVER_TIMEOUT_CODE 504
#define SIP_SERVER_TIMEOUT_TEXT "Server Timeout"

#define SIP_BAD_VERSION_CODE 505
#define SIP_BAD_VERSION_TEXT "Version Not Supported"

#define SIP_MESSAGE_TOO_LARGE_CODE 513
#define SIP_MESSAGE_TOO_LARGE_TEXT "Message Too Large"

#define SIP_PRECONDITION_FAILURE_CODE 580
#define SIP_PRECONDITION_FAILURE_TEXT "Precondition Failure"

#define SIP_6XX_CLASS_CODE 600

#define SIP_GLOBAL_BUSY_CODE 600
#define SIP_GLOBAL_BUSY_TEXT "Busy Everywhere"

#define SIP_DOESNT_EXIST_ANYWHERE_CODE 604
#define SIP_DOESNT_EXIST_ANYWHERE_TEXT "Does Not Exist Anywhere"

#define SIP_DECLINE_CODE 603
#define SIP_DECLINE_TEXT "Declined"

#define SIP_GLOBAL_NOT_ACCEPTABLE_CODE 606
#define SIP_GLOBAL_NOT_ACCEPTABLE_TEXT "Not Acceptable"

// there is no class 7 code, this is only end marker
#define SIP_7XX_CLASS_CODE 700

// Warning codes
#define SIP_WARN_MEDIA_NAVAIL_CODE 304
#define SIP_WARN_MEDIA_NAVAIL_TEXT "Media type not available"
#define SIP_WARN_MEDIA_INCOMPAT_CODE 305
#define SIP_WARN_MEDIA_INCOMPAT_TEXT "Insufficient compatible media types"
#define SIP_WARN_MISC_CODE 399

// Transport stuff
#define SIP_PORT 5060
#define SIP_TLS_PORT 5061
#define SIP_PROTOCOL_VERSION "SIP/2.0"
#define SIP_SUBFIELD_SEPARATOR " "
#define SIP_SUBFIELD_SEPARATORS "\t "
#define SIP_MULTIFIELD_SEPARATOR ","
#define SIP_SINGLE_SPACE " "
#define SIP_MULTIFIELD_SEPARATORS "\t ,"
#define SIP_TRANSPORT "transport"
#define SIP_TRANSPORT_UDP_STR "UDP"
#define SIP_TRANSPORT_TCP_STR "TCP"
#define SIP_TRANSPORT_TLS_STR "TLS"
#define SIP_URL_TYPE "SIP:"
#define SIPS_URL_TYPE "SIPS:"
#define SIP_DEFAULT_MAX_FORWARDS 70

// Caller preference request dispostions tokens
#define SIP_DISPOSITION_QUEUE "QUEUE"

// NOTIFY method event types
#define SIP_EVENT_MESSAGE_SUMMARY           "message-summary"
#define SIP_EVENT_SIMPLE_MESSAGE_SUMMARY    "simple-message-summary"
#define SIP_EVENT_CHECK_SYNC                "check-sync"
#define SIP_EVENT_REFER                     "refer"
#define SIP_EVENT_CONFIG                    "sip-config"
#define SIP_EVENT_UA_PROFILE                "ua-profile"
#define SIP_EVENT_REGISTER                  "reg" //  RFC 3680 
#define SIP_EVENT_PRESENCE                  "presence"

// NOTIFY Subscription-State values
#define SIP_SUBSCRIPTION_ACTIVE             "active"
#define SIP_SUBSCRIPTION_PENDING            "pending"
#define SIP_SUBSCRIPTION_TERMINATED         "terminated"

// The following are used for the REFER NOTIFY message body contents
#define CONTENT_TYPE_SIP_APPLICATION        "application/sip"
#define CONTENT_TYPE_MESSAGE_SIPFRAG        "message/sipfrag"
#define CONTENT_TYPE_SIMPLE_MESSAGE_SUMMARY "application/simple-message-summary"
#define CONTENT_TYPE_XPRESSA_SCRIPT         "text/xpressa-script"
#define CONTENT_TYPE_VQ_RTCP_XR             "application/vq-rtcpxr"

// Added for RFC 3680 
#define CONTENT_TYPE_REG_INFO 		"application/reg-info+xml"

#define SIP_REFER_SUCCESS_STATUS "SIP/2.0 200 OK\r\n"
#define SIP_REFER_FAILURE_STATUS "SIP/2.0 503 Service Unavailable\r\n"

//Added for Diversion header reason parameters

#define SIP_DIVERSION_UNKNOWN "unknown"
#define SIP_DIVERSION_BUSY "user-busy"
#define SIP_DIVERSION_UNAVAILABLE "unavailable"
#define SIP_DIVERSION_UNCONDITIONAL "unconditional"
#define SIP_DIVERSION_TIMEOFDAY "time-of-day"
#define SIP_DIVERSION_DND "do-not-disturb"
#define SIP_DIVERSION_DEFLECTION "deflection"
#define SIP_DIVERSION_OTOFSERVICE "out-of-service"
#define SIP_DIVERSION_FOLLOWME "follow-me"
#define SIP_DIVERSION_AWAY "away"

// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class SipTransaction;

/// Specialization of HttpMessage to contain and manipulate SIP messages
/**
 * @see HttpMessage for the descriptions of the general constructs
 * manipulators and accessors for the three basic parts of a SIP
 * message.  A message can be queried as to whether it is a request or a
 * response via the isResponse method.
 *
 * @nosubgrouping
 */
class SipMessage : public HttpMessage, public ISmimeNotifySink
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   // See sipXcall's CpCallManager for more defines

   enum EventSubTypes
   {
       NET_UNSPECIFIED = 0,
       NET_SIP_MESSAGE
   };

/* ============================ CREATORS ================================== */

    //! Construct from a buffer
    SipMessage(const char* messageBytes = NULL,
               int byteCount = -1);

    //! Construct from bytes read from a socket
    SipMessage(OsSocket* inSocket,
              int bufferSize = HTTP_DEFAULT_SOCKET_BUFFER_SIZE);

    //! Copy constructor
    SipMessage(const SipMessage& rSipMessage);

    //! Assignment operator
    SipMessage& operator=(const SipMessage& rhs);

    virtual
    ~SipMessage();
    //:Destructor

/* ============================ MANIPULATORS ============================== */

    static UtlBoolean getShortName( const char* longFieldName,
                                   UtlString* shortFieldName );

    static UtlBoolean getLongName( const char* shortFieldName,
                                  UtlString* longFieldName );

    void replaceShortFieldNames();

    void replaceLongFieldNames();

/* ============================ ACCESSORS ================================= */

    //! @name SIP URL manipulators
    //@{
    static void buildSipUrl(UtlString* url, const char* address,
                            int port = PORT_NONE,
                            const char* protocol = NULL,
                            const char* user = NULL,
                            const char* userLabel = NULL,
                            const char* tag = NULL);

    static void buildReplacesField(UtlString& replacesField,
                                   const char* callId,
                                   const char* fromField,
                                   const char* toFIeld);

    static UtlBoolean parseParameterFromUri(const char* uri,
                                           const char* parameterName,
                                           UtlString* parameterValue);

    static void setUriParameter(UtlString* uri, const char* parameterName,
                                const char* parameterValue);

    static void parseAddressFromUri(const char* uri, UtlString* address,
                                    int* port, UtlString* protocol,
                                    UtlString* user = NULL,
                                    UtlString* userLabel = NULL,
                                    UtlString* tag = NULL);

    static void ParseContactFields(const SipMessage *sipResponse,
                                   const SipMessage* ipRequest,
                                   const UtlString& subFieldName,
                                   int& subFieldValue);

    static void setUriTag(UtlString* uri, const char* tagValue);

    static UtlBoolean getViaTag(const char* viaField,
                           const char* tagName,
                           UtlString& tagValue);
    //@}

    //! @name SIP specific first header line accessors
    //@{
    void setSipRequestFirstHeaderLine(const char* method,
                                      const char* uri,
                                      const char* protocolVersion = SIP_PROTOCOL_VERSION);

    void changeUri(const char* uri);

    void getUri(UtlString* address, int* port,
                UtlString* protocol,
                UtlString* user = NULL) const;
    //@}


    //! @name Request builder methods
    /*! The following set of methods are used for building
     * requests in this message
     */
    //@{
    void setAckData(const char* uri,
                    const char* fromAddress,
                    const char* toAddress,
                    const char* callId,
                    int sequenceNumber = 1);

    void setAckData(const SipMessage* inviteResponse,
                    const SipMessage* inviteRequest,
                    const char* contactUri = NULL,
                    int sessionExpiresSeconds = 0);

    void setAckErrorData(const SipMessage* inviteResponse);

    void setByeData(const char* uri,
                    const char* fromAddress,
                    const char* toAddress,
                    const char* callId,
                    const char* localContact,
                    int sequenceNumber = 1);

    void setByeData(const SipMessage* inviteResponse,
                    const char * lastRespContact,
                    UtlBoolean byeToCallee,
                    int localSequenceNumber,
                    const char* routeField,
                    const char* alsoInviteUri,
                    const char* localContact);

    void setCancelData(const char* fromAddress, const char* toAddress,
                       const char* callId,
                       int sequenceNumber = 1,
                       const char* localContact = NULL);

    void setCancelData(const SipMessage* inviteResponse,
                       const char* localContact=NULL);

    void setPrackData(const char* fromAddress,
                      const char* toAddress,
                      const char* callId,
                      int sequenceNumber = 1,
                      int prackRSequenceNumber = 1,
                      int prackCSequenceNumber = 1,
                      const char* prackMethod = NULL,
                      const char* localContact=NULL);

    void setInviteData(const char* fromAddress,
                       const char* toAddress,
                       const char * farEndContact,
                       const char* contactUrl,
                       const char* callId,                       
                       int sequenceNumber = 1,
                       int sessionReinviteTimer = 0);

    void setInviteData(const SipMessage* previousInvite,
                       const char* newUri);

    void setReinviteData(SipMessage* invite,
                         const char* farEndContact,
                         const char* contactUrl,
                         const char* routeField,
                         int sequenceNumber,
                         int sessionReinviteTimer);

    void setOptionsData(const SipMessage* inviteRequest,
                        const char* LastRespContact,
                        UtlBoolean optionsToCallee,
                        int localCSequenceNumber,
                        const char* routeField,
                        const char* localContact);

    void setNotifyData(SipMessage *subscribeRequest,
                       int lastLocalSequenceNumber,
                       const char* route,
                       const char* stateField = NULL,
                       const char* eventField = NULL,
                       const char* id = NULL);

    void setNotifyData( const char* uri,
                        const char* fromField,
                        const char* toField,
                        const char* callId,
                        int lastNotifyCseq,
                        const char* eventtype,
                        const char* id,
                        const char* state,
                        const char* contact,
                        const char* routeField);

    void setSubscribeData( const char* uri,
                        const char* fromField,
                        const char* toField,
                        const char* callId,
                        int cseq,
                        const char* eventField,
                        const char* acceptField,
                        const char* id,
                        const char* contact,
                        const char* routeField,
                        int expiresInSeconds);

    void setEnrollmentData(const char* uri,
                           const char* fromField,
                           const char* toField,
                           const char* callId,
                           int CSequenceNum,
                           const char* contactUrl,
                           const char* protocolField,
                           const char* profileField,
                           int expiresInSeconds = -1);

    /* RFC 3428 - MWI*/
    void setMessageSummaryData(
                  UtlString& msgSummaryData,
                  const char* msgAccountUri,
                  UtlBoolean bNewMsgs=FALSE,
                  UtlBoolean bVoiceMsgs=FALSE,
                  UtlBoolean bFaxMsgs=FALSE,
                  UtlBoolean bEmailMsgs=FALSE,
                  int numNewMsgs=-1,
                  int numOldMsgs=-1,
                  int numFaxNewMsgs=-1,
                  int numFaxOldMsgs=-1,
                  int numEmailNewMsgs=-1,
                  int numEmailOldMsgs=-1);

    /* RFC 3428 - MWI */
    void setMWIData(const char *method,
				  const char* fromField,
                  const char* toField,
                  const char* uri,
                  const char* contactUrl,
                  const char* callId,
                  int CSeq,
                  UtlString bodyString);

    /* RFC 3680 - Registration event */
    void setRegInfoData(const char *method,
		    const char* fromField,
                  const char* toField,
                  const char* uri,
                  const char* contactUrl,
                  const char* callId,
                  int CSeq,
                  SipRegInfoBody& regInfoBody);

    void setVoicemailData(const char* fromField,
                           const char* toField,
                           const char* Uri,
                           const char* contactUrl,
                           const char* callId,
                           int CSeq,
                           int subscribePeriod = -1);


    void setReferData(const SipMessage* inviteResponse,
                    UtlBoolean byeToCallee,
                    int localSequenceNumber,
                    const char* routeField,
                    const char* contactUrl,
                    const char* remoteContactUrl,
                    const char* transferTargetAddress,
                    const char* targetCallId);

    void setRegisterData(const char* registererUri,
                         const char* registerAsUri,
                         const char* registrarServerUri,
                         const char* takeCallsAtUri,
                         const char* callId,
                         int sequenceNumber,
                         int expiresInSeconds = -1);

    void setRequestData(const char* method,
                        const char* uri,
                        const char* fromAddress,
                        const char* toAddress,
                        const char* callId,
                        int sequenceNumber = 1,
                        const char* contactUrl = NULL);

    //! Set a PUBLISH request
    void setPublishData( const char* uri,
                         const char* fromField,
                         const char* toField,
                         const char* callId,
                         int cseq,
                         const char* eventField,
                         const char* id,
                         const char* sipIfMatchField,
                         int expiresInSeconds,
                         const char* contact);
    /// Apply any header parameters in the request uri to the message
    void applyTargetUriHeaderParams();
    /**
     * There are some special rules implemented by this routine:
     *
     * - The header must not be forbidden by isUrlHeaderAllowed
     *
     * - If the header is a From, then any tag parameter on that
     *   new From header value is removed, and the tag parameter
     *   from the original From header is inserted.  The original
     *   From header value is inserted in an X-Original-From header.
     *
     * - If the header is a Route, it is forced to be a loose route
     *   and inserted as the topmost Route.
     *
     * - If it a unique header per isUrlHeaderUnique, then the new
     *   value replaces the old one.
     *
     * - otherwise, the new header is just added.to the message.
     */

    //@}

    void addSdpBody(int nRtpContacts,
                    UtlString hostAddresses[],
                    int rtpAudioPorts[],
                    int rtcpAudiopPorts[],
                    int rtpVideoPorts[],
                    int rtcpVideoPorts[],
                    RTP_TRANSPORT transportTypes[],
                    int numRtpCodecs,
                    SdpCodec* rtpCodecs[],
                    const SdpSrtpParameters& srtpParams,
                    int videoBandwidth,
                    int videoFramerate,
                    const SipMessage* pRequest = NULL,
                    const RTP_TRANSPORT rtpTransportOptions = RTP_TRANSPORT_UDP,
                    UtlBoolean bLocalHold = FALSE);

    void setSecurityAttributes(const SIPXTACK_SECURITY_ATTRIBUTES* const pSecurity);
    SIPXTACK_SECURITY_ATTRIBUTES* const getSecurityAttributes() const { return mpSecurity; } 
    bool fireSecurityEvent(const void* pEventData,
                           const enum SIP_SECURITY_EVENT,
                           const enum SIP_SECURITY_CAUSE,
                           SIPXTACK_SECURITY_ATTRIBUTES* const pSecurity,
                           void* pCert = NULL,
                           char* szSubjectAltName = NULL) const;
    bool smimeEncryptSdp(const void *pEventData) ;

    //! Accessor to get SDP body, optionally decrypting it if key info. is provided
    const SdpBody* getSdpBody(SIPXTACK_SECURITY_ATTRIBUTES* const pSecurity = NULL,
                              const void* pEventData = NULL) const;


    //! @name Response builders
    /*! The following methods are used to build responses
     * in this message
     */
    //@{
    void setResponseData(const SipMessage* request,
                        int responseCode,
                        const char* responseText,
                        const char* localContact = NULL);

    void setResponseData(int statusCode, const char* statusText,
                        const char* fromAddress,
                        const char* toAddress,
                        const char* callId,
                        int sequenceNumber,
                        const char* sequenceMethod,
                        const char* localContact = NULL);

    void setOkResponseData(const SipMessage* request,
                           const char* localContact = NULL);

    void setRequestTerminatedResponseData(const SipMessage* request);

    virtual void setRequestUnauthorized(const SipMessage* request,
                                const char* authenticationScheme,
                                const char* authenticationRealm,
                                const char* authenticationNonce = NULL,
                                const char* authenticationOpaque = NULL,
                                HttpEndpointEnum authEntity = SERVER);

    void setTryingResponseData(const SipMessage* request);

    void setInviteForbidden(const SipMessage* request);

    void setRequestBadMethod(const SipMessage* request,
                             const char* allowedMethods);

    void setRequestUnimplemented(const SipMessage* request);

    void setRequestBadExtension(const SipMessage* request,
                                const char* unsuportedExtensions);

    void setRequestBadAddress(const SipMessage* request);

    void setRequestBadVersion(const SipMessage* request);

    void setRequestBadRequest(const SipMessage* request);

    void setRequestBadUrlType(const SipMessage* request);

    void setRequestBadContentEncoding(const SipMessage* request,
                             const char* allowedEncodings);

    void setInviteRingingData(const char* fromAddress, const char* toAddress,
                              const char* callId,
                              int sequenceNumber = 1);

    void setInviteRingingData(const SipMessage* inviteRequest);

    void setQueuedResponseData(const SipMessage* inviteRequest);

    void setRequestPendingData(const SipMessage* inviteRequest);

    void setForwardResponseData(const SipMessage* inviteRequest,
                                const char* forwardAddress);

    void setInviteBusyData(const char* fromAddress, const char* toAddress,
                       const char* callId,
                       int sequenceNumber = 1);

    void setBadTransactionData(const SipMessage* inviteRequest);

    void setLoopDetectedData(const SipMessage* inviteRequest);

    void setInviteBusyData(const SipMessage* inviteRequest);

    void setInviteOkData(const SipMessage* inviteRequest,                         
                         int maxSessionExpiresSeconds,
                         const char* localContact = NULL);

    void setByeErrorData(const SipMessage* byeRequest);

    void setReferOkData(const SipMessage* referRequest);

    void setReferDeclinedData(const SipMessage* referRequest);

    void setReferFailedData(const SipMessage* referRequest);

    void setEventData(void* pEventData) { mpEventData = pEventData; }
    //@}


    //! @name Specialized header field accessors
    //@{
    UtlBoolean getFieldSubfield(const char* fieldName, int subfieldIndex,
                               UtlString* subfieldValue) const;

    UtlBoolean getContactUri(int addressIndex, UtlString* uri) const;

    UtlBoolean getContactUri(int addressIndex,
                             Url& contactField) const;

    UtlBoolean getContactField(int addressIndex,
                               UtlString& contactField) const;

    UtlBoolean getContactEntry(int addressIndex,
                              UtlString* uriAndParameters) const;

    UtlBoolean getContactAddress(int addressIndex,
                                UtlString* contactAddress,
                                int* contactPort,
                                UtlString* protocol,
                                UtlString* user = NULL,
                                UtlString* userLabel = NULL) const;

    void setViaFromRequest(const SipMessage* request);

    /// fills in parameters in topmost via based on actual received information.
    void setReceivedViaParams(const UtlString& fromIpAddress, ///< actual sender ip
                              int              fromPort       ///< actual sender port
                              );

    void addVia(const char* domainName,
                int port,
                const char* protocol,
                const char* branchId = NULL,
                const bool bIncludeRport = false,
                const char* customRouteId = NULL) ;

    void addViaField(const char* viaField, UtlBoolean afterOtherVias = TRUE);

    void setLastViaTag(const char* tagValue,
                       const char* tagName = "received");

    void setCallIdField(const char* callId = NULL);

    void setCSeqField(int sequenceNumber, const char* method);
    void setRSeqField(int sequenceNumber);
    void setRAckField(int rsequenceNumber, int csequenceNumber, const char* method);
    void incrementCSeqNumber();

    void setFromField(const char* fromField);

    void setFromField(const char* fromAddress, int fromPort,
                      const char* fromProtocol = NULL,
                      const char* fromUser = NULL,
                      const char* fromLabel = NULL);

    void setRawToField(const char* toField);

    void setRawFromField(const char* toField);

    void setToField(const char* toAddress, int toPort,
                    const char* fromProtocol = NULL,
                    const char* toUser = NULL,
                    const char* toLabel = NULL);
    void setToFieldTag(const char* tagValue);

    void setToFieldTag(int tagValue);

    void setExpiresField(int expiresInSeconds);

    void setMinExpiresField(int minimumExpiresInSeconds);

    void setContactField(const char* contactField, int index = 0);

    /**
     * Configures if contact may be replaced if a better one is found.
     */
    void allowContactOverride(UtlBoolean allowOverride = TRUE) { mbAllowContactOverride = allowOverride; }

    /**
     * When true we may replace contact with a better one if found.
     */
    UtlBoolean isContactOverrideAllowed() { return mbAllowContactOverride; }

    void setRequestDispositionField(const char* dispositionField);

    void addRequestDisposition(const char* dispositionToken);

    void setWarningField(int code, const char* hostname, const char* text);
    
    void getFromLabel(UtlString* fromLabel) const;

    void getToLabel(UtlString* toLabel) const;

    void getFromField(UtlString* fromField) const;

    void getFromFieldTag(UtlString& fromTag) const;

    void getFromUri(UtlString* uri) const;

    void getFromUrl(Url& url) const;

    void getFromAddress(UtlString* fromAddress, int* fromPort, UtlString* protocol,
                        UtlString* user = NULL, UtlString* userLabel = NULL,
                        UtlString* tag = NULL) const;

    //! Get the identity value from the P-Asserted-Identity header field
    /*! Get the identity from the index'th P-Asserted-Identity header
     *  field if it exists.
     *  \param identity - network asserted SIP identity (name-addr or
     *                  addr-spec format).  Use the Url class to parse the identity
     *  \param index - indicates which occurrance of P-Asserted-Identity header
     *                 to retrieve.
     *  \return TRUE/FALSE if the header of the given index exists
     */
    UtlBoolean getPAssertedIdentityField(UtlString& identity, int index = 0) const;

    //! Remove all of the P-Asserted-Identity header fields
    void removePAssertedIdentityFields();

    //! Add the P-Asserted-Identity value 
    void addPAssertedIdentityField(const UtlString& identity);

    UtlBoolean getResponseSendAddress(UtlString& address,
                                     int& port,
                                     UtlString& protocol) const;

    static void convertProtocolStringToEnum(const char* protocolString,
                        OsSocket::IpProtocolSocketType& protocolEnum);

    static void convertProtocolEnumToString(OsSocket::IpProtocolSocketType protocolEnum,
                                            UtlString& protocolString);

    UtlBoolean getWarningCode(int* warningCode, int index = 0) const;

    // Retrieves the index-th Via: header as it appears in the message,
    // but does not parse Via: headers for ",".
    // You probably want to use getViaFieldSubField().
    UtlBoolean getViaField(UtlString* viaField, int index) const;

    // Retrieves the index-th logical Via: header value, folding together
    // all the Via: headers and parsing ",".
    UtlBoolean getViaFieldSubField(UtlString* viaSubField, int subFieldIndex) const;

    void getLastVia(UtlString* viaAddress,
                    int* viaPort,
                    UtlString* protocol,
                    int* recievedPort = NULL,
                    UtlBoolean* receivedSet = NULL,
                    UtlBoolean* maddrSet = NULL,
                    UtlBoolean* receivedPortSet = NULL) const;

    UtlBoolean removeLastVia();

    void getToField(UtlString* toField) const;

    void getToFieldTag(UtlString& toTag) const;

    void getToUri(UtlString* uri) const;

    void getToUrl(Url& url) const;

    void getToAddress(UtlString* toAddress,
                      int* toPort,
                      UtlString* protocol,
                      UtlString* user = NULL,
                      UtlString* userLabel = NULL,
                      UtlString* tag = NULL) const;

    void getCallIdField(UtlString* callId) const;
    void getCallIdField(UtlString& callId) const;

    UtlBoolean getCSeqField(int* sequenceNum, UtlString* sequenceMethod = NULL) const;
    UtlBoolean getCSeqField(int& sequenceNum, UtlString& sequenceMethod) const;
    UtlBoolean getRSeqField(int& rsequenceNum) const;
    UtlBoolean getRAckField(int& rsequenceNum, int& csequenceNum, UtlString& sequenceMethod) const;

    UtlBoolean getRequireExtension(int extensionIndex, UtlString* extension) const;

    UtlBoolean getProxyRequireExtension(int extensionIndex, UtlString* extension) const;

    void addRequireExtension(const char* extension);

    UtlBoolean getContentEncodingField(UtlString* contentEncodingField) const;

    /// Retrieve the event type, id, and other params from the Event Header
    UtlBoolean getEventField(UtlString* eventType,
                             UtlString* eventId = NULL, //< set to the 'id' parameter value if not NULL
                             UtlHashMap* params = NULL  //< holds parameter name/value pairs if not NULL
                             ) const;

    UtlBoolean getEventField(UtlString& eventField) const;

    void setEventField(const char* eventField, const char* id = NULL);

    UtlBoolean getExpiresField(int* expiresInSeconds) const;

    UtlBoolean getRequestDispositionField(UtlString* dispositionField) const;

    UtlBoolean getRequestDisposition(int tokenIndex,
                                    UtlString* dispositionToken) const;

    /**
     * Sets Subscription-State header field value.
     */
    void setSubscriptionState(const UtlString& state,
                              const UtlString& reason = NULL,
                              int* expiresInSeconds = NULL,
                              int* retryAfterSeconds = NULL);

    /**
    * Gets Subscription-State header field value.
    *
    * Default value of integer values is -1. String values
    * will be empty if not present.
    */
    UtlBoolean getSubscriptionState(UtlString& state,
                                    UtlString& reason,
                                    int& expiresInSeconds,
                                    int& retryAfterSeconds) const;

    UtlBoolean getSessionExpires(int* sessionExpiresSeconds, UtlString* refresher) const;

    /**
     * Sets Session-Expires header field value. Setting generic parameters is not supported.
     *
     * @param sessionExpiresSeconds Value in seconds
     * @param refresher "uas" or "uac"
     */
    void setSessionExpires(int sessionExpiresSeconds, const UtlString& refresher = NULL);

    /**
     * Sets the value of Min-SE header field used in session timers.
     */
    void setMinSe(int minSe);

    /**
     * Gets the value of Min-SE header field used in session timers.
     */
    UtlBoolean getMinSe(int& minSe) const;

    /** Sets value of Retry-After field. */
    void setRetryAfterField(int periodSeconds);

    UtlBoolean getSupportedField(UtlString& supportedField) const;

    void setSupportedField(const char* supportedField);

    //! Test whether "token" is present in the Supported: header.
    UtlBoolean isInSupportedField(const char* token) const;

    //! Get the SIP-IF-MATCH field from the PUBLISH request
    UtlBoolean getSipIfMatchField(UtlString& sipIfMatchField) const;

    //! Set the SIP-IF-MATCH field for a PUBLISH request
    void setSipIfMatchField(const char* sipIfMatchField);

    //! Get the SIP-ETAG field from the response to a PUBLISH request
    UtlBoolean getSipETagField(UtlString& sipETagField) const;

    //! Set the SIP-ETAG field in a response to the PUBLISH request
    void setSipETagField(const char* sipETagField);

    const UtlString& getLocalIp() const;
    
    void setLocalIp(const UtlString& localIp);

    void setLocalIp(const SipMessage* pMsg) ;

    /**
     * Sets the transport that should be used for sending this message.
     */
    void setPreferredTransport(OsSocket::IpProtocolSocketType transport);

    /**
     * Gets the transport that should be used for sending this message.
     */
    OsSocket::IpProtocolSocketType getPreferredTransport() const;

    //@}

    //! @name SIP Routing header field accessors and manipulators
    //@{
    UtlBoolean getMaxForwards(int& maxForwards) const;

    void setMaxForwards(int maxForwards);

    void decrementMaxForwards();

    void setRecordRoutes(const SipMessage *inviteRequest);

    UtlBoolean getRecordRouteField(int index,
                                  UtlString* recordRouteField) const;

    UtlBoolean getRecordRouteUri(int index, UtlString* recordRouteUri) const;

    void setRecordRouteField(const char* recordRouteField, int index);

    void addRecordRouteUri(const char* recordRouteUri);

    // isClientMsgStrictRouted returns whether or not a message
    //    is set up such that it requires strict routing.
    //    This is appropriate only when acting as a UAC
    UtlBoolean isClientMsgStrictRouted() const;

    UtlBoolean getRouteField(UtlString* routeField) const;

    UtlBoolean getRouteUri(int index, UtlString* routeUri) const;

    void addRouteUri(const char* routeUri);

    void addLastRouteUri(const char* routeUri);

    UtlBoolean getLastRouteUri(UtlString& routeUri,
                              int& lastIndex);

    UtlBoolean removeRouteUri(int index, UtlString* routeUri);

    void setRouteField(const char* routeField);

    UtlBoolean buildRouteField(UtlString* routeField) const;

    /// Adjust route values as required when receiving at a proxy.
    void normalizeProxyRoutes(const SipUserAgent* sipUA, ///< used to check isMyHostAlias
                              Url& requestUri,           ///< returns normalized request uri
                              UtlSList* removedRoutes = NULL // route headers popped 
                              );
    /**<
     * Check the request URI and the topmost route
     *   - Detect and correct for any strict router upstream
     *     as specified by RFC 3261 section 16.4 Route Information Preprocessing
     *   - Pop off the topmost route until it is not me
     *  
     * If the removedRoutes is non-NULL, then any removed routes are returned
     *   on this list (in the order returned - topmost first) as UtlString objects.
     *   The caller is responsible for deleting these UtlStrings (a call to
     *   removedRoutes->destroyAll will delete them all).
     */

    //@}


    //! @name Call control header field accessors
    //@{
    //! Deprecated
    UtlBoolean getAlsoUri(int index, UtlString* alsoUri) const;
    //! Deprecated
    UtlBoolean getAlsoField(UtlString* alsoField) const;
    //! Deprecated
    void setAlsoField(const char* alsoField);
    //! Deprecated
    void addAlsoUri(const char* alsoUri);

    void setRequestedByField(const char* requestedByUri);

    UtlBoolean getRequestedByField(UtlString& requestedByField) const;

    void setReferToField(const char* referToField);

    UtlBoolean getReferToField(UtlString& referToField) const;

    UtlBoolean getReferSubField(UtlBoolean& referSubField) const;

    void setReferSubField(UtlBoolean referSubField);

    void setReferredByField(const char* referredByField);

    UtlBoolean getReferredByField(UtlString& referredByField) const;

    UtlBoolean getReferredByUrls(UtlString* referrerUrl = NULL,
                                 UtlString* referredToUrl = NULL) const;

    void setAllowField(const char* referToField);

    UtlBoolean getAllowField(UtlString& referToField) const;

    /** Sets Replaces field, used in INVITE */
    void setReplacesField(const char* replacesField);

    UtlBoolean getReplacesData(UtlString& callId,
                              UtlString& toTag,
                              UtlString& fromTag) const;

    /// @returns true if the message has either a User-Agent or Server header
    bool hasSelfHeader() const;

    // SERVER-header accessors
    void getServerField(UtlString* serverFieldValue) const;
    void setServerField(const char* serverField);
    void setAcceptField(const char* acceptField);
    void setAuthField(const char* authField);

    // RFC 3326 REASON-header
    void setReasonField(const char* reasonField);

    /**
     * Sets RFC 3326 REASON header. Only 1 reason can be specified.
     * Example:
     * Reason: SIP ;cause=200 ;text="Call completed elsewhere"
     */
    void setReasonField(const UtlString& protocol, int cause, const UtlString& text);

    /** Gets value of whole unparsed Reason: field.*/
    UtlBoolean getReasonField(UtlString& reasonField) const;

    /** 
     * Gets protocol, first cause and text values from Reason: header field.
     * Example:
     * Reason: SIP ;cause=600 ;text="Busy Everywhere"
     *
     * @param index - index of the "reason-value" from RFC3326 to parse. Normally 0 can be used.
     */
    UtlBoolean getReasonField(int index, UtlString& protocol, int& cause, UtlString& text) const;

    /**
     * Gets values of Join header field - call-id, from tag, to tag
     * Example:
     * Join: 12adf2f34456gs5;to-tag=12345;from-tag=54321
     */
    UtlBoolean getJoinField(UtlString& sipCallId, UtlString& fromTag, UtlString& toTag) const;

    /** Gets value of whole unparsed Join: field.*/
    UtlBoolean getJoinField(UtlString& joinField) const;

    /**
    * Sets value of Join header field - call-id, from tag, to tag
    */
    void setJoinField(const UtlString& sipCallId, const UtlString& fromTag, const UtlString& toTag);

    // Diversion-header
    void addDiversionField(const char* addr, const char* reasonParam,
    								UtlBoolean afterOtherDiversions=FALSE);

    void addDiversionField(const char* diversionField, UtlBoolean afterOtherDiversions=FALSE);

	
    UtlBoolean getLastDiversionField(UtlString& diversionField,int& lastIndex);

    UtlBoolean getDiversionField(int index, UtlString& diversionField);

    UtlBoolean getDiversionField(int index, UtlString& addr, UtlString& reasonParam);
	
    //@}

    // This method is needed to cover the symetrical situation which is
    // specific to SIP authorization (i.e. authentication and authorize
    // fields may be in either requests or responses
    UtlBoolean verifyMd5Authorization(const char* userId,
                                      const char* password,
                                      const char* nonce,
                                      const char* realm,
                                      const char* uri = NULL,
                                      HttpEndpointEnum authEntity = SERVER) const;

    //! @name DNS SRV state accessors
    /*! \note this will be deprecated
     */
    //@{
        //SDUA
    UtlBoolean getDNSField(UtlString * Protocol,
                          UtlString * Address,
                          UtlString * Port) const;
    void setDNSField( const char* Protocol,
                     const char* Address,
                     const char* Port);

    void clearDNSField();
    //@}


    //! Accessor to store transaction reference
    /*! \note the transaction may be NULL
     */
    void setTransaction(SipTransaction* transaction);

    //! Accessor to get transaction reference
    /*! \note the transaction may be NULL
     */
    SipTransaction* getSipTransaction() const;

    //! Accessor to retrieve any transport string from
    /*! \the to-field.
     */
    const UtlString getTransportName() const;       

    void setUseShortFieldNames(UtlBoolean bUseShortNames)
        { mbUseShortNames = bUseShortNames; } ; 

    UtlBoolean getUseShortFieldNames() const
        { return mbUseShortNames; } ;

/* ============================ INQUIRY =================================== */

    //! Returns TRUE if this a SIP response
    //! as opposed to a request.
    UtlBoolean isResponse() const;

    /** TRUE for 100rel responses */
    UtlBoolean is100RelResponse() const;

    UtlBoolean isRequest() const;

    /** TRUE if this message is PRACK request */
    UtlBoolean isPrackRequest() const;

    /** TRUE if this message is INVITE request */
    UtlBoolean isInviteRequest() const;

    /** TRUE if this message is CANCEL request */
    UtlBoolean isCancelRequest() const;

    /** TRUE if this message is BYE request */
    UtlBoolean isByeRequest() const;

    /** TRUE if this message is OPTIONS request */
    UtlBoolean isOptionsRequest() const;

    /** TRUE if this message is REFER request */
    UtlBoolean isReferRequest() const;

    /** TRUE if this message is UPDATE request */
    UtlBoolean isUpdateRequest() const;

    /** TRUE if this message is INFO request */
    UtlBoolean isInfoRequest() const;

    /** TRUE if this message is SUBSCRIBE request */
    UtlBoolean isSubscribeRequest() const;

    /** TRUE if this message is NOTIFY request */
    UtlBoolean isNotifyRequest() const;

    /** TRUE if this message is REGISTER request */
    UtlBoolean isRegisterRequest() const;

    /** TRUE if this message is ACK request */
    UtlBoolean isAckRequest() const;

    /** TRUE if this message belongs to INVITE dialog usage */
    UtlBoolean isInviteDialogUsage() const;

    /** TRUE if this message belongs to SUBSCRIBE dialog usage */
    UtlBoolean isSubscribeDialogUsage() const;

    /** TRUE if this message is target refresh request or response */
    UtlBoolean isTargetRefresh() const;

    //! @ Transaction and session related inquiry methods
    //@{
    UtlBoolean isSameMessage(const SipMessage* message,
                            UtlBoolean responseCodesMustMatch = FALSE) const;

    //! Is message part of a server or client transaction?
    /*! \param isOutgoing - the message is to be sent as opposed to received
     */
    UtlBoolean isServerTransaction(UtlBoolean isOutgoing) const;

    UtlBoolean isSameSession(const SipMessage* message) const;
    static UtlBoolean isSameSession(Url& previousUrl, Url& newUrl);
    UtlBoolean isResponseTo(const SipMessage* message) const;
    UtlBoolean isAckFor(const SipMessage* inviteResponse) const;
    
    //SDUA
    UtlBoolean isInviteFor(const SipMessage* inviteRequest) const;
    UtlBoolean isSameTransaction(const SipMessage* message) const;
    //@}

    //
    UtlBoolean isRequestDispositionSet(const char* dispositionToken) const;

    UtlBoolean isRequireExtensionSet(const char* extension) const;

    UtlBoolean isRecordRouteAccepted( void ) const;

    //! Is this a header parameter we want to allow users or apps. to
    //  pass through in the URL
    static UtlBoolean isUrlHeaderAllowed(const char*);

    //! Does this header allow multiple values, or only one.
    static UtlBoolean isUrlHeaderUnique(const char*);

    static void parseViaParameters( const char* viaField,
                                    UtlContainer& viaParameterList
                                   );
    // ISmimeNotifySink implementations                               
    void OnError(SIP_SECURITY_EVENT event, SIP_SECURITY_CAUSE cause);
    bool OnSignature(void* pCert, char* szSubjAltName);        
    /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

    SipTransaction* mpSipTransaction;
    mutable SIPXTACK_SECURITY_ATTRIBUTES* mpSecurity;
    mutable void* mpEventData;

    OsSocket::IpProtocolSocketType mPreferredTransport;

    UtlString mLocalIp;
    UtlBoolean mbUseShortNames;
    UtlBoolean mbAllowContactOverride; ///< when true then a better contact may be selected if found

    //SDUA
    UtlString m_dnsProtocol ;
    UtlString m_dnsAddress ;
    UtlString m_dnsPort ;

    // Class for the singleton object that carries the field properties
    class SipMessageFieldProps
       {
         public:

          SipMessageFieldProps();
          ~SipMessageFieldProps(); 

          UtlHashBag mShortFieldNames;
          UtlHashBag mLongFieldNames;
          // Headers that may not be referenced in a URI header parameter.
          UtlHashBag mDisallowedUrlHeaders;
          // Headers that do not take a list of values.
          UtlHashBag mUniqueUrlHeaders;

          void initNames();
          void initDisallowedUrlHeaders();
          void initUniqueUrlHeaders();
       };

    // Singleton object to carry the field properties.
    static SipMessageFieldProps sSipMessageFieldProps;
};

/* ============================ INLINE METHODS ============================ */

#endif  // _SipMessage_h_
