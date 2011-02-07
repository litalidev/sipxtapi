// 
// Copyright (C) 2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2004-2007 SIPfoundry Inc.
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

// SYSTEM INCLUDES
#include <stdlib.h>
#include <string.h>

//uncomment next line to track the create and destroy of messages
//#define TRACK_LIFE

// APPLICATION INCLUDES
#include <utl/UtlInit.h>
#include <utl/UtlDListIterator.h>
#include <utl/UtlHashBagIterator.h>
#include <utl/UtlHashMap.h>
#include <net/SipMessage.h>
#include <net/MimeBodyPart.h>
#include <net/SmimeBody.h>
#include <net/NameValueTokenizer.h>
#include <net/Url.h>
#include <net/SipUserAgent.h>
#include <os/OsDateTime.h>
#include <os/OsSysLog.h>
#include <net/HttpBody.h>
#include <os/OsDefs.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define MAXIMUM_INTEGER_STRING_LENGTH 20

// STATIC VARIABLES
SipMessage::SipMessageFieldProps SipMessage::sSipMessageFieldProps;

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
SipMessage::SipMessage(const char* messageBytes,
                       int byteCount)
: HttpMessage(messageBytes, byteCount)
, mpSecurity(NULL)
, mpEventData(NULL)
, mbAllowContactOverride(TRUE)
, mPreferredTransport(OsSocket::UNKNOWN)
{
   mbUseShortNames = false ;
   mLocalIp = "";
   mpSipTransaction = NULL;
   replaceShortFieldNames();

#ifdef TRACK_LIFE
   osPrintf("Created SipMessage @ address:%X\n",this);
#endif
}

SipMessage::SipMessage(OsSocket* inSocket, 
                       int bufferSize)
: HttpMessage(inSocket, bufferSize)
, mpSecurity(NULL)
, mpEventData(NULL)
, mbAllowContactOverride(TRUE)
, mPreferredTransport(OsSocket::UDP)
{
#ifdef TRACK_LIFE
   osPrintf("Created SipMessage @ address:%X\n",this);
#endif
   mbUseShortNames = false ;
   mpSipTransaction = NULL;
   replaceShortFieldNames();
}

// Copy constructor
SipMessage::SipMessage(const SipMessage& rSipMessage)
: HttpMessage(rSipMessage)
, mpSecurity(NULL)
, mbAllowContactOverride(rSipMessage.mbAllowContactOverride)
, mPreferredTransport(rSipMessage.mPreferredTransport)
{
#ifdef TRACK_LIFE
   osPrintf("Created SipMessage @ address:%X\n",this);
#endif
   replaceShortFieldNames();
   //SDUA
   mLocalIp = rSipMessage.mLocalIp;
   m_dnsProtocol = rSipMessage.m_dnsProtocol;
   m_dnsAddress = rSipMessage.m_dnsAddress;
   m_dnsPort = rSipMessage.m_dnsPort;
   mpSipTransaction = rSipMessage.mpSipTransaction;
   mbUseShortNames = rSipMessage.mbUseShortNames;
}

// Destructor
SipMessage::~SipMessage()
{
#ifdef TRACK_LIFE
   osPrintf("Deleted SipMessage from address :%X\n",this);
#endif
}

// Assignment operator
SipMessage&
SipMessage::operator=(const SipMessage& rSipMessage)
{
   HttpMessage::operator =((HttpMessage&)rSipMessage);
   if (this != &rSipMessage)
   {
      replaceShortFieldNames();
      mLocalIp = rSipMessage.mLocalIp;
      m_dnsProtocol = rSipMessage.m_dnsProtocol;
      m_dnsAddress = rSipMessage.m_dnsAddress;
      m_dnsPort = rSipMessage.m_dnsPort;
      mpSipTransaction = rSipMessage.mpSipTransaction;
      mbAllowContactOverride = rSipMessage.mbAllowContactOverride;
      mPreferredTransport = rSipMessage.mPreferredTransport;
      mbUseShortNames = rSipMessage.mbUseShortNames;
   }
   return *this;
}


/* ============================ MANIPULATORS ============================== */

UtlBoolean SipMessage::getShortName(const char* longFieldName,
                       UtlString* shortFieldName)
{
   NameValuePair longNV(longFieldName);
   UtlBoolean nameFound = FALSE;

   shortFieldName->remove(0);
   NameValuePair* shortNV = (NameValuePair*) sSipMessageFieldProps.mLongFieldNames.find(&longNV);
   if(shortNV)
   {
      shortFieldName->append(shortNV->getValue());
      nameFound = TRUE;
   }
   return(nameFound);
}

UtlBoolean SipMessage::getLongName(const char* shortFieldName,
                      UtlString* longFieldName)
{
   UtlBoolean nameFound = FALSE;

    // Short names are currently only 1 character long
    // If the short name is exactly 1 character long
    if(shortFieldName && shortFieldName[0] &&
        shortFieldName[1] == '\0')
    {
        UtlString shortNV(shortFieldName);
        // Convert to lower case
        shortNV.toLower();

       NameValuePair* longNV =
          (NameValuePair*) sSipMessageFieldProps.mShortFieldNames.find(&shortNV);
       if(longNV)
       {
          *longFieldName = longNV->getValue();
          nameFound = TRUE;
       }
       else
       {
           // Now try upper case
           shortNV.toUpper();
           longNV = (NameValuePair*) sSipMessageFieldProps.mShortFieldNames.find(&shortNV);
           if(longNV)
           {
              *longFieldName = longNV->getValue();
              nameFound = TRUE;
           }
       }
        // Optimization in favor of utility
        //else
        //{
        //    longFieldName->remove(0);
        //}
    }
   return(nameFound);
}

void SipMessage::replaceShortFieldNames()
{
   NameValuePair* nvPair;
   UtlString longName;
   size_t position;
   
   for ( position= 0;
         (nvPair = dynamic_cast<NameValuePair*>(mNameValues.at(position)));
         position++
        )
   {
      if(getLongName(nvPair->data(), &longName))
      {
         // There is a long form for this name, so replace it.
         mHeaderCacheClean = FALSE;
         NameValuePair* modified;

         /*
          * NOTE: the header name is the containable key, so we must remove the
          *       NameValuePair from the mNameValues list and then reinsert the
          *       modified version; you are not allowed to modify key values while
          *       an object is in a container.
          */
         modified = dynamic_cast<NameValuePair*>(mNameValues.removeAt(position));
         nvPair->remove(0);
         nvPair->append(longName);
         mNameValues.insertAt(position, modified);
      }
   }  
}

void SipMessage::replaceLongFieldNames()
{
   UtlDListIterator iterator(mNameValues);
   NameValuePair* nvPair;
   UtlString shortName;

   while ((nvPair = (NameValuePair*) iterator()))
   {
      if(getShortName(nvPair->data(), &shortName))
      {
         mHeaderCacheClean = FALSE;
         nvPair->remove(0);
         nvPair->append(shortName.data());
      }
   }
   mNameValues.rehash() ;
}

/* ============================ ACCESSORS ================================= */
void SipMessage::setSipRequestFirstHeaderLine(const char* method,
                                   const char* uri,
                                   const char* protocolVersion)
{
   Url tempRequestUri(uri, TRUE);
   UtlString strRequestUri;
   tempRequestUri.removeUrlParameter("method");
   tempRequestUri.removeAngleBrackets();
   tempRequestUri.getUri(strRequestUri);

   setRequestFirstHeaderLine(method, strRequestUri.data(), protocolVersion);
}

void SipMessage::setRegisterData(const char* registererUri,
                        const char* registerAsUri,
                        const char* registrarServerUri,
                        const char* takeCallsAtUri,
                        const char* callId,
                        int sequenceNumber,
                        int expiresInSeconds)
{
   setRequestData(SIP_REGISTER_METHOD,
                     registrarServerUri, // uri
                     registererUri, // from
                     registerAsUri, // to
                     callId,
                     sequenceNumber);

   setContactField(takeCallsAtUri);
   setExpiresField(expiresInSeconds);
}

void SipMessage::setReinviteData(SipMessage* invite,
                                 const char* farEndContact,
                                 const char* contactUrl,
                                 const char* routeField,
                                 int sequenceNumber,
                                 int sessionReinviteTimer)
{
    UtlString toField;
    UtlString fromField;
    UtlString callId;
    UtlString contactUri;
    UtlString lastResponseContact;

    setLocalIp(invite) ;

    // Get the to, from and callId fields
    if(invite->isFromThisSide())
    {
      invite->getToField(&toField);
      invite->getFromField(&fromField);
    }
    else// Reverse the to from, this invite came from the other side
    {
      invite->getToField(&fromField);
      invite->getFromField(&toField);
    }

   invite->getCallIdField(&callId);

   if (farEndContact)
      lastResponseContact.append(farEndContact);

   if (!invite->isFromThisSide() && lastResponseContact.isNull())
   {
      //if invite from other side and LastResponseContact is null because there has not been any
      //final responses from the other side yet ...check the otherside's invite request and get the
      //contact field from the request
       invite->getContactUri(0, &lastResponseContact);
    }

   setInviteData(fromField, 
         toField,
         lastResponseContact,
         contactUrl,
         callId,
         sequenceNumber,
         sessionReinviteTimer);

    setRouteField(routeField);
}

void SipMessage::setInviteData(const char* fromField,
                               const char* toField,
                               const char* farEndContact,
                               const char* contactUrl,
                               const char* callId,
                               int sequenceNumber,
                               int sessionReinviteTimer)
{
   UtlString bodyString;
   UtlString uri;

    Url toUrl(toField);
   // Create the top header line

   // If we have a contact for the other side use it
    // for the URI, otherwise use the To field
   if (farEndContact && *farEndContact)
   {
      uri = farEndContact;
   }
   else
    {
        // Clean out the header field parameters if they exist
        Url uriUrl(toUrl); // copy constructor is more efficient than parsing the toField string
        //uri.append(uriUrl.toString());
        uriUrl.removeHeaderParameters();
        uriUrl.getUri(uri);
    }

    // Check for header fields in the To URL
    UtlString headerName;
    UtlString headerValue;
    int headerIndex = 0;
    // Look through the headers and add them to the message
    while(toUrl.getHeaderParameter(headerIndex, headerName, headerValue))
    {
        // If the header is allowed to be passed through
        if(isUrlHeaderAllowed(headerName.data()))
        {
           if (isUrlHeaderUnique(headerName.data()))
           {
              // If the field exists, change it, if does not exist, create it.
              setHeaderValue(headerName.data(), headerValue.data(), 0);
           }
           else
           {
              addHeaderField(headerName.data(), headerValue.data());
           }
#ifdef TEST_PRINT
            osPrintf("SipMessage::setInviteData: name=%s, value=%s\n",
                    headerName.data(), headerValue.data());
#endif
        }
        else
        {
           OsSysLog::add(FAC_SIP, PRI_WARNING,
                         "SipMessage::setInviteData "
                         "URL header '%s: %s' may not be added using a header parameter",
                headerName.data(), headerValue.data());
        }

        headerIndex++;
    }

    // Remove the header fields from the URL as them
    // have been added to the message
    toUrl.removeHeaderParameters();
    UtlString toFieldString;
    toUrl.toString(toFieldString);

   setRequestData(SIP_INVITE_METHOD,
        uri, // URI
        fromField,
        toFieldString.data(),
        callId,
        sequenceNumber,
        contactUrl);

    // Set the session timer in seconds
    if(sessionReinviteTimer > 0)
        setSessionExpires(sessionReinviteTimer);

#ifdef TEST
   osPrintf("SipMessage::setInviteData rtpAddress: %s\n", rtpAddress);
#endif              
}

void SipMessage::addSdpBody(int nRtpContacts,
                            UtlString hostAddresses[], 
                            int rtpAudioPorts[], 
                            int rtcpAudioPorts[],
                            int rtpVideoPorts[], 
                            int rtcpVideoPorts[],
                            RTP_TRANSPORT transportTypes[],
                            int numRtpCodecs, 
                            SdpCodec* rtpCodecs[],
                            const SdpSrtpParameters& srtpParams,
                            int videoBandwidth,
                            int videoFramerate,
                            const SipMessage* pRequest,
                            RTP_TRANSPORT rtpTransportOptions,
                            UtlBoolean bLocalHold)
{
   if(numRtpCodecs > 0)
   {
      UtlString bodyString;
      int len;

      // Create and add the SDP body
      SdpBody* sdpBody = new SdpBody();
      sdpBody->setStandardHeaderFields("call",
                                       NULL,
                                       NULL,
                                       hostAddresses[0]); // Originator address
      
      if (pRequest && pRequest->getSdpBody())
      {
        sdpBody->addCodecsAnswer(nRtpContacts,
                                hostAddresses, 
                                rtpAudioPorts, 
                                rtcpAudioPorts, 
                                rtpVideoPorts, 
                                rtcpVideoPorts,
                                transportTypes,
                                numRtpCodecs, 
                                rtpCodecs,
                                srtpParams,
                                videoBandwidth,
                                videoFramerate,
                                pRequest->getSdpBody(),
                                bLocalHold);
      }
      else
      {
        sdpBody->addCodecsOffer(nRtpContacts,
                                hostAddresses, 
                                rtpAudioPorts, 
                                rtcpAudioPorts, 
                                rtpVideoPorts, 
                                rtcpVideoPorts,
                                transportTypes,
                                numRtpCodecs, 
                                rtpCodecs,
                                srtpParams,
                                videoBandwidth,
                                videoFramerate,
                                rtpTransportOptions,
                                bLocalHold);
      }

      setBody(sdpBody);

      // Add the content type for the body
      setContentType(SDP_CONTENT_TYPE);

      // Add the content length
      sdpBody->getBytes(&bodyString, &len);
      setContentLength(len);
   }
}

void SipMessage::setSecurityAttributes(const SIPXTACK_SECURITY_ATTRIBUTES* const pSecurity)
{
    mpSecurity = (SIPXTACK_SECURITY_ATTRIBUTES*)pSecurity;
}

const SdpBody* SipMessage::getSdpBody(SIPXTACK_SECURITY_ATTRIBUTES* const pSecurity,
                                      const void* pEventData) const
{
    if (pEventData)
    {
        mpEventData = (void*)pEventData;
    }
    const SdpBody* body = NULL;
    UtlString contentType;
    UtlString sdpType(SDP_CONTENT_TYPE);
    UtlString smimeType(CONTENT_SMIME_PKCS7);

    getContentType(&contentType);

    // Make them all lower case so they compare
    contentType.toLower();
    sdpType.toLower();
    smimeType.toLower();

    // If the body is of SDP type, return it
    body = dynamic_cast<const SdpBody*>(getBody());
    if (body)
    {
       return body;
    }
#if __SMIME
    // If we have a private key and this is a S/MIME body
    else if(pSecurity &&
            dynamic_cast<const SmimeBody*>(getBody()) )
    {
        if (getBody())
        {
            HttpBody* pBody = (HttpBody*)getBody();
            SmimeBody* smimeBody = dynamic_cast<SmimeBody*>(pBody);
            assert(smimeBody);

            // Try to decrypt if it has not already been decrypted
            if(! smimeBody->isDecrypted())
            {
                if (pSecurity)
                {
                    mpSecurity = pSecurity;
                }
                // Try to decrypt using the given private key pwd and signer cert.
                smimeBody->decrypt(NULL, 
                                0,
                                NULL,
                                pSecurity->szCertDbPassword,
                                pSecurity->szSmimeKeyDer,
                                pSecurity->nSmimeKeyLength,
                                (ISmimeNotifySink*)this);
            }

            // If it did not get encrypted, act like there is no SDP body
            if(smimeBody->isDecrypted())
            {
                const HttpBody* decryptedHttpBody = smimeBody->getDecryptedBody();
                // If the decrypted body is an SDP body type, use it
                if(strcmp(decryptedHttpBody->getContentType(), sdpType) == 0)
                {
                    body = (const SdpBody*) decryptedHttpBody;
                }
            }
            else
            {
                OsSysLog::add(FAC_SIP, PRI_WARNING, "Could not decrypt S/MIME body");
            }
        }
    }
#endif

    // Else if this is a multipart MIME body see
    // if there is an SDP part
    else
    {
        const HttpBody* multipartBody = getBody();
        if(multipartBody  && multipartBody->isMultipart())
        {
            int partIndex = 0;
            const HttpBody* bodyPart = NULL;
            while ((bodyPart = multipartBody->getMultipart(partIndex)))
            {
                if(strcmp(bodyPart->getContentType(), SDP_CONTENT_TYPE) == 0)
                {
                    // Temporarily disable while fixing multipart bodies
                    body = dynamic_cast<const SdpBody*>(bodyPart);
                    break;
                }
#ifdef SMIME
                // Check for S/MIME body
                else if(strcmp(bodyPart->getContentType(), smimeType) == 0 &&
                        pSecurity)
                {
                    SmimeBody* smimeBody = (SmimeBody*) bodyPart;

                    // Try to decrypt if it has not already been decrypted
                    if(! smimeBody->isDecrypted())
                    {
                        // Try to decrypt using the given private pwd and signer cert.
                        smimeBody->decrypt(NULL, 
                                        0,
                                        NULL,
                                        pSecurity->szCertDbPassword,
                                        pSecurity->szSmimeKeyDer,
                                        pSecurity->nSmimeKeyLength,
                                        (ISmimeNotifySink*)this);
                    }

                    // If it did not get encrypted, act like there is no SDP body
                    if(smimeBody->isDecrypted())
                    {
                        const HttpBody* decryptedHttpBody = smimeBody->getDecryptedBody();
                        // If the decrypted body is an SDP body type, use it
                        if(strcmp(decryptedHttpBody->getContentType(), sdpType) == 0)
                        {
                            body = (const SdpBody*) decryptedHttpBody;
                            break;
                        }
                    }
                    else
                    {
                        OsSysLog::add(FAC_SIP, PRI_WARNING, "Could not decrypt S/MIME body");
                    }
                }
#endif
                partIndex++ ;
            }
        }
    }

    return(body);
}

void SipMessage::setRequestData(const char* method, const char* uri,
                     const char* fromField, const char* toField,
                     const char* callId,
                     int sequenceNumber,
                            const char* contactUrl)
{
   // Create the top header line
   setSipRequestFirstHeaderLine(method, uri, SIP_PROTOCOL_VERSION);

   // Add the From field
   setRawFromField(fromField);

   // Add the to field
   setRawToField(toField);

   // Add the call-id field
   setCallIdField(callId);

   // Add the CSeq field
   setCSeqField(sequenceNumber, method);

    if(contactUrl && *contactUrl)
    {
        setContactField(contactUrl);
    }
}

void SipMessage::setResponseData(int statusCode, const char* statusText,
                     const char* fromField, const char* toField,
                     const char* callId,
                     int sequenceNumber, const char* sequenceMethod,
                     const char* localContact )
{
   // Create the top header line
   setResponseFirstHeaderLine(SIP_PROTOCOL_VERSION, statusCode,
      statusText);

   // Add the From field
   setRawFromField(fromField);

   // Add the to field
   setRawToField(toField);

   // Add the call-id field
   setCallIdField(callId);

   // Add the CSeq field
   if(sequenceNumber >= 0)
   {
      setCSeqField(sequenceNumber, sequenceMethod);
   }

   // Local Contact
   if (localContact)
   {
      setContactField(localContact) ;
   }
}

void SipMessage::setTryingResponseData(const SipMessage* request)
{
   setResponseData(request, SIP_TRYING_CODE, SIP_TRYING_TEXT);
}

void SipMessage::setInviteRingingData(const SipMessage* inviteRequest)
{
   setResponseData(inviteRequest, SIP_RINGING_CODE, SIP_RINGING_TEXT);
}

void SipMessage::setQueuedResponseData(const SipMessage* inviteRequest)
{
   setResponseData(inviteRequest, SIP_QUEUED_CODE, SIP_QUEUED_TEXT);
}

void SipMessage::setInviteBusyData(const char* fromField, const char* toField,
               const char* callId,
               int sequenceNumber)
{
      setResponseData(SIP_BUSY_CODE, SIP_BUSY_TEXT,
                     fromField, toField,
                     callId, sequenceNumber, SIP_INVITE_METHOD);
}

void SipMessage::setBadTransactionData(const SipMessage* inviteRequest)
{
   setResponseData(inviteRequest, SIP_BAD_TRANSACTION_CODE,
        SIP_BAD_TRANSACTION_TEXT);
}

void SipMessage::setLoopDetectedData(const SipMessage* inviteRequest)
{
   setResponseData(inviteRequest, SIP_LOOP_DETECTED_CODE,
        SIP_LOOP_DETECTED_TEXT);
}

void SipMessage::setInviteBusyData(const SipMessage* inviteRequest)
{
   UtlString fromField;
   UtlString toField;
   UtlString callId;
   int sequenceNum;
   UtlString sequenceMethod;

   setLocalIp(inviteRequest) ;

   inviteRequest->getFromField(&fromField);
   inviteRequest->getToField(&toField);
   inviteRequest->getCallIdField(&callId);
   inviteRequest->getCSeqField(&sequenceNum, &sequenceMethod);

   setInviteBusyData(fromField.data(), toField.data(),
      callId.data(), sequenceNum);

   setViaFromRequest(inviteRequest);
}

void SipMessage::setRequestPendingData(const SipMessage* inviteRequest)
{
   UtlString fromField;
   UtlString toField;
   UtlString callId;
   int sequenceNum;
   UtlString sequenceMethod;

   setLocalIp(inviteRequest) ;

   inviteRequest->getFromField(&fromField);
   inviteRequest->getToField(&toField);
   inviteRequest->getCallIdField(&callId);
   inviteRequest->getCSeqField(&sequenceNum, &sequenceMethod);

   setResponseData(SIP_REQUEST_PENDING_CODE, SIP_REQUEST_PENDING_TEXT,
           fromField, toField,
           callId, sequenceNum, SIP_INVITE_METHOD);

   setViaFromRequest(inviteRequest);
}

void SipMessage::setForwardResponseData(const SipMessage* request,
                     const char* forwardAddress)
{   
   setResponseData(request, SIP_TEMPORARY_MOVE_CODE, SIP_TEMPORARY_MOVE_TEXT);

   // Add the contact field for the forward address
   UtlString contactAddress;

   Url contactUrl(forwardAddress);
   contactUrl.removeFieldParameters();
   contactUrl.toString(contactAddress);
   setContactField(contactAddress.data());
}

void SipMessage::setInviteForbidden(const SipMessage* request)
{
   setResponseData(request, SIP_FORBIDDEN_CODE, SIP_FORBIDDEN_TEXT);
}

void SipMessage::setRequestBadMethod(const SipMessage* request,
                            const char* allowedMethods)
{
   setResponseData(request, SIP_BAD_METHOD_CODE, SIP_BAD_METHOD_TEXT);

   // Add a methods supported field
   addHeaderField(SIP_ALLOW_FIELD, allowedMethods);
}

void SipMessage::setRequestUnimplemented(const SipMessage* request)
{
   setResponseData(request, SIP_UNIMPLEMENTED_METHOD_CODE,
        SIP_UNIMPLEMENTED_METHOD_TEXT);
}

void SipMessage::setRequestBadExtension(const SipMessage* request,
                            const char* disallowedExtension)
{
   setResponseData(request, SIP_BAD_EXTENSION_CODE, SIP_BAD_EXTENSION_TEXT);

   // Add a methods supported field
   addHeaderField(SIP_UNSUPPORTED_FIELD, disallowedExtension);
}

void SipMessage::setRequestBadContentEncoding(const SipMessage* request,
                   const char* allowedEncodings)
{
   setResponseData(request, SIP_BAD_MEDIA_CODE, SIP_BAD_MEDIA_TEXT);

   // Add a encodings supported field
   addHeaderField(SIP_ACCEPT_ENCODING_FIELD, allowedEncodings);
   addHeaderField(SIP_ACCEPT_FIELD, "application/sdp");

   const char* explanation = "Content Encoding value not supported";
   setBody(new HttpBody(explanation, strlen(explanation), CONTENT_TYPE_TEXT_PLAIN));
}

void SipMessage::setRequestBadAddress(const SipMessage* request)
{
   setResponseData(request, SIP_BAD_ADDRESS_CODE, SIP_BAD_ADDRESS_TEXT);
}

void SipMessage::setRequestBadVersion(const SipMessage* request)
{
   setResponseData(request, SIP_BAD_VERSION_CODE, SIP_BAD_VERSION_TEXT);
}

void SipMessage::setRequestBadRequest(const SipMessage* request)
{
   setResponseData(request, SIP_BAD_REQUEST_CODE, SIP_BAD_REQUEST_TEXT);
}

void SipMessage::setRequestBadUrlType(const SipMessage* request)
{
   setResponseData(request, SIP_UNSUPPORTED_URI_SCHEME_CODE,
                   SIP_UNSUPPORTED_URI_SCHEME_TEXT);
}

void SipMessage::setInviteOkData(const SipMessage* inviteRequest,                                 
                int maxSessionExpiresSeconds, const char* localContact)
{
   UtlString fromField;
   UtlString toField;
   UtlString callId;
   int sequenceNum;
   UtlString sequenceMethod;
   const SdpBody* inviteSdp = NULL;

   setLocalIp(inviteRequest) ;

   inviteRequest->getFromField(&fromField);
   inviteRequest->getToField(&toField);
   inviteRequest->getCallIdField(&callId);
   inviteRequest->getCSeqField(&sequenceNum, &sequenceMethod);

   inviteSdp = inviteRequest->getSdpBody(mpSecurity);
   
   if (mpSecurity && !inviteSdp)
   {
       setResponseData(SIP_REQUEST_UNDECIPHERABLE_CODE,
                       SIP_REQUEST_UNDECIPHERABLE_TEXT,
                       fromField, toField, callId, sequenceNum,
                       SIP_INVITE_METHOD, localContact);
   }
   else
   {
        setResponseData(SIP_OK_CODE, SIP_OK_TEXT, fromField, toField,
                    callId, sequenceNum, SIP_INVITE_METHOD, localContact);

        setViaFromRequest(inviteRequest);

        UtlString recordRouteField;
        int recordRouteIndex = 0;
        while(inviteRequest->getRecordRouteField(recordRouteIndex,
                &recordRouteField))
        {
            setRecordRouteField(recordRouteField.data(), recordRouteIndex);
            recordRouteIndex++;
        }

        int inviteSessionExpires;
        UtlString refresher ;
        // If max session timer is less than the requested timer length
        // or if the other side did not request a timer, use
        // the max session timer
        if(!inviteRequest->getSessionExpires(&inviteSessionExpires, &refresher) ||
            (maxSessionExpiresSeconds > 0 &&
            inviteSessionExpires > maxSessionExpiresSeconds))
        {
            inviteSessionExpires = maxSessionExpiresSeconds;
        }
        if(inviteSessionExpires > 0)
        {
            setSessionExpires(inviteSessionExpires);
        }
   }
}

void SipMessage::setOkResponseData(const SipMessage* request,
                                   const char* localContact)
{
   setResponseData(request, SIP_OK_CODE, SIP_OK_TEXT, localContact);

   setRecordRoutes(request);
}

void SipMessage::setNotifyData(SipMessage *subscribeRequest,
                               int localCSequenceNumber,
                               const char* route,
                               const char* stateField,
                               const char* eventField,
                               const char* id)
{
   UtlString fromField;
   UtlString toField;
   UtlString uri;
   UtlString callId;
   int dummySequenceNum;
   UtlString sequenceMethod;

   setLocalIp(subscribeRequest) ;

   subscribeRequest->getFromField(&fromField);
   subscribeRequest->getToField(&toField);
   subscribeRequest->getCallIdField(&callId);
   subscribeRequest->getCSeqField(&dummySequenceNum, &sequenceMethod);

    // Set the NOTIFY event type
    if(eventField && *eventField)
    {
       UtlString eventHeaderValue(eventField);
       if (id && *id)
       {
          eventHeaderValue.append(";id=");
          eventHeaderValue.append(id);
       }
       setEventField(eventHeaderValue.data());
    }
    else
    {
        // Try to get it from the subscribe message
        UtlString subscribeEventField;
        subscribeRequest->getEventField(subscribeEventField);
        if(!subscribeEventField.isNull())
        {
            setEventField(subscribeEventField.data());
        }
    }

    // Set the Subscription-State header
    if(stateField && *stateField)
    {
      setHeaderValue(SIP_SUBSCRIPTION_STATE_FIELD, stateField);
    }
    else
    {
      setHeaderValue(SIP_SUBSCRIPTION_STATE_FIELD, SIP_SUBSCRIPTION_ACTIVE);
    }

    // Set the route for the NOTIFY request
    setRouteField(route);

    // Use contact if present
    if(subscribeRequest->getContactUri(0, &uri) && !uri.isNull())
    {

    }
    // Use the from field as we have nothing better to use
   else
   {
      uri.append(fromField.data());
   }

   setRequestData(SIP_NOTIFY_METHOD, uri.data(),
        toField.data(), fromField.data(), callId, localCSequenceNumber);
}

void SipMessage::setNotifyData(const char* uri,
                                const char* fromField,
                                const char* toField,
                                const char* callId,
                                int lastNotifyCseq,
                                const char* eventField,
                                const char* id,
                                const char* state,
                                const char* contact,
                                const char* routeField)
{
    // if uri is not set set it to the toField
    UtlString uriStr;
    if( uri && *uri )
    {
        uriStr.append(uri);
    }
    else if ( toField )
    {
        // check for toField  null
        uriStr.append( toField );
    }

    // if contact is not set set it to the fromField
    UtlString contactStr;
    if( contact && *contact )
    {
        contactStr.append(contact);
    }
    else if ( fromField )
    {
        // check for toField  null
        contactStr.append( fromField );
    }

    // Set the NOTIFY event type
    if( eventField && *eventField )
    {
        UtlString eventHeaderValue(eventField);
        if (id && *id)
        {
           eventHeaderValue.append(";id=");
           eventHeaderValue.append(id);
        }
        setEventField(eventHeaderValue.data());
    }

    // Set the Subscription-State header
    if(state && *state)
    {
      setHeaderValue(SIP_SUBSCRIPTION_STATE_FIELD, state);
    }
    else
    {
      // Force the caller to pass a valid subscription context -- add a default
      // param if needed...
      // setHeaderValue(SIP_SUBSCRIPTION_STATE_FIELD, SIP_SUBSCRIPTION_ACTIVE);
    }

    setRouteField(routeField );

   setRequestData(
        SIP_NOTIFY_METHOD,
        uriStr.data(),
        fromField,
        toField,
        callId,
        lastNotifyCseq,
        contactStr.data() );
}

void SipMessage::setSubscribeData(const char* uri,
                                const char* fromField,
                                const char* toField,
                                const char* callId,
                                int cseq,
                                const char* eventField,
                                const char* acceptField,
                                const char* id,
                                const char* contact,
                                const char* routeField,
                                int expiresInSeconds)
{
    Url toUrl(toField);
    Url fromUrl(fromField);
    toUrl.includeAngleBrackets();
    fromUrl.includeAngleBrackets();

    setRequestData(SIP_SUBSCRIBE_METHOD, uri,
                     fromUrl.toString(), toUrl.toString(),
                     callId,
                     cseq,
                     contact);

   // Set the event type, if any.
    if( eventField && *eventField )
    {
        UtlString eventHeaderValue(eventField);
        if (id && *id)
        {
           eventHeaderValue.append(";id=");
           eventHeaderValue.append(id);
        }
        setEventField(eventHeaderValue.data());
        setHeaderValue(SIP_EVENT_FIELD, eventHeaderValue, 0);
    }

   // Set the content type, if any.
    if( acceptField && *acceptField )
    {
        setHeaderValue(SIP_ACCEPT_FIELD, acceptField, 0);
    }

    // Set the route, if any.
    if (routeField && *routeField)
    {
        setRouteField(routeField);
    }
    
   //setExpires
   setExpiresField(expiresInSeconds);
}

void SipMessage::setEnrollmentData(const char* uri,
                       const char* fromField,
                       const char* toField,
                       const char* callId,
                       int CSeq,
                       const char* contactUrl,
                       const char* protocolField,
                       const char* profileField,
                       int expiresInSeconds)
{
    setRequestData(SIP_SUBSCRIBE_METHOD, uri,
                     fromField, toField,
                     callId,
                     CSeq,
                            contactUrl);

    // Set the event type
    setHeaderValue(SIP_EVENT_FIELD, SIP_EVENT_CONFIG, 0);

    // Set the protocols
    setHeaderValue(SIP_CONFIG_ALLOW_FIELD, protocolField, 0);

    // Set the profile
    setHeaderValue(SIP_CONFIG_REQUIRE_FIELD, profileField, 0);

   //setRxpires
   setExpiresField(expiresInSeconds);
}


// RFC 3248 MWI

/******** SAMPLE CODE FOR CREATING MWI NOTIFY message****************************

			SipMessage notifyRequest;
			UtlString fromField;
			UtlString toData;
			UtlString contactField;
			UtlString msgSummaryData;
			
			char *uri = "sip:100@127.0.0.1:3000";

			sipMessage->getFromField(&fromField);
			sipMessage->getToField(&toData);
			sipMessage->getContactField(0,contactField);
			
		       notifyRequest.setMessageSummaryData(msgSummaryData,"udit@3com.com",TRUE,
		       								TRUE,FALSE,FALSE,4,2);
			notifyRequest.setMWIData(SIP_NOTIFY_METHOD, fromField.data(), toData.data(),uri,
							contactField.data(),"1",1,msgSummaryData);			
			
**********************************************************************************/

void SipMessage::setMessageSummaryData(
                  UtlString& msgSummaryData,
                  const char* msgAccountUri,
                  UtlBoolean bNewMsgs,
                  UtlBoolean bVoiceMsgs,
                  UtlBoolean bFaxMsgs,
                  UtlBoolean bEmailMsgs,
                  int numNewMsgs,
                  int numOldMsgs,
                  int numFaxNewMsgs,
                  int numFaxOldMsgs,
                  int numEmailNewMsgs,
                  int numEmailOldMsgs
)
{
    char integerString[255];
	
    // Adding  Message-summary information
    SNPRINTF(integerString, sizeof(integerString), "\r\n");
    msgSummaryData.append(integerString);

    if(NULL != msgAccountUri)
    {
        SNPRINTF(integerString, sizeof(integerString), "Message-Account: %s\r\n",msgAccountUri);
        msgSummaryData.append(integerString);
    }
	
    if (TRUE == bNewMsgs)
    {
         // Adding Messages-waiting Yes  
         SNPRINTF(integerString, sizeof(integerString), "Messages-Waiting: yes\r\n");
    }
    else
    {
         // Adding Messages-waiting No
        SNPRINTF(integerString, sizeof(integerString), "Messages-Waiting: no\r\n");
    }
    msgSummaryData.append(integerString);	

    if (bVoiceMsgs)
    {
        SNPRINTF(integerString, sizeof(integerString), "Voice-Message: %d/%d\r\n",numNewMsgs,numOldMsgs);
        msgSummaryData.append(integerString);
    }

    if (bFaxMsgs)
    {
        SNPRINTF(integerString, sizeof(integerString), "Fax-Message: %d/%d\r\n",numFaxNewMsgs,numFaxOldMsgs);
        msgSummaryData.append(integerString);
    }

    if (bEmailMsgs)
    {
        SNPRINTF(integerString, sizeof(integerString), "Email-Message: %d/%d\r\n",numEmailNewMsgs,numEmailOldMsgs);
        msgSummaryData.append(integerString);
    }
	
}

// Added by Udit for RFC 3248 MWI
void SipMessage::setMWIData(const char *method,
				  const char* fromField,
                  const char* toField,
                  const char* uri,
                  const char* contactUrl,
                  const char* callId,
                  int CSeq,
                  UtlString bodyString)
{
    setRequestData(method, uri,
                     fromField, toField,
                     callId,
                     CSeq,
                     contactUrl);
    // Set the allow field
    setHeaderValue(SIP_ACCEPT_FIELD, CONTENT_TYPE_SIMPLE_MESSAGE_SUMMARY, 0);
    // Set the event type
    setHeaderValue(SIP_EVENT_FIELD, SIP_EVENT_MESSAGE_SUMMARY, 0);
    
     // Add the content type
    setContentType(CONTENT_TYPE_SIMPLE_MESSAGE_SUMMARY);
	// Create and add the Http body
    HttpBody *httpBody = new HttpBody(bodyString.data(),bodyString.length(),
    	CONTENT_TYPE_SIMPLE_MESSAGE_SUMMARY);

    setContentLength(bodyString.length());
    setBody(httpBody);  
}

void SipMessage::setVoicemailData(const char* fromField,
                       const char* toField,
                  const char* uri,
                  const char* contactUrl,
                  const char* callId,
                  int CSeq,
                       int expiresInSeconds)
{
    setRequestData(SIP_SUBSCRIBE_METHOD, uri,
                     fromField, toField,
                     callId,
                     CSeq,
                           contactUrl);
    // Set the event type
    setHeaderValue(SIP_EVENT_FIELD, SIP_EVENT_MESSAGE_SUMMARY, 0);
    // Set the allow field
    setHeaderValue(SIP_ACCEPT_FIELD, CONTENT_TYPE_SIMPLE_MESSAGE_SUMMARY, 0);
   setExpiresField(expiresInSeconds);
}

void SipMessage::setRequestTerminatedResponseData(const SipMessage* request)
{
   setResponseData(request, SIP_REQUEST_TERMINATED_CODE, SIP_REQUEST_TERMINATED_TEXT);
}

void SipMessage::setRequestUnauthorized(const SipMessage* request,
                            const char* authenticationScheme,
                            const char* authenticationRealm,
                            const char* authenticationNonce,
                            const char* authenticationOpaque,
                            HttpEndpointEnum authEntity)
{
    setLocalIp(request) ;

    if(authEntity == SERVER)
    {
        setResponseData(request,
                        HTTP_UNAUTHORIZED_CODE,
                        HTTP_UNAUTHORIZED_TEXT);
    }
    else
    {
        setResponseData(request,
                        HTTP_PROXY_UNAUTHORIZED_CODE,
                        HTTP_PROXY_UNAUTHORIZED_TEXT);
    }

    setAuthenticationData(authenticationScheme, authenticationRealm,
                          authenticationNonce, authenticationOpaque,
                          NULL,
                          authEntity);
}

// This method is needed to cover the symetrical situation which is
// specific to SIP authorization (i.e. authentication and authorize
// fields may be in either requests or responses
UtlBoolean SipMessage::verifyMd5Authorization(const char* userId,
                                             const char* password,
                                             const char* nonce,
                                             const char* realm,
                                             const char* uri,
                                             HttpEndpointEnum authEntity) const
{
    UtlString uriString;
    UtlString method;

    if(isResponse())
    {
        int seqNum;
         // What is the correct URI for a response Authorization
        if(uri)
        {
            uriString.append(uri);
        }
        getCSeqField(&seqNum, &method);
    }
    else
    {
       // If the uri (should be Auth header uri parameter) is
       // passed in use it.
       if(uri)
       {
          uriString.append(uri);
       }
       // Otherwise dig out the request URI.  Note: it is not a good
       // idea to use the request uri to validate the digest hash as
       // it may not exactly match the Auth header uri parameter (in
       // which the validation will fail).
       else
       {
          getRequestUri(&uriString);
          OsSysLog::add(FAC_SIP,PRI_DEBUG, "SipMessage::verifyMd5Authorization using request URI: %s instead of Auth header uri parameter for digest\n",
                        uriString.data());
       }
       getRequestMethod(&method);
    }

#ifdef TEST
    OsSysLog::add(FAC_SIP,PRI_DEBUG, "SipMessage::verifyMd5Authorization - "
         "userId='%s', password='%s', nonce='%s', realm='%s', uri='%s', method='%s' \n",
         userId, password, nonce, realm, uriString.data(), method.data());
#endif

    UtlBoolean isAllowed = FALSE;
    isAllowed = HttpMessage::verifyMd5Authorization(userId,
                                        password,
                                        nonce,
                                        realm,
                                        method.data(),
                                        uriString.data(),
                                        authEntity);
    return isAllowed;
}

void SipMessage::setResponseData(const SipMessage* request,
                         int responseCode,
                         const char* responseText,
                         const char* localContact)
{
   setLocalIp(request) ;

   UtlString fromField;
   UtlString toField;
   UtlString callId;
   int sequenceNum;
   UtlString sequenceMethod;

   request->getFromField(&fromField);
   request->getToField(&toField);
   request->getCallIdField(&callId);
   request->getCSeqField(&sequenceNum, &sequenceMethod);

   setResponseData(responseCode, responseText,
      fromField.data(), toField.data(), callId,
               sequenceNum, sequenceMethod.data(), localContact) ;

   setViaFromRequest(request);
}

void SipMessage::setAckData(const char* uri,
                     const char* fromField,
                     const char* toField,
               const char* callId,
               int sequenceNumber)
{
   setRequestData(SIP_ACK_METHOD, uri,
                     fromField, toField,
                     callId, sequenceNumber);
}

void SipMessage::setAckData(const SipMessage* inviteResponse,
                     const SipMessage* inviteRequest,
                            const char* contact,
                            int sessionTimerExpires)
{
   setLocalIp(inviteResponse) ;

   UtlString fromField;
   UtlString toField;
   UtlString uri;
   UtlString callId;
   int sequenceNum;
   int responseCode ;
   UtlString sequenceMethod;
   UtlString requestContact;
   UtlBoolean SetDNSParameters = FALSE;

   inviteResponse->getFromField(&fromField);
   inviteResponse->getToField(&toField);
   responseCode = inviteResponse->getResponseStatusCode();

   // SDUA
   //Set URI

   //Only for 2xx responses check the record route and contact field of response
   if ( responseCode >= SIP_OK_CODE && responseCode < SIP_MULTI_CHOICE_CODE)
   {
       // Set route field if recordRoute was set.
      UtlString routeField;
      UtlString requestToField;
      if(inviteResponse->buildRouteField(&routeField))
      {
         setRouteField(routeField.data());

      }

        // Loose_route always put contact in URI
        inviteResponse->getContactUri( 0 , &uri);
        if (uri.isNull())
        {
            if(inviteRequest)
            inviteRequest->getRequestUri(&uri);
          else
            uri.append(toField.data());
        }

      //if no record route and no contact filed which is a problem of the
      //other side because they should have a contact field
      //We should be more tolerant and use the request uri of INVITE
      //or the to field
      if(uri.isNull())
      {
         if(inviteRequest)
         {
            inviteRequest->getRequestUri(&uri);
         }
         else
         {
            uri.append(toField.data());
         }
      }
   }
   else
   {
      //set uri from request if no contact field in the response
      UtlString routeField;
      if (inviteRequest)
      {
         inviteRequest->getRequestUri(&uri);
         OsSysLog::add(FAC_SIP,
                       (uri.data()[0] == '<') ? PRI_ERR : PRI_DEBUG,
                       "SipMessage::setAckData inviteRequest->getRequestUri() = '%s'",
                       uri.data());
         inviteRequest->getRouteField(&routeField);
         setRouteField(routeField);
      }
      else
         uri.append(toField.data());

      SetDNSParameters = TRUE;
   }

   //set senders contact
   if(contact && *contact)
   {
      setContactField(contact);
   }
   else if ( inviteRequest)
   {
      if( inviteRequest->getContactUri(0, &requestContact))
      {
         Url contactUrl(requestContact, true) ;
         contactUrl.includeAngleBrackets() ;
         contactUrl.toString(requestContact) ;
         setContactField(requestContact);
      }
   }

   //if no record route or contact add sticky DNS
   if ( SetDNSParameters)
   {
      // set the DNS fields
      UtlString protocol;
      UtlString address;
      UtlString port;

      if ( inviteResponse->getDNSField(&protocol , &address , &port))
      {
         setDNSField(protocol , address , port);
      }
   }

   inviteResponse->getCallIdField(&callId);
   inviteResponse->getCSeqField(&sequenceNum, &sequenceMethod);

   setAckData(uri.data(), fromField.data(), toField.data(), callId, sequenceNum);

    if(sessionTimerExpires > 0)
        setSessionExpires(sessionTimerExpires);
}

void SipMessage::setAckErrorData(const SipMessage* byeRequest)
{
   setResponseData(byeRequest, SIP_BAD_REQUEST_CODE, SIP_BAD_REQUEST_TEXT);
}

void SipMessage::setByeData(const char* uri, 
                            const char* fromField, 
                            const char* toField,
                            const char* callId,
                            const char* localContact,
                            int sequenceNumber)
{
   setRequestData(SIP_BYE_METHOD, uri,
                     fromField, toField,
                     callId, sequenceNumber, localContact);
}

void SipMessage::setByeData(const SipMessage* inviteRequest,
                            const char* remoteContact,
                            UtlBoolean byeToCallee,
                            int localCSequenceNumber,
                            const char* routeField,
                            const char* alsoInviteUri,
                            const char* localContact)
{
   UtlString fromField;
   UtlString toField;
   UtlString uri;
   UtlString callId;
   int dummySequenceNum;
   UtlString sequenceMethod;
   UtlString remoteContactString;

   setLocalIp(inviteRequest) ;

   if (remoteContact)
      remoteContactString.append(remoteContact);

   inviteRequest->getFromField(&fromField);
   inviteRequest->getToField(&toField);
   inviteRequest->getCallIdField(&callId);
   inviteRequest->getCSeqField(&dummySequenceNum, &sequenceMethod);

   setRouteField(routeField);

   if (!remoteContactString.isNull())
   {
      uri.append(remoteContactString);
   }

    // Use the route uri if set
    if(!uri.isNull())
    {
    }
    // Use the original uri from the INVITE if the INVITE is from
    // this side.
    else if(byeToCallee)
    {
        inviteRequest->getRequestUri(&uri);
    }
    // Use contact if present
    else if(inviteRequest->getContactUri(0, &uri) && !uri.isNull())
    {

    }

   else
   {
      uri.append(fromField.data());
   }

   if(byeToCallee)
   {
      setByeData(uri.data(), fromField.data(), toField.data(), callId, localContact, localCSequenceNumber);
   }
   else
   {
      setByeData(uri.data(), toField.data(), fromField.data(), callId, localContact, localCSequenceNumber);
   }


    if(alsoInviteUri && *alsoInviteUri)
    {
        if(!isRequireExtensionSet(SIP_CALL_CONTROL_EXTENSION))
        {
            addRequireExtension(SIP_CALL_CONTROL_EXTENSION);
        }
        addAlsoUri(alsoInviteUri);
    }

}


void SipMessage::setReferData(const SipMessage* inviteRequest,
                     UtlBoolean referToCallee,
                            int localCSequenceNumber,
                            const char* routeField,
                            const char* contactUrl,
                            const char* remoteContactUrl,
                            const char* transferTargetAddress,
                            const char* targetCallId)
{
   UtlString fromField;
   UtlString toField;
   UtlString uri;
   UtlString callId;
   int dummySequenceNum;
   UtlString sequenceMethod;

   setLocalIp(inviteRequest) ;

   inviteRequest->getFromField(&fromField);
   inviteRequest->getToField(&toField);
   inviteRequest->getCallIdField(&callId);
   inviteRequest->getCSeqField(&dummySequenceNum, &sequenceMethod);

   setRouteField(routeField);

    if(remoteContactUrl)
    {
        uri = remoteContactUrl;
    }

    // Use the route uri if set
    else if(!referToCallee)
    {
        inviteRequest->getContactUri(0, &uri);
    }

    // Use the original uri from the INVITE if the INVITE is from
    // this side.
   else if(referToCallee)
   {
      inviteRequest->getRequestUri(&uri);
   }

    // Use contact if present
    else if(inviteRequest->getContactUri(0, &uri) && !uri.isNull())
    {

    }

   else
   {
      uri.append(fromField.data());
   }

    UtlString referredByField;
   if(referToCallee)
   {
        setRequestData(SIP_REFER_METHOD, uri.data(),
                  fromField.data(), toField.data(),
                  callId, localCSequenceNumber,
                        contactUrl);

        Url referToUrl(fromField);
        referToUrl.removeFieldParameters();
        referToUrl.includeAngleBrackets();
        referToUrl.toString(referredByField);
   }
   else
   {
        setRequestData(SIP_REFER_METHOD, uri.data(),
                  toField.data(), fromField.data(),
                  callId, localCSequenceNumber,
                        contactUrl);

        Url referToUrl(toField);
        referToUrl.removeFieldParameters();
        referToUrl.includeAngleBrackets();
        referToUrl.toString(referredByField);
   }

    //referredByField.append("; ");
    if(transferTargetAddress && *transferTargetAddress)
    {
        UtlString targetAddress(transferTargetAddress);
        Url targetUrl(targetAddress);
        UtlString referTo;

        if(targetCallId && *targetCallId)
        {
            //targetAddress.append("?Call-ID=");
            //targetAddress.append(targetCallId);
            targetUrl.setHeaderParameter(SIP_CALLID_FIELD, targetCallId);
        }

        // Include angle brackets on the Refer-To header.  We don't
        // need to do this, but it is the friendly thing to do.
        targetUrl.includeAngleBrackets();
        targetUrl.toString(referTo);

        // This stuff went away in the Transfer-05/Refer-02 drafts
        // We need angle brackets for the refer to url in the referred-by
        //targetUrl.includeAngleBrackets();
        //targetUrl.toString(targetAddress);
        //referredByField.append("ref=");
        //referredByField.append(targetAddress);

        setReferredByField(referredByField.data());
        setReferToField(referTo.data());
    }

}

void SipMessage::setReferOkData(const SipMessage* referRequest)
{
   setResponseData(referRequest, SIP_OK_CODE, SIP_OK_TEXT);
}

void SipMessage::setReferDeclinedData(const SipMessage* referRequest)
{
   setResponseData(referRequest, SIP_DECLINE_CODE, SIP_DECLINE_TEXT);
}

void SipMessage::setReferFailedData(const SipMessage* referRequest)
{
   setResponseData(referRequest, SIP_SERVICE_UNAVAILABLE_CODE, SIP_SERVICE_UNAVAILABLE_TEXT);
}

void SipMessage::setOptionsData(const SipMessage* inviteRequest,
                        const char* remoteContact,
                        UtlBoolean optionsToCallee,
                        int localCSequenceNumber,
                        const char* routeField,
                        const char* localContact)
{
   UtlString fromField;
   UtlString toField;
   UtlString uri;
   UtlString callId;
   int dummySequenceNum;
   UtlString sequenceMethod;

   setLocalIp(inviteRequest) ;

   inviteRequest->getFromField(&fromField);
   inviteRequest->getToField(&toField);
   inviteRequest->getCallIdField(&callId);
   inviteRequest->getCSeqField(&dummySequenceNum, &sequenceMethod);

   setRouteField(routeField);

   //set the uri to the contact uri returned in the last response from the other side
   if (remoteContact && *remoteContact)
   {
      uri.append(remoteContact);
   }

    // Use the remoteContact uri if set
    if(!uri.isNull())
    {
    }

    // Use the original uri from the INVITE if the INVITE is from
    // this side.
   else if(optionsToCallee)
   {
      inviteRequest->getRequestUri(&uri);
   }

    // Use contact if present
    else if(inviteRequest->getContactUri(0, &uri) && !uri.isNull())
    {

    }

   else
   {
      uri.append(fromField.data());
   }

    UtlString referredByField;
   if(optionsToCallee)
   {
        setRequestData(SIP_OPTIONS_METHOD, uri.data(),
                  fromField.data(), toField.data(),
                  callId, localCSequenceNumber, localContact);

        referredByField = fromField;
   }
   else
   {
        setRequestData(SIP_OPTIONS_METHOD, uri.data(),
                  toField.data(), fromField.data(),
                  callId, localCSequenceNumber, localContact);

        referredByField = toField;
   }
}

void SipMessage::setByeErrorData(const SipMessage* byeRequest)
{
   setResponseData(byeRequest, SIP_BAD_REQUEST_CODE, SIP_BAD_REQUEST_TEXT);
}

void SipMessage::setCancelData(const char* fromField, const char* toField,
               const char* callId,
               int sequenceNumber, const char* localContact)
{
   setRequestData(SIP_CANCEL_METHOD, toField,
                     fromField, toField,
                     callId, sequenceNumber, localContact);
}

void SipMessage::setCancelData(const SipMessage* inviteRequest,
                               const char* localContact)
{
    UtlString uri;
   UtlString fromField;
   UtlString toField;
   UtlString callId;
   int sequenceNum;
   UtlString sequenceMethod;

   setLocalIp(inviteRequest) ;

   inviteRequest->getFromField(&fromField);
   inviteRequest->getToField(&toField);
   inviteRequest->getCallIdField(&callId);
   inviteRequest->getCSeqField(&sequenceNum, &sequenceMethod);
    inviteRequest->getRequestUri(&uri);

    setRequestData(SIP_CANCEL_METHOD, uri,
                  fromField, toField,
                  callId, sequenceNum, localContact);
}

void SipMessage::setPrackData(const char* fromAddress,
                              const char* toAddress,
                              const char* callId,
                              int sequenceNumber,
                              int prackRSequenceNumber,
                              int prackCSequenceNumber,
                              const char* prackMethod,
                              const char* localContact)
{
   setRequestData(SIP_PRACK_METHOD, toAddress,
      fromAddress, toAddress,
      callId, sequenceNumber, localContact);
   setRAckField(prackRSequenceNumber, prackCSequenceNumber, prackMethod);
}

void SipMessage::setPublishData(const char* uri,
                                const char* fromField,
                                const char* toField,
                                const char* callId,
                                int cseq,
                                const char* eventField,
                                const char* id,
                                const char* sipIfMatchField,
                                int expiresInSeconds,
                                const char* contact)
{
    setRequestData(SIP_PUBLISH_METHOD, uri,
                   fromField, toField,
                   callId,
                   cseq,
                   contact);

    // Set the event type
    if( eventField && *eventField )
    {
        UtlString eventHeaderValue(eventField);
        if (id && *id)
        {
           eventHeaderValue.append(";id=");
           eventHeaderValue.append(id);
        }
        setEventField(eventHeaderValue.data());
        setHeaderValue(SIP_EVENT_FIELD, eventHeaderValue, 0);
    }

    // Set the SipIfMatch field
    if( sipIfMatchField && *sipIfMatchField )
    {
       setSipIfMatchField(sipIfMatchField);
    }
    
    // setExpires
    setExpiresField(expiresInSeconds);
}

void SipMessage::applyTargetUriHeaderParams()
{
   UtlString uriWithHeaderParams;
   getRequestUri(&uriWithHeaderParams);

   Url requestUri(uriWithHeaderParams, TRUE);

   int header;
   UtlString hdrName;
   UtlString hdrValue;
   for (header=0; requestUri.getHeaderParameter(header, hdrName, hdrValue); header++ )
   {
      // If the header is allowed in a header parameter?
      if(isUrlHeaderAllowed(hdrName.data()))
      {
         if (0 == hdrName.compareTo(SIP_FROM_FIELD, UtlString::ignoreCase))
         {
            /*
             * The From header requires special handling
             * - we need to preserve the tag, if any, from the original header
             */
            UtlString originalFromHeader;
            getFromField(&originalFromHeader);
            Url originalFromUrl(originalFromHeader);

            UtlString originalFromTag;
            originalFromUrl.getFieldParameter("tag", originalFromTag);

            Url newFromUrl(hdrValue.data());
            newFromUrl.removeFieldParameter("tag"); // specifying a tag is a no-no
            if ( !originalFromTag.isNull() )
            {
               newFromUrl.setFieldParameter("tag", originalFromTag.data());
            }
            UtlString newFromFieldValue;
            newFromUrl.toString(newFromFieldValue);
                   
            setRawFromField(newFromFieldValue.data());

            // I suspect that this really should be adding a History-Info element of some kind
            addHeaderField("X-Original-From", originalFromHeader);
         }
         else if (0 == hdrName.compareTo(SIP_ROUTE_FIELD, UtlString::ignoreCase))
         {
            /*
             * The Route header requires special handling
             *   Examine each redirected route and ensure that it is a loose route
             */
            OsSysLog::add(FAC_SIP, PRI_DEBUG,
                          "SipMessage::applyTargetUriHeaderParams found route header '%s'",
                          hdrValue.data()
                          );

            UtlString routeParams;
            int route;
            UtlString thisRoute;
            for (route=0;
                 NameValueTokenizer::getSubField(hdrValue.data(), route,
                                                 SIP_MULTIFIELD_SEPARATOR, &thisRoute);
                 thisRoute.remove(0), route++
                 )
            {
               Url newRouteUrl(thisRoute.data());
               UtlString unusedValue;
               if ( ! newRouteUrl.getUrlParameter("lr", unusedValue, 0))
               {
                  newRouteUrl.setUrlParameter("lr",NULL); // force a loose route
               }
            
               UtlString newRoute;
               newRouteUrl.toString(newRoute);
               if (!routeParams.isNull())
               {
                  routeParams.append(SIP_MULTIFIELD_SEPARATOR);
               }
               routeParams.append(newRoute);
            }
            // If we found any routes, push them onto the route set
            if (!routeParams.isNull())
            {
               OsSysLog::add(FAC_SIP, PRI_DEBUG,
                             "SipMessage::applyTargetUriHeaderParams adding route(s) '%s'",
                             routeParams.data()
                             );
               addRouteUri(routeParams.data());
            }
            
         }
         else if (isUrlHeaderUnique(hdrName.data()))
         {
            // If the field exists, change it,
            // if does not exist, create it.
            setHeaderValue(hdrName.data(), hdrValue.data(), 0);
         }
         else
         {
            addHeaderField(hdrName.data(), hdrValue.data());
         }
      }
      else
      {
         OsSysLog::add(FAC_SIP, PRI_WARNING, "URL header disallowed: %s: %s",
                       hdrName.data(), hdrValue.data());
      }
   }
   if (header)
   {
      // Remove the header fields from the URL as they
      // have been added to the message
      UtlString uriWithoutHeaderParams;
      requestUri.removeHeaderParameters();
      // Use getUri to get the addr-spec formmat, not the name-addr
      // format, because uriWithoutHeaderParams will be used as the
      // request URI of the request.
      requestUri.getUri(uriWithoutHeaderParams);

      changeRequestUri(uriWithoutHeaderParams);
   }
}


void SipMessage::addVia(const char* domainName,
                        int port,
                        const char* protocol,
                        const char* branchId,
                        const bool  bIncludeRport,
                        const char* customRouteId)
{
   UtlString viaField(SIP_PROTOCOL_VERSION);
   char portString[MAXIMUM_INTEGER_STRING_LENGTH + 1];
   bool bCustomRouteId = (customRouteId && strlen(customRouteId)) ;

   viaField.append("/");
   if(protocol && strlen(protocol))
   {
      viaField.append(protocol);
   }
   // Default the protocol if not set
   else
   {
      viaField.append(SIP_TRANSPORT_TCP_STR);
   }
   viaField.append(" ");
   
    if (bCustomRouteId)
    {
        viaField.append(customRouteId);
    }
    else
    {
        viaField.append(domainName);
        if(portIsValid(port))
        {
            SNPRINTF(portString, sizeof(portString), ":%d", port);
            viaField.append(portString);
        }        
    }
   

    if(branchId && *branchId)
    {
        viaField.append(';');
        viaField.append("branch");
        viaField.append('=');
        viaField.append(branchId);
    }

    if (bIncludeRport && !bCustomRouteId)
    {
        viaField.append(';');
        viaField.append("rport");
    }

   addViaField(viaField.data());
}

void SipMessage::addViaField(const char* viaField, UtlBoolean afterOtherVias)
{
    mHeaderCacheClean = FALSE;

   NameValuePair* nv = new NameValuePair(SIP_VIA_FIELD, viaField);
    // Look for other via fields
    unsigned int fieldIndex = mNameValues.index(nv);

    //osPrintf("SipMessage::addViaField via: %s after: %s fieldIndex: %d headername: %s\n",
    //    viaField, afterOtherVias ? "true" : "false", fieldIndex,
    //    SIP_VIA_FIELD);


    if(fieldIndex == UTL_NOT_FOUND)
    {
#ifdef TEST_PRINT
        UtlDListIterator iterator(mNameValues);

        //remove whole line
        NameValuePair* nv = NULL;
        while(nv = (NameValuePair*) iterator())
        {
            osPrintf("\tName: %s\n", nv->data());
        }
#endif
    }

    mHeaderCacheClean = FALSE;

    if(fieldIndex == UTL_NOT_FOUND || !afterOtherVias)
    {
      mNameValues.insert(nv);
    }
    else
    {
        mNameValues.insertAt(fieldIndex, nv);
    }
}

void SipMessage::setLastViaTag(const char* tagValue,
                               const char* tagName)
{
     UtlString lastVia;
   //get last via field and remove it
    getViaFieldSubField(&lastVia, 0);
   removeLastVia();
   //update the last via and add the updated field
    //setUriParameter(&lastVia, tagName, receivedFromIpAddress);

    //parse all name=value pairs into a collectable object
    UtlSList list;
    parseViaParameters(lastVia,list);

    //create an iterator to walk the list
    UtlSListIterator iterator(list);
   NameValuePair* nvPair;
    UtlString newVia;
    UtlBoolean bFoundTag = FALSE;
    UtlString value;

   while ((nvPair = (NameValuePair*) iterator()))
   {
        value.remove(0);

        //only if we have something in our newVia string do we add a semicolon
        if (newVia.length())
            newVia.append(";");

        //always append the name part of the value pair
        newVia.append(nvPair->data());

        UtlString strPairName = nvPair->data();
        UtlString strTagName = tagName;

        //convert both to upper
        strPairName.toUpper();
        strTagName.toUpper();

        //if the value we are looking for is found, then we are going to replace the value with this value
        if (strTagName == strPairName)
        {
            if (tagValue)
                value = tagValue;
            else
                //the value could come in as NULL.  In this case make it be an empty string
                value = "";

            bFoundTag = TRUE;
        }
        else
            //ok we didn't find the one we are looking for
            value = nvPair->getValue();

        //if the value has a length then append it
        if (value.length())
        {
            newVia.append("=");
            newVia.append(value);
        }

   }

    //if we didn't find the tag we are looking for after looping, then
    //we should add the name and value pair at the end
    if (!bFoundTag)
    {
        //add a semicolon before our new name value pair is added
        newVia.append(";");
        newVia.append(tagName);

        //only if it is non-NULL and has a length do we add the equal and value
        //So, if the value is "" we will only put the name (without equal)
        if (tagValue  && strlen(tagValue))
        {
            newVia.append("=");
            newVia.append(tagValue);
        }
    }

    addViaField(newVia);

    list.destroyAll();
}

void SipMessage::setViaFromRequest(const SipMessage* request)
{
   UtlString viaSubField;
   int subFieldindex = 0;

   while(request->getViaFieldSubField(&viaSubField, subFieldindex ))
   {
#ifdef TEST
      osPrintf("Adding via field: %s\n", viaSubField.data());
#endif
      addViaField(viaSubField.data(), FALSE);
      subFieldindex++;
   }
}

void SipMessage::setReceivedViaParams(const UtlString& fromIpAddress,
                                      int              fromPort
                                      )
{
   UtlString lastAddress;
   UtlString lastProtocol;
   int lastPort;

   int receivedPort;

   UtlBoolean receivedSet;
   UtlBoolean maddrSet;
   UtlBoolean receivedPortSet;

   // Check that the via is set to the address from whence
   // this message came
   getLastVia(&lastAddress, &lastPort, &lastProtocol,
              &receivedPort, &receivedSet, &maddrSet, &receivedPortSet);

   // The via address is different from that of the sockets
   if(lastAddress.compareTo(fromIpAddress) != 0)
   {
      // Add a receive from tag
      setLastViaTag(fromIpAddress.data());
   }

   // If the rport tag is present the sender wants to
   // know what port this message was received from
   int tempLastPort = lastPort;
   if (!portIsValid(lastPort))
   {
      tempLastPort = 5060;
   }

   if (receivedPortSet)
   {
      char portString[20];
      SNPRINTF(portString, sizeof(portString), "%d", fromPort);
      setLastViaTag(portString, "rport");
   }
}

void SipMessage::setCallIdField(const char* callId)
{
    setHeaderValue(SIP_CALLID_FIELD, callId);
}

void SipMessage::setCSeqField(int sequenceNumber, const char* method)
{
   UtlString value;
   char numString[HTTP_LONG_INT_CHARS];

   SNPRINTF(numString, sizeof(numString), "%d", sequenceNumber);

   value.append(numString);
   value.append(SIP_SUBFIELD_SEPARATOR);
   value.append(method);

    setHeaderValue(SIP_CSEQ_FIELD, value.data());
}

void SipMessage::setRSeqField(int sequenceNumber)
{
   UtlString value;
   char numString[HTTP_LONG_INT_CHARS];

   SNPRINTF(numString, sizeof(numString), "%d", sequenceNumber);
   value.append(numString);

   setHeaderValue(SIP_RSEQ_FIELD, value.data());
}

void SipMessage::setRAckField(int rsequenceNumber, int csequenceNumber, const char* method)
{
   UtlString value;
   char numString1[HTTP_LONG_INT_CHARS];
   char numString2[HTTP_LONG_INT_CHARS];

   SNPRINTF(numString1, sizeof(numString1), "%d", rsequenceNumber);
   SNPRINTF(numString2, sizeof(numString2), "%d", csequenceNumber);
   value.append(numString1);
   value.append(SIP_SUBFIELD_SEPARATOR);
   value.append(numString2);
   value.append(SIP_SUBFIELD_SEPARATOR);
   value.append(method);

   setHeaderValue(SIP_RACK_FIELD, value.data());
}

void SipMessage::incrementCSeqNumber()
{
    int seqNum;
    UtlString seqMethod;
    if(!getCSeqField(&seqNum, &seqMethod))
    {
        seqNum = 1;
        seqMethod.append("UNKNOWN");
    }
    seqNum++;
    setCSeqField(seqNum, seqMethod.data());
}

void SipMessage::buildSipUrl(UtlString* url, const char* address, int port,
                      const char* protocol, const char* user,
                      const char* userLabel, const char* tag)
{
   char portString[MAXIMUM_INTEGER_STRING_LENGTH + 1];
   url->remove(0);
   UtlString tmpAddress(address);
   tmpAddress.toUpper();

   if(userLabel && strlen(userLabel))
   {
      url->append(userLabel);
      url->append("<");
   }

   // If the SIP url type label is not already in the address
   int sipLabelIndex = tmpAddress.index(SIP_URL_TYPE);
   //osPrintf("Found \"%s\" in \"%s\" at index: %d\n",
   // SIP_URL_TYPE, tmpAddress.data(),
   //   sipLabelIndex);

   if(sipLabelIndex < 0 && !tmpAddress.isNull())
   {
      // Use lower case for more likely interoperablity
      UtlString sipLabel(SIP_URL_TYPE);
      sipLabel.toLower();
      url->append(sipLabel.data());
   }
   //else
   //{
   // osPrintf("SIP: not added index: %d\n", sipLabelIndex);
   //}

   if(!strstr(address, "@") && user && strlen(user))
   {
      url->append(user);
      url->append('@');
   }

   url->append(address);

   if (portIsValid(port))
   {
      SNPRINTF(portString, sizeof(portString), ":%d", port);
      url->append(portString);
   }

   if(protocol && strlen(protocol) > 0)
   {
      url->append(";transport=");
      url->append(protocol);
   }
   if(userLabel && strlen(userLabel))
   {
      url->append(">");
   }

   if(tag && strlen(tag))
   {
      url->append(";tag=");
      url->append(tag);
   }

   tmpAddress.remove(0);
}

void SipMessage::setFromField(const char* url)
{
   UtlString value;
   UtlString address;
   UtlString protocol;
   UtlString user;
   UtlString userLabel;
   int port;

   parseAddressFromUri(url, &address, &port, &protocol, &user,
      &userLabel);
   buildSipUrl(&value, address.data(), port, protocol.data(),
      user.data(), userLabel.data());

   // If the field exists change it, if does not exist create it.
   setHeaderValue(SIP_FROM_FIELD, value.data(), 0);
}

void SipMessage::setFromField(const char* address, int port,
                       const char* protocol,
                       const char* user, const char* userLabel)
{
   UtlString url;
   buildSipUrl(&url, address, port, protocol, user, userLabel);

   // If the field exists change it, if does not exist create it
   setHeaderValue(SIP_FROM_FIELD, url.data(), 0);
}

void SipMessage::setRawToField(const char* url)
{
   setHeaderValue(SIP_TO_FIELD, url, 0);
}

void SipMessage::setRawFromField(const char* url)
{
   // If the field exists change it, if does not exist create it
   setHeaderValue(SIP_FROM_FIELD, url, 0);
}

void SipMessage::setToField(const char* address, int port,
                       const char* protocol,
                       const char* user,
                       const char* userLabel)
{
   UtlString url;

   buildSipUrl(&url, address, port, protocol, user, userLabel);

   // If the field exists change it, if does not exist create it
   setHeaderValue(SIP_TO_FIELD, url.data(), 0);
}

void SipMessage::setExpiresField(int expiresInSeconds)
{
   if (expiresInSeconds >= 0)
   {
      char secondsString[MAXIMUM_INTEGER_STRING_LENGTH];
      SNPRINTF(secondsString, sizeof(secondsString), "%d", expiresInSeconds);

       // If the field exists change it, if does not exist create it
       setHeaderValue(SIP_EXPIRES_FIELD, secondsString, 0);
   }
}

void SipMessage::setMinExpiresField(int minimumExpiresInSeconds)
{
   char secondsString[MAXIMUM_INTEGER_STRING_LENGTH];
   SNPRINTF(secondsString, sizeof(secondsString), "%d", minimumExpiresInSeconds);

   // If the field exists change it, if does not exist create it
   setHeaderValue(SIP_MIN_EXPIRES_FIELD, secondsString, 0);
}

void SipMessage::setContactField(const char* contactField, int index)
{
    // If the field exists change it.  If it does not exist, create it.
    setHeaderValue(SIP_CONTACT_FIELD, contactField, index);
}

void SipMessage::setRequestDispositionField(const char* dispositionField)
{
   // If the field exists change it, if does not exist create it
   setHeaderValue(SIP_REQUEST_DISPOSITION_FIELD, dispositionField, 0);
}

void SipMessage::addRequestDisposition(const char* dispositionToken)
{
    // Append to the field value already there, if it exists
    UtlString field;
    getRequestDispositionField(&field);
    if(!field.isNull())
    {
        field.append(' ');
    }

    field.append(dispositionToken);
    setRequestDispositionField(field.data());
}

void SipMessage::setWarningField(int code, const char* hostname, const char* text)
{
   UtlString warningContent;
   size_t sizeNeeded = 3 /* warning code size */ + strlen(hostname) + strlen(text) + 3 /* blanks & null */;
   size_t allocated = warningContent.capacity(sizeNeeded);

   if (allocated >= sizeNeeded)
   {
      SNPRINTF((char*)warningContent.data(), allocated, "%3d %s %s", code, hostname, text);
      
      setHeaderValue(SIP_WARNING_FIELD, warningContent.data());
   }
   else
   {
      OsSysLog::add(FAC_SIP, PRI_WARNING,
                    "SipMessage::setWarningField value too large (max %d) host '%s' text '%s'",
                    allocated, hostname, text
                    );
   }
}

void SipMessage::changeUri(const char* newUri)
{
   UtlString uriString;

    // Remove the stuff that should not be in a URI
    Url cleanUri(newUri);
    cleanUri.getUri(uriString);
   changeRequestUri(uriString.data());
}

UtlBoolean SipMessage::getMaxForwards(int& maxForwards) const
{
    const char* value = getHeaderValue(0, SIP_MAX_FORWARDS_FIELD);

    if(value)
    {
        maxForwards = atoi(value);
    }
    return(value != NULL);
}


void SipMessage::setMaxForwards(int maxForwards)
{
    char buf[64];
    SNPRINTF(buf, sizeof(buf), "%d", maxForwards);
    setHeaderValue(SIP_MAX_FORWARDS_FIELD,buf, 0);
}

void SipMessage::decrementMaxForwards()
{
    int maxForwards;
    if(!getMaxForwards(maxForwards))
    {
        maxForwards = SIP_DEFAULT_MAX_FORWARDS;
    }
    maxForwards--;
    setMaxForwards(maxForwards);
}

void SipMessage::getFromField(UtlString* field) const
{
   const char* value = getHeaderValue(0, SIP_FROM_FIELD);

   if(value)
   {
        *field = value;
   }
    else
    {
        field->remove(0);
    }
}

void SipMessage::getFromFieldTag(UtlString& fromTag) const
{
   Url fromUrl;
   getFromUrl(fromUrl);
   fromUrl.getFieldParameter("tag", fromTag);
}

void SipMessage::getToField(UtlString* field) const
{
   const char* value = getHeaderValue(0, SIP_TO_FIELD);

   if(value)
   {
      *field = value;
   }
   else
   {
      field->remove(0);
   }
}

void SipMessage::getToFieldTag(UtlString& toTag) const
{
   Url toUrl;
   getToUrl(toUrl);
   toUrl.getFieldParameter("tag", toTag);
}

void SipMessage::getToUrl(Url& toUrl) const
{
    //UtlString toField;
    //getToField(&toField);
    const char* toField = getHeaderValue(0, SIP_TO_FIELD);

    toUrl = toField ? toField : "";
}

void SipMessage::getFromUrl(Url& fromUrl) const
{
    //UtlString fromField;
    //getFromField(&fromField);
    const char* fromField = getHeaderValue(0, SIP_FROM_FIELD);

    fromUrl = fromField ? fromField : "";
}

void SipMessage::getFromLabel(UtlString* label) const
{
   UtlString field;
   int labelEnd;

   getFromField(&field);

   label->remove(0);

   if(!field.isNull())
   {
      labelEnd = field.index(" <");
      if(labelEnd < 0)   // seen without space too 
      {
         labelEnd = field.index("<");
      }

      if(labelEnd >= 0)
      {
         label->append(field.data());
         label->remove(labelEnd);
      }
   }
}

UtlBoolean SipMessage::getPAssertedIdentityField(UtlString& identity,
                                                 int index) const
{
    UtlBoolean fieldExists = 
        getFieldSubfield(SIP_P_ASSERTED_IDENTITY_FIELD, index, &identity);
    NameValueTokenizer::frontBackTrim(&identity, " \t");
    return(fieldExists && !identity.isNull());
}

void SipMessage::removePAssertedIdentityFields()
{
    while(removeHeader(SIP_P_ASSERTED_IDENTITY_FIELD, 0))
    {
    }
}

void SipMessage::addPAssertedIdentityField(const UtlString& identity)
{
    // Order does not matter so just get the first occurance and 
    // add a field seperator and the new identity value
    UtlString firstValue;
    const char* value = getHeaderValue(0, SIP_P_ASSERTED_IDENTITY_FIELD);
    if(value)
    {
        firstValue = value;
        firstValue.append(SIP_MULTIFIELD_SEPARATOR);
        firstValue.append(' ');
    }
    firstValue.append(identity);

    setHeaderValue(SIP_P_ASSERTED_IDENTITY_FIELD, firstValue, 0);
}

void SipMessage::getToLabel(UtlString* label) const
{
   UtlString field;
   int labelEnd;

   getToField(&field);

   label->remove(0);

   if(!field.isNull())
   {
      labelEnd = field.index(" <");
      if(labelEnd < 0)  // seen without space too 
      {
         labelEnd = field.index("<");
      }

      if(labelEnd >= 0)
      {
         label->append(field.data());
         label->remove(labelEnd);
      }
   }
}

UtlBoolean SipMessage::parseParameterFromUri(const char* uri,
                                            const char* parameterName,
                                            UtlString* parameterValue)
{
    UtlString parameterString(parameterName);
    UtlString uriString(uri);
    parameterString.append("=");
    // This may need to be changed to be make case insensative
    int parameterStart = uriString.index(parameterString.data());
    // 0, UtlString::ignoreCase);

    parameterValue->remove(0);

    //osPrintf("SipMessage::parseParameterFromUri uri: %s parameter: %s index: %d\n",
    //    uriString.data(), parameterString.data(), parameterStart);
    if(parameterStart >= 0)
    {
        parameterStart += parameterString.length();
        uriString.remove(0, parameterStart);
        NameValueTokenizer::frontTrim(&uriString, " \t");
        //osPrintf("SipMessage::parseParameterFromUri uriString: %s index: %d\n",
        //  uriString.data(), parameterStart);
        NameValueTokenizer::getSubField(uriString.data(), 0,
            " \t;>", parameterValue);

    }

    return(parameterStart >= 0);
}

void SipMessage::parseAddressFromUri(const char* uri,
                            UtlString* address,
                            int* port,
                            UtlString* protocol,
                            UtlString* user,
                            UtlString* userLabel,
                            UtlString* tag)
{
   // A SIP url looks like the following:
   // "user label <SIP:user@address:port ;tranport=protocol> ;tag=nnnn"
   Url parsedUri(uri);
   
   if (address)
   {
      parsedUri.getHostAddress(*address);
   }
   if (port)
   {
      *port = parsedUri.getHostPort();
   }
   if (protocol)
   {
      parsedUri.getUrlParameter("transport",*protocol);
   }
   if (user)
   {
      parsedUri.getUserId(*user);
   }
   if (userLabel)
   {
      parsedUri.getDisplayName(*userLabel);
   }
   if (tag)
   {
      parsedUri.getFieldParameter("tag", *tag);
   }
}

void SipMessage::setUriParameter(UtlString* uri, const char* parameterName,
                                 const char* parameterValue)
{
    UtlString parameterString(parameterName);

    //only append the '=' if not null and has a length
    if (parameterValue && *parameterValue != '\0')
        parameterString.append('=');

   int tagIndex = uri->index(parameterString.data());

   // Tag already exists, replace it
   if(tagIndex >= 0)
   {
      //osPrintf("Found tag at index: %d\n", tagIndex);
        // Minimally start after the tag name & equal sign
      tagIndex+= parameterString.length();
      int tagSpace = uri->index(' ', tagIndex);
      int tagTab = uri->index('\t', tagIndex);
      int tagSemi = uri->index(';', tagIndex);
      int tagEnd = tagSpace;
      if(tagTab >= tagIndex && (tagTab < tagEnd || tagEnd < tagIndex))
      {
         tagEnd = tagTab;
      }
      if(tagSemi >= tagIndex && (tagSemi < tagEnd || tagEnd < tagIndex))
      {
         tagEnd = tagSemi;
      }

      // Remove up to the separator
      if(tagEnd >= tagIndex)
      {
         uri->remove(tagIndex, tagEnd - tagIndex);
      }
      // Remove to the end, no separator found
      else
      {
         uri->remove(tagIndex);
      }

        //only insert the value if not null and has a length
        if (parameterValue && *parameterValue != '\0')
          uri->insert(tagIndex, parameterValue);
   }

   // Tag does not exist append it
   else
   {
      //osPrintf("Found no tag appending to the end\n");
      uri->append(";");
        uri->append(parameterString.data());

        //only add the value if not null and has a length
        if (parameterValue && *parameterValue != '\0')
         uri->append(parameterValue);
   }
}

void SipMessage::setToFieldTag(const char* tagValue)
{
   UtlString toField;
   getToField(&toField);
   //osPrintf("To field before: \"%s\"\n", toField.data());
   setUriTag(&toField, tagValue);
   //osPrintf("To field after: \"%s\"\n", toField.data());
   setRawToField(toField.data());
}

void SipMessage::setToFieldTag(int tagValue)
{
    char tagString[MAXIMUM_INTEGER_STRING_LENGTH];
    SNPRINTF(tagString, sizeof(tagString), "%d", tagValue);
    setToFieldTag(tagString);
}

void SipMessage::setUriTag(UtlString* uri, const char* tagValue)
{
    setUriParameter(uri, "tag", tagValue);
}

void SipMessage::getUri(UtlString* address, int* port,
                     UtlString* protocol,
                     UtlString* user) const
{

   UtlString uriField;
   getRequestUri(&uriField);

   if( !uriField.isNull())
   {
      //Uri field will only have URL parameters . So add angle backets around the
      //whole string. else the url parameters will be trated as filed and header parameters

      Url uriUrl(uriField, TRUE); // is addrSpec

      if(address)
      {
          uriUrl.getHostAddress(*address);
      }
      if(protocol)
      {
          uriUrl.getUrlParameter("transport", *protocol);
      }
      if(port)
      {
          *port = uriUrl.getHostPort();
      }

      if(user)
      {
         uriUrl.getUserId(*user);
      }
   }
// parseAddressFromUri(uriField.data(), address, port, protocol, user);
}

void SipMessage::getFromAddress(UtlString* address, int* port, UtlString* protocol,
                        UtlString* user, UtlString* userLabel,
                        UtlString* tag) const
{
   UtlString uri;
   getFromField(&uri);

   parseAddressFromUri(uri.data(), address, port, protocol, user, userLabel,
      tag);
}

void SipMessage::getToAddress(UtlString* address, int* port, UtlString* protocol,
                       UtlString* user, UtlString* userLabel,
                       UtlString* tag) const
{
   UtlString uri;
   getToField(&uri);

   parseAddressFromUri(uri.data(), address, port, protocol, user, userLabel,
      tag);
}

void SipMessage::getFromUri(UtlString* uri) const
{
   UtlString field;
   int labelEnd;

   getFromField(&field);

   unsigned int uriEnd;

   uri->remove(0);

   if(!field.isNull())
   {
      labelEnd = field.index("<");
      // Look for a label terminator
      if(labelEnd >= 0)
      {
         // Remove the label and terminator
         labelEnd += 1;
         field.remove(0, labelEnd);

         // Find the URI terminator
         uriEnd = field.index(">");

         // No URI terminator found, assume the whole thing
         if(uriEnd == UTL_NOT_FOUND)
         {
            uri->append(field.data());
         }
         // Remove the terminator
         else
         {
            field.remove(uriEnd);
            uri->append(field.data());
         }
      }
      // There is no label take the whole thing as the URI
      else
      {
         uri->append(field.data());
      }
   }
}

UtlBoolean SipMessage::getResponseSendAddress(UtlString& address,
                                             int& port,
                                             UtlString& protocol) const
{
    int receivedPort;
    UtlBoolean receivedSet;
    UtlBoolean maddrSet;
    UtlBoolean receivedPortSet;

    // use the via as the place to send the response
    getLastVia(&address, &port, &protocol, &receivedPort,
        &receivedSet, &maddrSet, &receivedPortSet);

    // If the sender of the request indicated support of
    // rport (i.e. received port) send this response back to
    // the same port it came from
    if(receivedPortSet && portIsValid(receivedPort))
    {
        OsSysLog::add(FAC_SIP, PRI_DEBUG, "SipMessage::getResponseSendAddress response to receivedPort %s:%d not %d\n",
            address.data(), receivedPort, port);
        port = receivedPort;
    }

    // No  via, use the from
    if(address.isNull())
    {
        OsSysLog::add(FAC_SIP, PRI_WARNING,
                      "SipMessage::getResponseSendAddress No VIA address, using From address\n");

        getFromAddress(&address, &port, &protocol);
    }

    return(!address.isNull());
}

void SipMessage::convertProtocolStringToEnum(const char* protocolString,
                        OsSocket::IpProtocolSocketType& protocolEnum)
{
    if(strcasecmp(protocolString, SIP_TRANSPORT_UDP_STR) == 0)
    {
        protocolEnum = OsSocket::UDP;
    }
    else if(strcasecmp(protocolString, SIP_TRANSPORT_TCP_STR) == 0)
    {
        protocolEnum = OsSocket::TCP;
    }

    else if(strcasecmp(protocolString, SIP_TRANSPORT_TLS_STR) == 0)
    {
        protocolEnum = OsSocket::SSL_SOCKET;
    }
    else
    {
        protocolEnum = OsSocket::UNKNOWN;
    }

}

void SipMessage::convertProtocolEnumToString(OsSocket::IpProtocolSocketType protocolEnum,
                                            UtlString& protocolString)
{
   protocolString = OsSocket::ipProtocolString(protocolEnum);
}

void SipMessage::getToUri(UtlString* uri) const
{
   UtlString field;
   int labelEnd;

   getToField(&field);

   int uriEnd;

   uri->remove(0);

   if(!field.isNull())
   {
      labelEnd = field.index("<");
      // Look for a label terminator
      if(labelEnd >= 0)
      {
         // Remove the label and terminator
         labelEnd += 1;
         field.remove(0, labelEnd);

         // Find the URI terminator
         uriEnd = field.index(">", labelEnd);

         // No URI terminator found, assume the whole thing
         if(uriEnd < 0)
         {
            uri->append(field.data());
         }
         // Remove the terminator
         else
         {
            field.remove(uriEnd);
            uri->append(field.data());
         }
      }
      // There is no label take the whole thing as the URI
      else
      {
         uri->append(field.data());
      }
   }
}

UtlBoolean SipMessage::getWarningCode(int* warningCode, int index) const
{
   const char* value = getHeaderValue(index, SIP_WARNING_FIELD);
   UtlString warningField;
   int endOfCode;

   *warningCode = 0;

   if(value)
   {
      warningField.append(value);
      endOfCode = warningField.index(SIP_SUBFIELD_SEPARATOR);
      if(endOfCode > 0)
      {
         warningField.remove(endOfCode);
         *warningCode = atoi(warningField.data());
      }
   }
   return(value != NULL);
}

UtlBoolean SipMessage::removeLastVia()
{
   //do not remove the whole via header line . Remove only the first subfield
   UtlBoolean fieldFound = FALSE;
   UtlString NewViaHeader;
   UtlString viaField;

   if ( getViaField( &viaField , 0))
   {
      unsigned int posSubField = viaField.first(SIP_MULTIFIELD_SEPARATOR);
      if (posSubField != UTL_NOT_FOUND)
      {
         viaField.remove(0, posSubField + strlen(SIP_MULTIFIELD_SEPARATOR));
         NewViaHeader = viaField.strip(UtlString::both , ' ');
      }
   }


   NameValuePair viaHeaderField(SIP_VIA_FIELD);

   //remove whole line
   NameValuePair* nv = (NameValuePair*) mNameValues.find(&viaHeaderField);
   if(nv)
   {
        mHeaderCacheClean = FALSE;
      mNameValues.destroy(nv);
      nv = NULL;
      fieldFound = TRUE;
   }
   //add updated line
   if ( !NewViaHeader.isNull())
   {
      addViaField( NewViaHeader);
   }
   return(fieldFound);
}

UtlBoolean SipMessage::getViaField(UtlString* viaField, int index) const
{
   const char* value = getHeaderValue(index, SIP_VIA_FIELD);

   viaField->remove(0);
   if(value)
   {
      viaField->append(value);
   }
   return(value != NULL);
}

UtlBoolean SipMessage::getViaFieldSubField(UtlString* viaSubField, int subFieldIndex) const
{
   UtlBoolean retVal = FALSE;
   UtlString Via;
   if (getFieldSubfield(SIP_VIA_FIELD, subFieldIndex, &Via) )
   {
      viaSubField->remove(0);
      if(!Via.isNull())
      {
         viaSubField->append(Via);
         retVal = TRUE;
      }
   }
   return retVal;
}

void SipMessage::getLastVia(UtlString* address,
                            int* port,
                            UtlString* protocol,
                            int* receivedPort,
                            UtlBoolean* receivedSet,
                            UtlBoolean* maddrSet,
                            UtlBoolean* receivedPortSet) const
{
   UtlString Via;

   UtlString sipProtocol;
   UtlString url;
   UtlString receivedAddress;
   UtlString receivedPortString;
   UtlString maddr;
   int index;
   address->remove(0);
   *port = PORT_NONE;
   protocol->remove(0);

   if (port)
   {
      *port = 0;
   }
   if (address)
   {
      address->remove(0);
   }
   if (protocol)
   {
      protocol->remove(0);
   }
   if (receivedSet)
   {
      *receivedSet = FALSE;
   }
   if (maddrSet)
   {
      *maddrSet = FALSE;
   }
   if (receivedPortSet)
   {
      *receivedPortSet = FALSE;
   }
   
   // Get the first (most recent) Via value, which is the one that tells
   // how to send the response.
   if (getFieldSubfield(SIP_VIA_FIELD, 0, &Via))
   {
      NameValueTokenizer::getSubField(Via, 0, SIP_SUBFIELD_SEPARATORS,
                                      &sipProtocol);
      NameValueTokenizer::getSubField(Via, 1, SIP_SUBFIELD_SEPARATORS,
                                      &url);

      index = sipProtocol.index('/');
      if(index >= 0)
      {
         sipProtocol.remove(0, index + 1);
         index = sipProtocol.index('/');
      }

      if(index >= 0)
      {
         sipProtocol.remove(0, index + 1);
      }
      if (protocol)
      {
         protocol->append(sipProtocol.data());
      }

      Url viaParam(url,TRUE);
      if (address)
      {
         viaParam.getHostAddress(*address);
      }
      if (port)
      {
         *port = viaParam.getHostPort();
      }
      UtlBoolean receivedFound =
         viaParam.getUrlParameter("received", receivedAddress);
      UtlBoolean maddrFound = viaParam.getUrlParameter("maddr", maddr);
      UtlBoolean receivedPortFound =
         viaParam.getUrlParameter("rport", receivedPortString);

      // The maddr takes precedence over the received-by address
      if(address && !maddr.isNull())
      {
         *address = maddr;
      }

      // The received address takes precedence over the sent-by address
      else if(address && !receivedAddress.isNull())
      {
         address->remove(0);
         address->append(receivedAddress.data());
      }

      if(receivedPort
         && !receivedPortString.isNull())
      {
         *receivedPort = atoi(receivedPortString.data());
      }
      else if(receivedPort)
      {
         *receivedPort = PORT_NONE;
      }

      if(receivedSet)
      {
         *receivedSet = receivedFound;
      }
      if(maddrSet)
      {
         *maddrSet = maddrFound;
      }
      if(receivedPortSet)
      {
         *receivedPortSet = receivedPortFound;
      }
   }
}

UtlBoolean SipMessage::getViaTag(const char* viaField,
                                const char* tagName,
                                UtlString& tagValue)
{
    UtlString strNameValuePair;
    UtlBoolean tagFound = FALSE;
    UtlHashBag viaParameters;

    parseViaParameters(viaField,viaParameters);
    UtlString nameMatch(tagName);
    NameValuePair *pair = (NameValuePair *)viaParameters.find(&nameMatch);

    if (pair)
    {
        tagValue = pair->getValue();
        tagFound = TRUE;
    }
    else
        tagValue.remove(0);

    viaParameters.destroyAll();

    return(tagFound);
}


void SipMessage::getCallIdField(UtlString* callId) const
{
   const char* value = getHeaderValue(0, SIP_CALLID_FIELD);

   if(value)
   {
        *callId = value;
    }
    else
    {
        callId->remove(0);
    }
}

void SipMessage::getCallIdField(UtlString& callId) const
{
   getCallIdField(&callId);
}

UtlBoolean SipMessage::getCSeqField(int& sequenceNum, UtlString& sequenceMethod) const
{
   return getCSeqField(&sequenceNum, &sequenceMethod);
}

UtlBoolean SipMessage::getCSeqField(int* sequenceNum, UtlString* sequenceMethod) const
{
   const char* value = getHeaderValue(0, SIP_CSEQ_FIELD);
   if(value)
   {
        // Too slow:
       /*UtlString sequenceNumString;
      NameValueTokenizer::getSubField(value, 0,
               SIP_SUBFIELD_SEPARATORS, &sequenceNumString);
      *sequenceNum = atoi(sequenceNumString.data());

      NameValueTokenizer::getSubField(value, 1,
               SIP_SUBFIELD_SEPARATORS, sequenceMethod);*/

        // Ignore white space in the begining
        int valueStart = strspn(value, SIP_SUBFIELD_SEPARATORS);

        // Find the end of the sequence number
        int numStringLen = strcspn(&value[valueStart], SIP_SUBFIELD_SEPARATORS);

        // Get the method
        if(sequenceMethod)
        {
            *sequenceMethod = &value[numStringLen + valueStart];
            NameValueTokenizer::frontBackTrim(sequenceMethod, SIP_SUBFIELD_SEPARATORS);           
        }

		if(numStringLen > MAXIMUM_INTEGER_STRING_LENGTH)
        {
               OsSysLog::add(FAC_SIP, PRI_ERR,
                             "SipMessage::getCSeqField CSeq field '%.*s' containes %d digits, which exceeds MAXIMUM_INTEGER_STRING_LENGTH (%d).  Truncated.\n",
                             numStringLen, &value[valueStart], numStringLen,
                             MAXIMUM_INTEGER_STRING_LENGTH);
			numStringLen = MAXIMUM_INTEGER_STRING_LENGTH;
		}

        if(sequenceNum)
        {
            // Convert the sequence number
            char numBuf[MAXIMUM_INTEGER_STRING_LENGTH + 1];
            memcpy(numBuf, &value[valueStart], numStringLen);
            numBuf[numStringLen] = '\0';
            *sequenceNum = atoi(numBuf);
        }
   }
    else
    {
        if(sequenceNum)
        {
            *sequenceNum = -1;
        }

        if(sequenceMethod)
        {
            sequenceMethod->remove(0);
        }
    }

    return(value != NULL);
}

UtlBoolean SipMessage::getRSeqField(int& rsequenceNum) const
{
   const char* value = getHeaderValue(0, SIP_RSEQ_FIELD);
   if(value)
   {
      // Ignore white space in the beginning
      int valueStart = strspn(value, SIP_SUBFIELD_SEPARATORS); // find first char that is not separator
      // Find the end of the sequence number
      int numStringLen = strcspn(&value[valueStart], SIP_SUBFIELD_SEPARATORS);

      if(numStringLen > MAXIMUM_INTEGER_STRING_LENGTH)
      {
         numStringLen = MAXIMUM_INTEGER_STRING_LENGTH;
      }

      // Convert the sequence number
      char numBuf[MAXIMUM_INTEGER_STRING_LENGTH + 1];
      memcpy(numBuf, &value[valueStart], numStringLen);
      numBuf[numStringLen] = '\0';
      rsequenceNum = atoi(numBuf);
   }

   return value != NULL;
}

UtlBoolean SipMessage::getRAckField(int& rsequenceNum, int& csequenceNum, UtlString& sequenceMethod) const
{
   const char* value = getHeaderValue(0, SIP_RACK_FIELD);
   if(value)
   {
      // Ignore white space in the beginning
      int value1Start = strspn(value, SIP_SUBFIELD_SEPARATORS); // find first char that is not separator
      int value1End = strcspn(&value[value1Start], SIP_SUBFIELD_SEPARATORS) + value1Start; // find next separator for value1
      int value2Start = strspn(&value[value1End], SIP_SUBFIELD_SEPARATORS) + value1End; // find first char that is not separator
      int value2End = strcspn(&value[value2Start], SIP_SUBFIELD_SEPARATORS) + value2Start; // find next separator for value2
      int value3Start = strspn(&value[value2End], SIP_SUBFIELD_SEPARATORS) + value2End; // find first char that is not separator
      int value3End = strcspn(&value[value3Start], SIP_SUBFIELD_SEPARATORS) + value3Start; // find next separator for value3

      int numString1Len = value1End - value1Start;
      int numString2Len = value2End - value2Start;
      if(numString1Len > MAXIMUM_INTEGER_STRING_LENGTH)
      {
         numString1Len = MAXIMUM_INTEGER_STRING_LENGTH;
      }
      if(numString2Len > MAXIMUM_INTEGER_STRING_LENGTH)
      {
         numString2Len = MAXIMUM_INTEGER_STRING_LENGTH;
      }

      // Convert the sequence number
      char numBuf1[MAXIMUM_INTEGER_STRING_LENGTH + 1];
      char numBuf2[MAXIMUM_INTEGER_STRING_LENGTH + 1];
      memcpy(numBuf1, &value[value1Start], numString1Len);
      memcpy(numBuf2, &value[value2Start], numString2Len);
      numBuf1[numString1Len] = '\0';
      numBuf2[numString2Len] = '\0';
      rsequenceNum = atoi(numBuf1);
      csequenceNum = atoi(numBuf2);
      sequenceMethod.remove(0);
      sequenceMethod.append(value, value3Start, value3End - value3Start);
      NameValueTokenizer::frontBackTrim(&sequenceMethod, SIP_SUBFIELD_SEPARATORS);           
   }

   return value != NULL;
}

UtlBoolean SipMessage::getContactUri(int addressIndex, UtlString* uri) const
{
    return getContactField(addressIndex, *uri);
}

UtlBoolean SipMessage::getContactUri(int addressIndex, Url& contactField) const
{
   UtlString sContactField;
   UtlBoolean bRes = getContactField(addressIndex, sContactField);
   if (bRes)
   {
      contactField = Url(sContactField);
   }
   return bRes;
}

UtlBoolean SipMessage::getContactField(int addressIndex, UtlString& contactField) const
{
    const char* value = getHeaderValue(addressIndex, SIP_CONTACT_FIELD);
    contactField = value ? value : "";

    return(value != NULL);
}

// Make sure that the getContactEntry does the right thing for


UtlBoolean SipMessage::getContactEntry(int addressIndex, UtlString* uriAndParameters) const
{
   // return(getFieldSubfield(SIP_CONTACT_FIELD, addressIndex, uriAndParameters));

   UtlBoolean contactFound = FALSE;
   int currentHeaderFieldValue = 0;
   int currentEntryValue = 0;
   const char* value = NULL;

   while ( (value = getHeaderValue(currentHeaderFieldValue, SIP_CONTACT_FIELD)) &&
      (currentEntryValue <= addressIndex) )
   {
      uriAndParameters->remove(0);
      if(value)
      {
            #ifdef TEST_PRINT
                    osPrintf("SipMessage::getContactEntry addressIndex: %d\n", addressIndex);
            #endif
           //NameValueTokenizer::getSubField(value, addressIndex, ",",
         //    uriAndParameters);
           int addressStart = 0;
           int addressCount = 0;
           int charIndex = 0;
           int doubleQuoteCount = 0;

           while(1)
           {
               if(value[charIndex] == '"')
               {
                   doubleQuoteCount++;
                  #ifdef TEST_PRINT
                                  osPrintf("SipMessage::getContactEntry doubleQuoteCount parity:%d",
                                      doubleQuoteCount % 2);
                  #endif
               }

               // We found a comma that is not in the middle of a quoted string
               if(   (value[charIndex] == ',' || value[charIndex] == '\0')
                  && !(doubleQuoteCount % 2)
                  )
               {
                   if(currentEntryValue == addressIndex)
                   {

                       uriAndParameters->append(&value[addressStart], charIndex - addressStart);
                        #ifdef TEST_PRINT
                                            osPrintf("SipMessage::getContactEntry found contact[%d] starting: %d ending: %d \"%s\"\n",
                                                addressIndex, addressStart, charIndex, uriAndParameters->data());
                        #endif
                       currentEntryValue ++;
                       contactFound = TRUE;
                       break;
                   }
                   currentEntryValue ++;
                   addressStart = charIndex + 1;
                   addressCount++;
               }

               if(value[charIndex] == '\0') break;
               charIndex++;
           }
      }
      currentHeaderFieldValue++;
   }
   return(contactFound);
}


UtlBoolean SipMessage::getContactAddress(int addressIndex,
                                        UtlString* contactAddress,
                                        int* contactPort,
                                        UtlString* protocol,
                                    UtlString* user,
                                        UtlString* userLabel) const
{
    UtlString uri;
    UtlBoolean foundUri = getContactUri(addressIndex, &uri);

    if(foundUri) parseAddressFromUri(uri.data(), contactAddress,
                  contactPort, protocol,
                  user,
                  userLabel);

    return(foundUri);
}

UtlBoolean SipMessage::getRequireExtension(int extensionIndex,
                                UtlString* extension) const
{
   return(getFieldSubfield(SIP_REQUIRE_FIELD, extensionIndex, extension));
}

UtlBoolean SipMessage::getProxyRequireExtension(int extensionIndex,
                                                UtlString* extension) const
{
   return(getFieldSubfield(SIP_PROXY_REQUIRE_FIELD, extensionIndex, extension));
}

void SipMessage::addRequireExtension(const char* extension)
{
    addHeaderField(SIP_REQUIRE_FIELD, extension);
}

/// Retrieve the event type, id, and other params from the Event Header
UtlBoolean SipMessage::getEventField(UtlString* eventType,
                                     UtlString* eventId, //< set to the 'id' parameter value if not NULL
                                     UtlHashMap* params  //< holds parameter name/value pairs if not NULL
                                     ) const
{
   UtlString  eventField;
   UtlBoolean gotHeader = getEventField(eventField);

   if (NULL != eventId)
   {
      eventId->remove(0);
   }
   
   if (gotHeader)
   {
      NameValueTokenizer::getSubField(eventField, 0, ";", eventType);
      NameValueTokenizer::frontBackTrim(eventType, " \t");

      UtlString eventParam;
      for (int param_idx = 1;
           NameValueTokenizer::getSubField(eventField.data(), param_idx, ";", &eventParam);
           param_idx++
           )
      {
         UtlString name;
         UtlString value;
       
         NameValueTokenizer paramPair(eventParam);
         if (paramPair.getNextPair('=', &name, &value))
         {
            if (eventId && name.compareTo("id", UtlString::ignoreCase) == 0)
            {
               *eventId = value;
            }
            else if (NULL != params)
            {
               UtlString* returnedName  = new UtlString(name);
               UtlString* returnedValue = new UtlString(value);
               params->insertKeyAndValue( returnedName, returnedValue );
            }
         }
         else
         {
            OsSysLog::add(FAC_SIP,PRI_WARNING,"invalid event parameter '%s'", eventParam.data());
         }
      }
   }
   
   return gotHeader;
}


UtlBoolean SipMessage::getEventField(UtlString& eventField) const
{
   const char* value = getHeaderValue(0, SIP_EVENT_FIELD);
    eventField.remove(0);

   if(value)
   {
      eventField.append(value);
   }

   return(value != NULL);
}

void SipMessage::setEventField(const char* eventField, const char* id)
{
   UtlString sEventField(eventField);

   if (id && *id)
   {
      sEventField.appendFormat(";id=%s", id);
   }

   setHeaderValue(SIP_EVENT_FIELD, sEventField, 0);
}

UtlBoolean SipMessage::getExpiresField(int* expiresInSeconds) const
{
   const char* fieldValue = getHeaderValue(0, SIP_EXPIRES_FIELD);
   if(fieldValue)
   {
        UtlString subfieldText;
        NameValueTokenizer::getSubField(fieldValue, 1,
               " \t:;,", &subfieldText);

        //
        if(subfieldText.isNull())
        {
            *expiresInSeconds = atoi(fieldValue);
        }
        // If there is more than one token assume it is a text date
        else
        {
            long dateExpires = OsDateTime::convertHttpDateToEpoch(fieldValue);
            long dateSent = 0;
            // If the date was not set in the message
            if(!getDateField(&dateSent))
            {
#ifdef TEST
                osPrintf("Date field not set\n");
#endif

                // Assume date sent is now
                dateSent = OsDateTime::getSecsSinceEpoch();
            }
#ifdef TEST_PRINT
            osPrintf("Expires date: %ld\n", dateExpires);
            osPrintf("Current time: %ld\n", dateSent);
#endif

            *expiresInSeconds = dateExpires - dateSent;
        }
   }
   else
   {
      *expiresInSeconds = -1;
   }

   return(fieldValue != NULL);
}

UtlBoolean SipMessage::getRequestDispositionField(UtlString* dispositionField) const
{
   const char* value = getHeaderValue(0, SIP_REQUEST_DISPOSITION_FIELD);
    dispositionField->remove(0);

   if(value)
   {
      dispositionField->append(value);
   }

   return(value != NULL);
}

UtlBoolean SipMessage::getRequestDisposition(int tokenIndex,
                                UtlString* dispositionToken) const
{
   return(getFieldSubfield(SIP_REQUEST_DISPOSITION_FIELD, tokenIndex,
        dispositionToken));
}

UtlBoolean SipMessage::getRecordRouteField(int index, UtlString* recordRouteField) const
{
    const char* fieldValue = getHeaderValue(index, SIP_RECORD_ROUTE_FIELD);
    recordRouteField->remove(0);
    if(fieldValue) recordRouteField->append(fieldValue);
    return(fieldValue != NULL);
}

UtlBoolean SipMessage::getRecordRouteUri(int index, UtlString* recordRouteUri) const
{
    //UtlString recordRouteField;
    //UtlBoolean fieldExists = getRecordRouteField(&recordRouteField);
    //NameValueTokenizer::getSubField(recordRouteField.data(), index,
   //          ",", recordRouteUri);
    UtlBoolean fieldExists = getFieldSubfield(SIP_RECORD_ROUTE_FIELD, index, recordRouteUri);
    NameValueTokenizer::frontBackTrim(recordRouteUri, " \t");
    return(fieldExists && !recordRouteUri->isNull());
}

void SipMessage::setRecordRoutes(const SipMessage *inviteRequest)
{
  UtlString recordRouteField;
  int recordRouteIndex = 0;
  while(inviteRequest->getRecordRouteField(recordRouteIndex,
    &recordRouteField))
  {
    setRecordRouteField(recordRouteField.data(), recordRouteIndex);
    recordRouteIndex++;
  }
}

void SipMessage::setRecordRouteField(const char* recordRouteField,
                                     int index)
{
    setHeaderValue(SIP_RECORD_ROUTE_FIELD, recordRouteField, index);
}

void SipMessage::addRecordRouteUri(const char* recordRouteUri)
{
    UtlString recordRouteField;
    UtlString recordRouteUriString;

    /*int recordRouteIndex = 0;
    while(getRecordRouteField(recordRouteIndex, &recordRouteField))
    {
        recordRouteIndex++;
    }
    // Try to format the same as it came in either all the record route
    // URLs on the same line or a different field for each
    if(recordRouteIndex == 1)
    {
        recordRouteIndex--;
        getRecordRouteField(recordRouteIndex, &recordRouteField);
    }

    if(recordRouteUri && !recordRouteField.isNull())
    {
        recordRouteField.insert(0, ',');
    }*/
    if(recordRouteUri && strchr(recordRouteUri, '<') == NULL)
    {
        recordRouteUriString.append('<');
        recordRouteUriString.append(recordRouteUri);
        recordRouteUriString.append('>');
    }
    else if(recordRouteUri)
    {
        recordRouteUriString.append(recordRouteUri);
    }

    recordRouteField.insert(0, recordRouteUriString);
    //setRecordRouteField(recordRouteField.data(), recordRouteIndex);

    // Record route is always added on the top
    NameValuePair* headerField =
        new NameValuePair(SIP_RECORD_ROUTE_FIELD,
        recordRouteUriString.data());

    mHeaderCacheClean = FALSE;
   mNameValues.insertAt(0, headerField);
}

// isClientMsgStrictRouted returns whether or not a message
//    is set up such that it requires strict routing.
//    This is appropriate only when acting as a UAC
UtlBoolean SipMessage::isClientMsgStrictRouted() const
{
    UtlBoolean result;
    UtlString routeField;

    if ( getRouteField( &routeField ) )
    {
        Url routeUrl( routeField, TRUE );
        UtlString valueIgnored;

        // there is a route header, so see if it is loose routed or not
        result = ! routeUrl.getUrlParameter( "lr", valueIgnored );
    }
    else
    {
        result = FALSE;
    }

    return result;
}


UtlBoolean SipMessage::getRouteField(UtlString* routeField) const
{
    const char* fieldValue = getHeaderValue(0, SIP_ROUTE_FIELD);
    routeField->remove(0);
    if(fieldValue) routeField->append(fieldValue);
    return(fieldValue != NULL);
}

void SipMessage::addRouteUri(const char* routeUri)
{
   UtlString routeField;
   UtlString routeParameter;
    const char* bracketPtr = strchr(routeUri, '<');
   if(bracketPtr == NULL)
   {
      routeParameter.append('<');
   }
   routeParameter.append(routeUri);
    bracketPtr = strchr(routeUri, '>');
   if(bracketPtr == NULL)
   {
      routeParameter.append('>');
   }
    // If there is already a route header
   if(getRouteField( &routeField))
    {
      routeParameter.append(SIP_MULTIFIELD_SEPARATOR);

       // Remove the entire header
        removeHeader(SIP_ROUTE_FIELD, 0);
    }

   //add the updated header
   routeField.insert(0,routeParameter);
   setRouteField(routeField);
}

void SipMessage::addLastRouteUri(const char* routeUri)
{
    if(routeUri && *routeUri)
    {
        // THis is the cheap brut force way to do this
        int index = 0;
        const char* routeField = NULL;
        while ((routeField = getHeaderValue(index, SIP_ROUTE_FIELD)))
        {
            index++;
        }

        UtlString routeString(routeField ? routeField : "");
        if(routeField)
        {
            // Add a field separator
            routeString.append(SIP_MULTIFIELD_SEPARATOR);
        }
        // Make sure the route is in name-addr format
        if(strstr(routeUri,"<") <= 0)
        {
            routeString.append("<");
        }

        routeString.append(routeUri);
        if(strstr(routeUri, ">") <= 0)
        {
            routeString.append(">");
        }

        setHeaderValue(SIP_ROUTE_FIELD, routeString.data(), index);
    }
}

UtlBoolean SipMessage::getLastRouteUri(UtlString& routeUri,
                                      int& lastIndex)
{
    int index = 0;

    UtlString tempRoute;
    while(getFieldSubfield(SIP_ROUTE_FIELD, index, &tempRoute))
    {
        index++;
        routeUri = tempRoute;
    }

    index--;
    lastIndex = index;

    return(!routeUri.isNull());
}

UtlBoolean SipMessage::getRouteUri(int index, UtlString* routeUri) const
{
    UtlString routeField;
    UtlBoolean fieldExists = getFieldSubfield(SIP_ROUTE_FIELD, index, routeUri);
    return(fieldExists && !routeUri->isNull());
}

UtlBoolean SipMessage::removeRouteUri(int index, UtlString* routeUri)
{
    UtlString newRouteField;
    UtlString aRouteUri;
    UtlBoolean uriFound = FALSE;
    int uriIndex = 0;
    while(getFieldSubfield(SIP_ROUTE_FIELD, uriIndex, &aRouteUri))
    {
#ifdef TEST_PRINT
        osPrintf("removeRouteUri::routeUri[%d]: %s\n", uriIndex, newRouteField.data());
#endif
        if(uriIndex == index)
        {
            *routeUri = aRouteUri;
            uriFound = TRUE;
        }
        else
        {
            if(!newRouteField.isNull())
            {
                newRouteField.append(',');
            }
            int routeUriIndex = aRouteUri.index('<');
            if(routeUriIndex < 0)
            {
                aRouteUri.insert(0, '<');
                aRouteUri.append('>');
            }
            newRouteField.append(aRouteUri.data());
        }
#ifdef TEST_PRINT
        osPrintf("removeRouteUri::newRouteField: %s\n", newRouteField.data());
#endif
        uriIndex++;
    }

    // Remove all the old route headers.
    while(removeHeader(SIP_ROUTE_FIELD, 0))
    {
    }

    // Set the route field to contain the uri list with the indicated
    // uri removed.
    if(!newRouteField.isNull())
    {
        insertHeaderField(SIP_ROUTE_FIELD, newRouteField.data());
    }

    return(uriFound);
}

void SipMessage::setRouteField(const char* routeField)
{
#ifdef TEST_PRINT
    osPrintf("setRouteField: %s\n", routeField);
#endif
    if (routeField && strlen(routeField))
    {
        setHeaderValue(SIP_ROUTE_FIELD, routeField, 0);
    }
}

UtlBoolean SipMessage::buildRouteField(UtlString* routeFld) const
{
    UtlBoolean recordRouteFound = FALSE;
    UtlString contactUri;
   UtlString routeField;

    // If request build from recordRoute verbatum
    if(!isResponse())
    {
#ifdef TEST_PRINT
        osPrintf("SipMessage::buildRouteField recordRoute verbatum\n");
#endif
        //recordRouteFound = getRecordRouteField(routeField);
        int recordRouteIndex = 0;
        UtlString recordRouteUri;
        while(getRecordRouteUri(recordRouteIndex, &recordRouteUri))
        {
            if(!routeField.isNull())
            {
                routeField.append(',');
            }
            routeField.append(recordRouteUri.data());
#ifdef TEST_PRINT
            osPrintf("SipMessage::buildRouteField recordRouteUri[%d] %s\n",
                recordRouteIndex, recordRouteUri.data());
#endif
            recordRouteIndex++;
        }
        if(recordRouteIndex) recordRouteFound = TRUE;
    }

    // If response build from recordeRoute in reverse order
    else
    {
#ifdef TEST_PRINT
        osPrintf("SipMessage::buildRouteField recordRoute reverse\n");
#endif
        UtlString recordRouteUri;
        routeField.remove(0);
        int index = 0;
        int recordRouteUriIndex;
        while(getRecordRouteUri(index, &recordRouteUri))
        {
            recordRouteFound = TRUE;
            if(index > 0)
            {
                routeField.insert(0, ", ");
            }

            recordRouteUriIndex = recordRouteUri.index('<');
            if(recordRouteUriIndex < 0)
            {
                recordRouteUri.insert(0, '<');
                recordRouteUri.append('>');
            }

#ifdef TEST_PRINT
            osPrintf("SipMessage::buildRouteField recordRouteUri[%d] %s\n",
                index, recordRouteUri.data());
#endif
            routeField.insert(0, recordRouteUri.data());
            index++;
        }
    }

#ifdef LOOSE_ROUTE
    // In either case if contact is present add to the end of the route list.
    if(recordRouteFound && getContactUri(0, &contactUri))
    {
        routeField.append(", ");
        int contactUriIndex = contactUri.index('<');
        if(contactUriIndex < 0)
        {
            contactUri.insert(0, '<');
            contactUri.append('>');
        }
        routeField.append(contactUri.data());
    }
#endif

#ifdef TEST_PRINT
    osPrintf("buildRouteField: %s\n", routeField.data());
#endif

   if (recordRouteFound)
   {
      //clear the previous recourd route field and set it to the new one
      routeFld->remove(0);
      routeFld->append(routeField);
   }
    return(recordRouteFound);
}

void SipMessage::buildReplacesField(UtlString& replacesField,
                            const char* callId,
                            const char* fromField,
                            const char* toField)
{
    replacesField = callId;

    replacesField.append(";to-tag=");
    Url toUrl(toField);
    UtlString toTag;
    toUrl.getFieldParameter("tag", toTag);
    replacesField.append(toTag);

    replacesField.append(";from-tag=");
    Url fromUrl(fromField);
    UtlString fromTag;
    fromUrl.getFieldParameter("tag", fromTag);
    replacesField.append(fromTag);
}

UtlBoolean SipMessage::getFieldSubfield(const char* fieldName, int addressIndex, UtlString* uri) const
{
   UtlBoolean uriFound = FALSE;
   UtlString url;
   int fieldIndex = 0;
   int subFieldIndex = 0;
   int index = 0;
   const char* value = getHeaderValue(fieldIndex, fieldName);

   uri->remove(0);

   while(value && index <= addressIndex)
   {
      subFieldIndex = 0;
      NameValueTokenizer::getSubField(value, subFieldIndex, SIP_MULTIFIELD_SEPARATOR, &url);
#ifdef TEST
      osPrintf("Got field: \"%s\" subfield[%d]: %s\n", fieldName, fieldIndex, url.data());
#endif

      while(!url.isNull() && index < addressIndex)
      {
         subFieldIndex++;
         index++;
         NameValueTokenizer::getSubField(value, subFieldIndex,
            SIP_MULTIFIELD_SEPARATOR, &url);
#ifdef TEST
         osPrintf("Got field: \"%s\" subfield[%d]: %s\n", fieldName, fieldIndex, url.data());
#endif
      }

      if(index == addressIndex && !url.isNull())
      {
         uri->append(url.data());
         uriFound = TRUE;
            break;
      }
      else if(index > addressIndex)
      {
         break;
      }

      fieldIndex++;
      value = getHeaderValue(fieldIndex, fieldName);
   }
   return(uriFound);
}

/*UtlBoolean SipMessage::setFieldSubfield(const char* fieldName,
                              int addressIndex, const char* subFieldValue) const
{
   UtlBoolean uriFound = FALSE;
   UtlString url;
   int fieldIndex = 0;
   int subFieldIndex = 0;
   int index = 0;
   const char* value = getHeaderValue(fieldIndex, fieldName);

   uri->remove(0);

   while(value && index <= addressIndex)
   {
      subFieldIndex = 0;
      NameValueTokenizer::getSubField(value, subFieldIndex,
         SIP_MULTIFIELD_SEPARATOR, &url);
#ifdef TEST
      osPrintf("Got field: \"%s\" subfield[%d]: %s\n", fieldName,
         fieldIndex, url.data());
#endif

      while(!url.isNull() && index < addressIndex)
      {
         subFieldIndex++;
         index++;
         NameValueTokenizer::getSubField(value, subFieldIndex,
            SIP_MULTIFIELD_SEPARATOR, &url);
#ifdef TEST
         osPrintf("Got field: \"%s\" subfield[%d]: %s\n", fieldName,
            fieldIndex, url.data());
#endif
      }

      if(index == addressIndex && !url.isNull())
      {
         url.remove(0);
         url.append(subFieldValue.data());

         uriFound = TRUE;
            break;
      }
      else if(index > addressIndex)
      {
         break;
      }

      fieldIndex++;
      value = getHeaderValue(fieldIndex, fieldName);
   }

   return(uriFound);
}
*/

UtlBoolean SipMessage::getContentEncodingField(UtlString* contentEncodingField) const
{
   const char* value = getHeaderValue(0, SIP_CONTENT_ENCODING_FIELD);

   contentEncodingField->remove(0);
   if(value)
   {
      contentEncodingField->append(value);
   }
    return(value != NULL);
}

void SipMessage::setSubscriptionState(const UtlString& state,
                                      const UtlString& reason,
                                      int* expiresInSeconds,
                                      int* retryAfterSeconds)
{
   UtlString subscriptionState;
   subscriptionState.append(state);

   if (!reason.isNull())
   {
      subscriptionState.appendFormat(";reason=%s", reason.data());
   }
   if (expiresInSeconds)
   {
      subscriptionState.appendFormat(";expires=%d", *expiresInSeconds);
   }
   if (retryAfterSeconds)
   {
      subscriptionState.appendFormat(";retry-after=%d", *retryAfterSeconds);
   }

   setHeaderValue(SIP_SUBSCRIPTION_STATE_FIELD, subscriptionState, 0);
}

UtlBoolean SipMessage::getSubscriptionState(UtlString& state,
                                            UtlString& reason,
                                            int& expiresInSeconds,
                                            int& retryAfterSeconds) const
{
   const char* value;
   state.remove(0);
   expiresInSeconds = -1;
   retryAfterSeconds = -1;
   reason.remove(0);
   value = getHeaderValue(0, SIP_SUBSCRIPTION_STATE_FIELD);

   if(value && *value)
   {
      UtlBoolean bReasonFound = FALSE;
      UtlBoolean bExpiresFound = FALSE;
      UtlBoolean bRetryAfterFound = FALSE;
      UtlBoolean bFieldFound = TRUE;
      UtlString sFieldValue(value);
      UtlString sValue;
      int i = 1;
      NameValueTokenizer::frontBackTrim(&sFieldValue, SIP_SUBFIELD_SEPARATORS);
      if (!sFieldValue.isNull())
      {
         // get state value
         NameValueTokenizer::getSubField(sFieldValue, 0, ";", &state);
         NameValueTokenizer::frontBackTrim(&state, SIP_SUBFIELD_SEPARATORS);
         // get reason & expiresInSeconds & retryAfterSeconds
         while(bFieldFound && (!bReasonFound || !bExpiresFound || !bRetryAfterFound))
         {
            sValue.remove(0);
            bFieldFound = NameValueTokenizer::getSubField(sFieldValue, i++, ";", &sValue);
            if (bFieldFound)
            {
               NameValueTokenizer::frontBackTrim(&sValue, SIP_SUBFIELD_SEPARATORS);

               if (!bReasonFound && sValue.index("reason") == 0)
               {
                  NameValueTokenizer::getSubField(sValue, 1, "=", &reason);
                  NameValueTokenizer::frontBackTrim(&reason, "\t \"");
                  bReasonFound = TRUE;
               }
               else if (!bExpiresFound && sValue.index("expires") == 0)
               {
                  UtlString sExpires;
                  NameValueTokenizer::getSubField(sValue, 1, "=", &sExpires);
                  NameValueTokenizer::frontBackTrim(&sExpires, SIP_SUBFIELD_SEPARATORS);
                  expiresInSeconds = atoi(sExpires);
                  bExpiresFound = TRUE;
               }
               else if (!bRetryAfterFound && sValue.index("retry-after") == 0)
               {
                  UtlString sRetryAfter;
                  NameValueTokenizer::getSubField(sValue, 1, "=", &sRetryAfter);
                  NameValueTokenizer::frontBackTrim(&sRetryAfter, SIP_SUBFIELD_SEPARATORS);
                  retryAfterSeconds = atoi(sRetryAfter);
                  bRetryAfterFound = TRUE;
               }
            }
         }

         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::getSessionExpires(int* sessionExpiresSeconds, UtlString* refresher) const
{
    const char* value = getHeaderValue(0, SIP_SESSION_EXPIRES_FIELD);
    if (value)
    {
        *sessionExpiresSeconds = atoi(value);

        UtlString expiresParams(value) ;
        int iIndex = expiresParams.first(';') ;
        if (iIndex != UTL_NOT_FOUND)
        {
            expiresParams.remove(0, iIndex+1) ;
            NameValueTokenizer tokenizer(expiresParams.data()) ;
            UtlString name ;
            UtlString value ;            
            while (tokenizer.getNextPair('=', &name, &value))
            {
                if (name.compareTo("refresher", UtlString::ignoreCase) == 0)
                {
                    *refresher = value ;
                    break ;
                }
            }
        }               
    }
    else
    {
        *sessionExpiresSeconds = 0;
        refresher->remove(0) ;
    }

    return(value != NULL);
}

void SipMessage::setSessionExpires(int sessionExpiresSeconds, const UtlString& refresher)
{
   UtlString fieldValue;
   fieldValue.appendFormat("%d", sessionExpiresSeconds);

   if (!refresher.isNull())
   {
      fieldValue.appendFormat(";refresher=%s", refresher.data());
   }

   setHeaderValue(SIP_SESSION_EXPIRES_FIELD, fieldValue);
}

void SipMessage::setMinSe(int minSe)
{
   UtlString sMinSe;
   sMinSe.appendFormat("%d", minSe);
   setHeaderValue(SIP_MIN_SE_FIELD, sMinSe);
}

UtlBoolean SipMessage::getMinSe(int& minSe) const
{
   const char* value = getHeaderValue(0, SIP_MIN_SE_FIELD);
   if (value)
   {
      UtlString sMinSe;
      NameValueTokenizer::getSubField(value, 0, ";", &sMinSe);
      NameValueTokenizer::frontBackTrim(&sMinSe, SIP_SUBFIELD_SEPARATORS);
      if (!sMinSe.isNull())
      {
         minSe = atoi(sMinSe.data());
         return TRUE;
      }
   }
   else
   {
      minSe = 0;
   }

   return FALSE;
}

void SipMessage::setRetryAfterField(int periodSeconds)
{
   UtlString sValue;
   sValue.appendFormat("%d", periodSeconds);
   setHeaderValue(SIP_RETRY_AFTER_FIELD, sValue);
}

bool SipMessage::hasSelfHeader() const
{
   UtlString value;
   getUserAgentField(&value);
   if (value.isNull())
   {
      getServerField(&value);
   }
   return ! value.isNull();
}

void SipMessage::getServerField(UtlString* serverFieldValue) const
{
   const char* server = getHeaderValue(0, SIP_SERVER_FIELD);
   serverFieldValue->remove(0);
   if(server)
   {
      serverFieldValue->append(server);
   }
}

void SipMessage::setServerField(const char* serverField)
{
   setHeaderValue(SIP_SERVER_FIELD, serverField);
}

void SipMessage::setAcceptField(const char* acceptField)
{
   setHeaderValue(SIP_ACCEPT_FIELD, acceptField);
}

UtlBoolean SipMessage::getSupportedField(UtlString& supportedField) const
{
   supportedField.remove(0);
   const char* value = getHeaderValue(0, SIP_SUPPORTED_FIELD); // get 1st Supported field
   supportedField.append(value);

   return value != NULL;
}

void SipMessage::setSupportedField(const char* supportedField)
{
    setHeaderValue(SIP_SUPPORTED_FIELD, supportedField);
}

UtlBoolean SipMessage::isInSupportedField(const char* token) const
{
   UtlBoolean tokenFound = FALSE;
   UtlString url;
   int fieldIndex = 0;
   int subFieldIndex = 0;
   const char* value = getHeaderValue(fieldIndex, SIP_SUPPORTED_FIELD);

   while (value && !tokenFound)
   {
      subFieldIndex = 0;
      NameValueTokenizer::getSubField(value, subFieldIndex,
                                      SIP_MULTIFIELD_SEPARATOR, &url);
#ifdef TEST
      OsSysLog::add(FAC_SIP, PRI_DEBUG, "Got field: \"%s\" subfield[%d]: %s\n", value,
               fieldIndex, url.data());
#endif
      url.strip(UtlString::both);
      if (url.compareTo(token, UtlString::ignoreCase) == 0)
      {
         tokenFound = TRUE;
      }

      while (!url.isNull() && !tokenFound)
      {
         subFieldIndex++;
         NameValueTokenizer::getSubField(value, subFieldIndex,
                                         SIP_MULTIFIELD_SEPARATOR, &url);
         url.strip(UtlString::both);
#ifdef TEST
         OsSysLog::add(FAC_SIP, PRI_DEBUG, "Got field: \"%s\" subfield[%d]: %s\n", SIP_SUPPORTED_FIELD,
                  fieldIndex, url.data());
#endif

         if (url.compareTo(token, UtlString::ignoreCase) == 0)
         {
            tokenFound = TRUE;
         }
      }

      fieldIndex++;
      value = getHeaderValue(fieldIndex, SIP_SUPPORTED_FIELD);
   }

   return(tokenFound);
}

// Call control accessors
UtlBoolean SipMessage::getAlsoUri(int index, UtlString* alsoUri) const
{
    return(getFieldSubfield(SIP_ALSO_FIELD, index, alsoUri));
}

UtlBoolean SipMessage::getAlsoField(UtlString* alsoField) const
{
    const char* value = getHeaderValue(0, SIP_ALSO_FIELD);
    alsoField->remove(0);
    if(value) alsoField->append(value);
    return(value != NULL);
}

void SipMessage::setAlsoField(const char* alsoField)
{
    setHeaderValue(SIP_ALSO_FIELD, alsoField);
}

void SipMessage::addAlsoUri(const char* alsoUri)
{
    // Append to the field value already there, if it exists
    UtlString field;
    if(getAlsoField(&field) && !field.isNull())
    {
        field.append(SIP_MULTIFIELD_SEPARATOR);
      field.append(SIP_SINGLE_SPACE);
    }

    if(!strchr(alsoUri, '<')) field.append('<');
    field.append(alsoUri);
    if(!strchr(alsoUri, '>')) field.append('>');
    setAlsoField(field.data());
}

void SipMessage::setRequestedByField(const char* requestedByUri)
{
    setHeaderValue(SIP_REQUESTED_BY_FIELD, requestedByUri);
}

UtlBoolean SipMessage::getRequestedByField(UtlString& requestedByField) const
{
    const char* value = getHeaderValue(0, SIP_REQUESTED_BY_FIELD);
    requestedByField.remove(0);
    if(value) requestedByField.append(value);
    return(value != NULL);
}

void SipMessage::setReferToField(const char* referToField)
{
    setHeaderValue(SIP_REFER_TO_FIELD, referToField);
}

UtlBoolean SipMessage::getReferToField(UtlString& referToField) const
{
    const char* value = getHeaderValue(0, SIP_REFER_TO_FIELD);
    referToField.remove(0);
    if(value) referToField.append(value);
    return(value != NULL);
}

UtlBoolean SipMessage::getReferSubField(UtlBoolean& referSubField) const
{
   referSubField = TRUE;

   const char* value = getHeaderValue(0, SIP_REFER_SUB_FIELD);
   if (value && *value)
   {
      UtlString sReferSubField(value);
      NameValueTokenizer::frontBackTrim(&sReferSubField, " \t");
      if (sReferSubField.compareTo("false") == 0)
      {
         referSubField = FALSE;
      }

      return TRUE;
   }

   return FALSE;
}

void SipMessage::setReferSubField(UtlBoolean referSubField)
{
   setHeaderValue(SIP_REFER_SUB_FIELD, referSubField ? "true" : "false");
}

void SipMessage::setReferredByField(const char* referredByField)
{
    setHeaderValue(SIP_REFERRED_BY_FIELD, referredByField);
}

UtlBoolean SipMessage::getReferredByField(UtlString& referredByField) const
{
    const char* value = getHeaderValue(0, SIP_REFERRED_BY_FIELD);
    referredByField.remove(0);
    if(value) referredByField.append(value);
    return(value != NULL);
}

UtlBoolean SipMessage::getReferredByUrls(UtlString* referrerUrl,
                                        UtlString* referredToUrl) const
{
    if(referrerUrl) referrerUrl->remove(0);
    if(referredToUrl) referredToUrl->remove(0);
    const char* value = getHeaderValue(0, SIP_REFERRED_BY_FIELD);
    if(value)
    {
        // The first element is the referrer URL
        if(referrerUrl) NameValueTokenizer::getSubField(value, 0,
            ";", referrerUrl);

        // The second element is the referred to URL
        if(referredToUrl) NameValueTokenizer::getSubField(value, 1,
            ";", referredToUrl);
    }
    return(value != NULL);
}

void SipMessage::setReplacesField(const char* replacesField)
{
   setHeaderValue(SIP_REPLACES_FIELD, replacesField);
}

UtlBoolean SipMessage::getReplacesData(UtlString& callId,
                                      UtlString& toTag,
                                      UtlString& fromTag) const
{
    const char* replacesField = getHeaderValue(0, SIP_REPLACES_FIELD);

    UtlString parameter;
    UtlString name;
    UtlString value("");
    int parameterIndex = 1;

   if (replacesField)
    {
        // Get the callId
       NameValueTokenizer::getSubField(replacesField, 0,
                   ";", &callId);
       NameValueTokenizer::frontBackTrim(&callId, " \t");

       // Look through the rest of the parameters
       do
       {
          // Get a name value pair
          NameValueTokenizer::getSubField(replacesField, parameterIndex,
                   ";", &parameter);


          // Parse out the parameter name
          NameValueTokenizer::getSubField(parameter.data(), 0,
                                  "=", &name);
          name.toLower();
          NameValueTokenizer::frontBackTrim(&name, " \t");

          // Parse out the parameter value
          NameValueTokenizer::getSubField(parameter.data(), 1,
                                  "=", &value);
          NameValueTokenizer::frontBackTrim(&value, " \t");

          // Set the to and from tags when we find them
          if(name.compareTo("to-tag") == 0)
          {
             toTag = value;
          }
          else if(name.compareTo("from-tag") == 0)
          {
             fromTag = value;
          }

          parameterIndex++;
       } while(!parameter.isNull());
    }

    return (replacesField != NULL) ;
}

void SipMessage::setAllowField(const char* allowField)
{
    setHeaderValue(SIP_ALLOW_FIELD, allowField);
}

UtlBoolean SipMessage::getAllowField(UtlString& allowField) const
{
    const char* value;
    int allowIndex = 0;
    allowField.remove(0);
    while ((value = getHeaderValue(allowIndex, SIP_ALLOW_FIELD)))
    {
        if(value && *value)
        {
            if(!allowField.isNull()) allowField.append(", ");
            allowField.append(value);
        }
        allowIndex++;
    }
    return !allowField.isNull();
}

///// RFC 3326 ///
void SipMessage::setReasonField(const char* reasonField)
{
    if(NULL != reasonField)
    {
	setHeaderValue(SIP_REASON_FIELD, reasonField);
    }
}

void SipMessage::setReasonField(const UtlString& protocol, int cause, const UtlString& text)
{
   if (!protocol.isNull() && (cause || !text.isNull()))
   {
      UtlString reasonField;
      reasonField.append(protocol.data());

      if (cause)
      {
         reasonField.appendFormat(" ;cause=%d", cause);
      }
      if (!text.isNull())
      {
         reasonField.appendFormat(" ;text=\"%s\"", text.data());
      }

      setHeaderValue(SIP_REASON_FIELD, reasonField);
   }
}

UtlBoolean SipMessage::getReasonField(UtlString& reasonField) const
{
    const char* value;
    reasonField.remove(0);
    value = getHeaderValue(0, SIP_REASON_FIELD);
    if(value && *value)
    {
        reasonField.append(value);
	 NameValueTokenizer::frontBackTrim(&reasonField, " \t\n\r");
    }
    return(value != NULL);
}

UtlBoolean SipMessage::getJoinField(UtlString& joinField) const
{
   const char* value;
   joinField.remove(0);
   value = getHeaderValue(0, SIP_JOIN_FIELD);

   if(value && *value)
   {
      joinField.append(value);
      NameValueTokenizer::frontBackTrim(&joinField, SIP_SUBFIELD_SEPARATORS);
      return TRUE;
   }

   return FALSE;
}

UtlBoolean SipMessage::getJoinField(UtlString& sipCallId, UtlString& fromTag, UtlString& toTag) const
{
   const char* value;
   sipCallId.remove(0);
   fromTag.remove(0);
   toTag.remove(0);
   value = getHeaderValue(0, SIP_JOIN_FIELD);

   if(value && *value)
   {
      UtlBoolean bFromTagFound = FALSE;
      UtlBoolean bToTagFound = FALSE;
      UtlBoolean bFieldFound = TRUE;
      UtlString sJoinValue(value);
      NameValueTokenizer::frontBackTrim(&sJoinValue, SIP_SUBFIELD_SEPARATORS);

      if (!sJoinValue.isNull())
      {
         // get call-id value
         NameValueTokenizer::getSubField(sJoinValue, 0, ";", &sipCallId);
         NameValueTokenizer::frontBackTrim(&sipCallId, SIP_SUBFIELD_SEPARATORS);
         // get from and to tags
         UtlString sValue;
         int i = 1;
         while(bFieldFound && (!bFromTagFound || !bToTagFound))
         {
            sValue.remove(0);
            bFieldFound = NameValueTokenizer::getSubField(sJoinValue, i++, ";", &sValue);
            if (bFieldFound)
            {
               NameValueTokenizer::frontBackTrim(&sValue, SIP_SUBFIELD_SEPARATORS);

               if (!bFromTagFound && sValue.index("from-tag") == 0)
               {
                  NameValueTokenizer::getSubField(sValue, 1, "=", &fromTag);
                  NameValueTokenizer::frontBackTrim(&fromTag, SIP_SUBFIELD_SEPARATORS);
                  bFromTagFound = TRUE;
               }
               else if (!bToTagFound && sValue.index("to-tag") == 0)
               {
                  NameValueTokenizer::getSubField(sValue, 1, "=", &toTag);
                  NameValueTokenizer::frontBackTrim(&toTag, SIP_SUBFIELD_SEPARATORS);
                  bToTagFound = TRUE;
               }
            }
         }

         return TRUE;
      }
   }

   return FALSE;
}

void SipMessage::setJoinField(const UtlString& sipCallId, const UtlString& fromTag, const UtlString& toTag)
{
   UtlString joinField;

   // Example:
   // Join: 12adf2f34456gs5;to-tag=12345;from-tag=54321
   joinField.appendFormat("%s;to-tag=%s;from-tag=%s", sipCallId.data(),
      toTag.data() != NULL ? toTag.data() : "0",
      fromTag.data() != NULL ? fromTag.data() : "0");

   setHeaderValue(SIP_JOIN_FIELD, joinField);
}

UtlBoolean SipMessage::getReasonField(int index, UtlString& protocol, int& cause, UtlString& text) const
{
   const char* value;
   protocol.remove(0);
   cause = 0;
   text.remove(0);
   value = getHeaderValue(0, SIP_REASON_FIELD);

   if(value && *value)
   {
      UtlBoolean bCauseFound = FALSE;
      UtlBoolean bTextFound = FALSE;
      UtlBoolean bFieldFound = TRUE;
      UtlString sReasonValue;
      UtlString sValue;
      int i = 1;
      NameValueTokenizer::getSubField(value, index, ",", &sReasonValue);
      NameValueTokenizer::frontBackTrim(&sReasonValue, SIP_SUBFIELD_SEPARATORS);
      if (!sReasonValue.isNull())
      {
         // get protocol value
         NameValueTokenizer::getSubField(sReasonValue, 0, ";", &protocol);
         NameValueTokenizer::frontBackTrim(&protocol, SIP_SUBFIELD_SEPARATORS);
         // get cause & text
         while(bFieldFound && (!bCauseFound || !bTextFound))
         {
            sValue.remove(0);
            bFieldFound = NameValueTokenizer::getSubField(sReasonValue, i++, ";", &sValue);
            if (bFieldFound)
            {
               NameValueTokenizer::frontBackTrim(&sValue, SIP_SUBFIELD_SEPARATORS);

               if (!bCauseFound && sValue.index("cause") == 0)
               {
                  UtlString sCause;
                  NameValueTokenizer::getSubField(sValue, 1, "=", &sCause);
                  NameValueTokenizer::frontBackTrim(&sCause, SIP_SUBFIELD_SEPARATORS);
                  cause = atoi(sCause);
                  bCauseFound = TRUE;
               }
               else if (!bTextFound && sValue.index("text") == 0)
               {
                  NameValueTokenizer::getSubField(sValue, 1, "=", &text);
                  NameValueTokenizer::frontBackTrim(&text, "\t \"");
                  bTextFound = TRUE;
               }
            }
         }

         return TRUE;
      }
   }

   return FALSE;
}

////////////////////////////////




// for Diversion-header
void SipMessage::addDiversionField(const char* addr, const char* reasonParam, UtlBoolean afterOtherDiversions)
{
   if (NULL == addr || NULL == reasonParam)
   {
	return;
   }

   char diversionString[255];
   SNPRINTF(diversionString, sizeof(diversionString), "%s;reason=%s",addr,reasonParam);
   
   addDiversionField(diversionString,afterOtherDiversions);
}

void SipMessage::addDiversionField(const char* diversionField, UtlBoolean afterOtherDiversions)
{
    if(NULL != diversionField)
    {
       mHeaderCacheClean = FALSE;

       NameValuePair* nv = new NameValuePair(SIP_DIVERSION_FIELD, diversionField);
       // Look for other diversion fields
       size_t fieldIndex = mNameValues.index(nv);

#  ifdef TEST_PRINT
	if(fieldIndex == UTL_NOT_FOUND)
	{
        UtlDListIterator iterator(nameValues);

        	remove whole line
       	NameValuePair* nv = NULL;
		while(nv = (NameValuePair*) iterator())
        	{
            		osPrintf("\tName: %s\n", nv->data());
        	}
	}
#  endif

	mHeaderCacheClean = FALSE;

	if(fieldIndex == UTL_NOT_FOUND || afterOtherDiversions)
	{
		mNameValues.insert(nv);
	}
	else
	{
		mNameValues.insertAt(fieldIndex, nv);
	}
    }
}

UtlBoolean SipMessage::getLastDiversionField(UtlString& diversionField,
                                      int& lastIndex)
{
    int index = 0;

    UtlString tempDiversion;
    while(getFieldSubfield(SIP_DIVERSION_FIELD, index, &tempDiversion))
    {
        index++;
        diversionField = tempDiversion;
    }

    index--;
    lastIndex = index;

    return(!diversionField.isNull());
}
	
UtlBoolean SipMessage::getDiversionField(int index, UtlString& diversionField) 
{
    diversionField.remove(0);

    return getFieldSubfield(SIP_DIVERSION_FIELD,index,&diversionField);
}

UtlBoolean SipMessage::getDiversionField(int index, UtlString& addr, UtlString& reasonParam) 
{
    UtlString divString;
    addr.remove(0);
    reasonParam.remove(0);
	
    if (getFieldSubfield(SIP_DIVERSION_FIELD,index,&divString))
    {
                            
         // Parse addr
	  int parameterIndex = divString.index(";");
	  if(parameterIndex > 0)
	  {
		addr.append(divString);
		addr.remove(parameterIndex);
		NameValueTokenizer::frontBackTrim(&addr, " \t\n\r");
		
		// Parse reason
	       int reasonIndex = divString.index("reason=", 0, UtlString::ignoreCase); // wdn ??? - RWString::ignoreCase

	       if(reasonIndex > parameterIndex)
	       {
		    reasonParam.append(&((divString.data())[reasonIndex + 7]));

		    int endIndex = reasonParam.length()-1;
		    int semicolonIndex = reasonParam.index(";");
		    if (endIndex > semicolonIndex && semicolonIndex > 0)
		    {
			endIndex = semicolonIndex;
 		       reasonParam.remove(endIndex);
			NameValueTokenizer::frontBackTrim(&reasonParam, " \t\n\r");
		    }

	       }
	  }
	  else if(parameterIndex)
	  {
		addr.append(divString);
		NameValueTokenizer::frontBackTrim(&addr, " \t\n\r");
	  }
 	  else
 	  {
 	  	return FALSE;
 	  }
	  return TRUE;
    }
    return(FALSE);

}
	

/* ============================ INQUIRY =================================== */

UtlBoolean SipMessage::isResponse() const
{
   UtlBoolean responseType = FALSE;
   //UtlString firstHeaderField;

   //getFirstHeaderLinePart(0, &firstHeaderField);
   if(mFirstHeaderLine.index(SIP_PROTOCOL_VERSION) == 0)
   {
      responseType = TRUE;
   }

   return(responseType);
}

UtlBoolean SipMessage::is100RelResponse() const
{
   if (isResponse())
   {
      int statusCode = getResponseStatusCode();

      if (statusCode > SIP_1XX_CLASS_CODE && // 100 cannot be sent reliably
          statusCode < SIP_2XX_CLASS_CODE &&
          isRequireExtensionSet(SIP_PRACK_EXTENSION))
      {
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::isRequest() const
{
   return !isResponse();
}

UtlBoolean SipMessage::isPrackRequest() const
{
   if (isRequest())
   {
      UtlString requestMethod;
      getRequestMethod(&requestMethod);

      if (requestMethod.compareTo(SIP_PRACK_METHOD) == 0)
      {
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::isInviteRequest() const
{
   if (isRequest())
   {
      UtlString requestMethod;
      getRequestMethod(&requestMethod);

      if (requestMethod.compareTo(SIP_INVITE_METHOD) == 0)
      {
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::isCancelRequest() const
{
   if (isRequest())
   {
      UtlString requestMethod;
      getRequestMethod(&requestMethod);

      if (requestMethod.compareTo(SIP_CANCEL_METHOD) == 0)
      {
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::isByeRequest() const
{
   if (isRequest())
   {
      UtlString requestMethod;
      getRequestMethod(&requestMethod);

      if (requestMethod.compareTo(SIP_BYE_METHOD) == 0)
      {
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::isOptionsRequest() const
{
   if (isRequest())
   {
      UtlString requestMethod;
      getRequestMethod(&requestMethod);

      if (requestMethod.compareTo(SIP_OPTIONS_METHOD) == 0)
      {
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::isReferRequest() const
{
   if (isRequest())
   {
      UtlString requestMethod;
      getRequestMethod(&requestMethod);

      if (requestMethod.compareTo(SIP_REFER_METHOD) == 0)
      {
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::isUpdateRequest() const
{
   if (isRequest())
   {
      UtlString requestMethod;
      getRequestMethod(&requestMethod);

      if (requestMethod.compareTo(SIP_UPDATE_METHOD) == 0)
      {
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::isInfoRequest() const
{
   if (isRequest())
   {
      UtlString requestMethod;
      getRequestMethod(&requestMethod);

      if (requestMethod.compareTo(SIP_INFO_METHOD) == 0)
      {
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::isSubscribeRequest() const
{
   if (isRequest())
   {
      UtlString requestMethod;
      getRequestMethod(&requestMethod);

      if (requestMethod.compareTo(SIP_SUBSCRIBE_METHOD) == 0)
      {
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::isNotifyRequest() const
{
   if (isRequest())
   {
      UtlString requestMethod;
      getRequestMethod(&requestMethod);

      if (requestMethod.compareTo(SIP_NOTIFY_METHOD) == 0)
      {
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::isRegisterRequest() const
{
   if (isRequest())
   {
      UtlString requestMethod;
      getRequestMethod(&requestMethod);

      if (requestMethod.compareTo(SIP_REGISTER_METHOD) == 0)
      {
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::isAckRequest() const
{
   if (isRequest())
   {
      UtlString requestMethod;
      getRequestMethod(&requestMethod);

      if (requestMethod.compareTo(SIP_ACK_METHOD) == 0)
      {
         return TRUE;
      }
   }

   return FALSE;
}

UtlBoolean SipMessage::isInviteDialogUsage() const
{
   UtlString requestMethod;

   if (isRequest())
   {
      getRequestMethod(&requestMethod);
   }
   else
   {
      int seqNum;
      getCSeqField(seqNum, requestMethod);
   }

   if (requestMethod.compareTo(SIP_INVITE_METHOD) == 0 ||
      requestMethod.compareTo(SIP_UPDATE_METHOD) == 0 ||
      requestMethod.compareTo(SIP_PRACK_METHOD) == 0 ||
      requestMethod.compareTo(SIP_INFO_METHOD) == 0 ||
      requestMethod.compareTo(SIP_BYE_METHOD) == 0)
   {
      return TRUE;
   }

   return FALSE;
}

UtlBoolean SipMessage::isSubscribeDialogUsage() const
{
   UtlString requestMethod;

   if (isRequest())
   {
      getRequestMethod(&requestMethod);
   }
   else
   {
      int seqNum;
      getCSeqField(seqNum, requestMethod);
   }

   if (requestMethod.compareTo(SIP_SUBSCRIBE_METHOD) == 0 ||
      requestMethod.compareTo(SIP_NOTIFY_METHOD) == 0 ||
      requestMethod.compareTo(SIP_REFER_METHOD) == 0)
   {
      return TRUE;
   }

   return FALSE;
}

UtlBoolean SipMessage::isTargetRefresh() const
{
   UtlString requestMethod;

   if (isRequest())
   {
      getRequestMethod(&requestMethod);
   }
   else
   {
      int seqNum;
      getCSeqField(seqNum, requestMethod);
   }

   if (requestMethod.compareTo(SIP_INVITE_METHOD) == 0 ||
      requestMethod.compareTo(SIP_UPDATE_METHOD) == 0 ||
      requestMethod.compareTo(SIP_SUBSCRIBE_METHOD) == 0 ||
      requestMethod.compareTo(SIP_NOTIFY_METHOD) == 0 ||
      requestMethod.compareTo(SIP_REFER_METHOD) == 0)
   {
      return TRUE;
   }

   return FALSE;
}

UtlBoolean SipMessage::isServerTransaction(UtlBoolean isOutgoing) const
{
    UtlBoolean returnCode;

    if(isResponse())
    {
        if(isOutgoing)
        {
            returnCode = TRUE;
        }
        else
        {
            returnCode = FALSE;
        }
    }
    else
    {
        if(isOutgoing)
        {
            returnCode = FALSE;
        }
        else
        {
            returnCode = TRUE;
        }
    }

    return(returnCode);
}

UtlBoolean SipMessage::isSameMessage(const SipMessage* message,
                           UtlBoolean responseCodesMustMatch) const
{
   UtlBoolean isSame = FALSE;
   UtlString thisMethod, thatMethod;
   int thisSequenceNum, thatSequenceNum;
   UtlString thisSequenceMethod, thatSequenceMethod;

   if(message)
   {
      // Compare the method, To, From, CallId, Sequence number and
      // sequence method
      UtlBoolean thatIsResponse = message->isResponse();
      UtlBoolean thisIsResponse = isResponse();
      int thisResponseCode = 38743;
      int thatResponseCode = 49276;

      // Both are responses or requests
      if(thatIsResponse == thisIsResponse)
      {
         if(!thisIsResponse)
         {
            getRequestMethod(&thisMethod);
            message->getRequestMethod(&thatMethod);
         }
         else
         {
            thisResponseCode = getResponseStatusCode();
            thatResponseCode = message->getResponseStatusCode();
         }
         if( (thisIsResponse && !responseCodesMustMatch) ||
            (thisIsResponse && responseCodesMustMatch &&
               thisResponseCode == thatResponseCode) ||
            (!thisIsResponse && thisMethod.compareTo(thatMethod) == 0))
         {
            if(isSameSession(message))
            {
               getCSeqField(&thisSequenceNum, &thisSequenceMethod);
               message->getCSeqField(&thatSequenceNum, &thatSequenceMethod);
               if(thisSequenceNum == thatSequenceNum &&
                  thisSequenceMethod.compareTo(thatSequenceMethod) == 0)
               {
                  isSame = TRUE;
               }

            }
         }
      }
   }

   return(isSame);
}

UtlBoolean SipMessage::isSameSession(const SipMessage* message) const
{
   UtlBoolean isSame = FALSE;
    UtlBoolean isSameFrom = FALSE;
   UtlString thisTo, thatTo;
   UtlString thisFrom, thatFrom;
   UtlString thisCallId, thatCallId;

   // Messages from the same session have the same To, From and CallId
   if(message)
   {
      getCallIdField(&thisCallId);
      message->getCallIdField(&thatCallId);
      if(thisCallId.compareTo(thatCallId) == 0)
      {
         getFromField(&thisFrom);
         message->getFromField(&thatFrom);
         if(thisFrom.compareTo(thatFrom) == 0)
         {
                isSameFrom = TRUE;

            }
            else
            {
                UtlString thisAddress;
            UtlString thatAddress;
            int thisPort;
            int thatPort;
            UtlString thisProtocol;
            UtlString thatProtocol;
            UtlString thisUser;
            UtlString thatUser;
            UtlString thisUserLabel;
            UtlString thatUserLabel;
            UtlString thisTag;
            UtlString thatTag;
            getFromAddress(&thisAddress, &thisPort, &thisProtocol,
               &thisUser, &thisUserLabel, &thisTag);
            message->getFromAddress(&thatAddress, &thatPort, &thatProtocol,
               &thatUser, &thatUserLabel, &thatTag);

            if (thisAddress.compareTo(thatAddress) == 0 &&
                (thisPort == thatPort ||
                 (thisPort == PORT_NONE && thatPort == SIP_PORT) ||
                 (thisPort == SIP_PORT && thatPort == PORT_NONE)) &&
                thisProtocol.compareTo(thatProtocol) == 0 &&
                thisUser.compareTo(thatUser) == 0 &&
                (thisTag.compareTo(thatTag, UtlString::ignoreCase) == 0 ||
                 (thisTag.isNull() && !isResponse()) ||
                 (thatTag.isNull() && !message->isResponse())))
            {
               isSameFrom = TRUE;
            }
                else
                {
#ifdef TEST_PRINT
                    osPrintf("ERROR: From field did not match: \nAddr: (%s!=%s)\nPort: %d!=%d\nUser: (%s!=%s)\nTag:  (%s!=%s)\n",
                        thisAddress.data(), thatAddress.data(),
                        thisPort, thatPort,
                        thisUser.data(), thatUser.data(),
                        thisTag.data(), thatTag.data());
#endif
                }
            }

         getToField(&thisTo);
         message->getToField(&thatTo);
         if(isSameFrom && thisTo.compareTo(thatTo ) == 0)
         {
            isSame = TRUE;
         }
         // Check for tag
         else if(isSameFrom)
         {
            UtlString thisAddress;
            UtlString thatAddress;
            int thisPort;
            int thatPort;
            UtlString thisProtocol;
            UtlString thatProtocol;
            UtlString thisUser;
            UtlString thatUser;
            UtlString thisUserLabel;
            UtlString thatUserLabel;
            UtlString thisTag;
            UtlString thatTag;
            getToAddress(&thisAddress, &thisPort, &thisProtocol,
               &thisUser, &thisUserLabel, &thisTag);
            message->getToAddress(&thatAddress, &thatPort, &thatProtocol,
               &thatUser, &thatUserLabel, &thatTag);

            // Everything must match with the exception that the
            // request may not have the tag set.
            if (thisAddress.compareTo(thatAddress) == 0 &&
                (thisPort == thatPort ||
                 (thisPort == PORT_NONE && thatPort == SIP_PORT) ||
                 (thisPort == SIP_PORT && thatPort == PORT_NONE)) &&
                thisProtocol.compareTo(thatProtocol) == 0 &&
                thisUser.compareTo(thatUser) == 0 &&
                (thisTag.compareTo(thatTag , UtlString::ignoreCase) == 0 ||
                 (thisTag.isNull() && !isResponse()) ||
                 (thatTag.isNull() && !message->isResponse())))
            {
               isSame = TRUE;
            }
                else
                {
#ifdef TEST_PRINT
                    osPrintf("ERROR: To field did not match:\n: (%s!=%s)\nPort: %d!=%d\nUser: (%s!=%s)\nTag:  (%s!=%s)\n",
                        thisAddress.data(), thatAddress.data(),
                        thisPort, thatPort,
                        thisUser.data(), thatUser.data(),
                        thisTag.data(), thatTag.data());
#endif
                }
         }
      }
   }
   return(isSame);
}

UtlBoolean SipMessage::isSameSession(Url& oldUrl, Url& newUrl)
{
   UtlBoolean isSame = FALSE;


    UtlString thisAddress;
   UtlString thatAddress;
   int thisPort;
   int thatPort;
   UtlString thisProtocol;
   UtlString thatProtocol;
   UtlString thisUser;
   UtlString thatUser;
   UtlString thisTag;
   UtlString thatTag;
    oldUrl.getHostAddress(thisAddress);
    newUrl.getHostAddress(thatAddress);
    thisPort = oldUrl.getHostPort();
    thatPort = newUrl.getHostPort();
    oldUrl.getUserId(thisUser);
    newUrl.getUserId(thatUser);
    oldUrl.getUrlParameter("transport", thisProtocol);
    newUrl.getUrlParameter("transport", thatProtocol);
    oldUrl.getFieldParameter("tag", thisTag);
    newUrl.getFieldParameter("tag", thatTag);

   if(thisAddress.compareTo(thatAddress) == 0 &&
   (thisPort == thatPort ||
      (thisPort == 0 && thatPort == SIP_PORT) ||
         (thisPort == SIP_PORT && thatPort == 0)) &&
   thisProtocol.compareTo(thatProtocol) == 0 &&
   thisUser.compareTo(thatUser) == 0 &&
   (thisTag.compareTo(thatTag , UtlString::ignoreCase) == 0 || thisTag.isNull()))
         // Allow the old tag to be NULL
            // We do not allow only the new tag to be NULL as
            // this will cause some false matches.  Both may be NULL.*/
   {
      isSame = TRUE;
   }
    else
    {
#ifdef TEST_PRINT
        osPrintf("SipMessage::isSameSession Url did not match: \nAddr: (%s!=%s)\nPort: %d!=%d\nUser: (%s!=%s)\nTag:  (%s!=%s)\n",
            thisAddress.data(), thatAddress.data(),
            thisPort, thatPort,
            thisUser.data(), thatUser.data(),
            thisTag.data(), thatTag.data());
#endif
    }

   return(isSame);
}

UtlBoolean SipMessage::isResponseTo(const SipMessage* request) const
{
   UtlBoolean isPair = FALSE;
   UtlString thisMethod, thatMethod;
   int thisSequenceNum, thatSequenceNum;
   UtlString thisSequenceMethod, thatSequenceMethod;

   // If this is a response and request is a request
   if(request && !request->isResponse() && isResponse())
   {
      // Compare the To, From, CallId, Sequence number and
      // sequence method
      if(isSameSession(request))
      {
         getCSeqField(&thisSequenceNum, &thisSequenceMethod);
         request->getCSeqField(&thatSequenceNum, &thatSequenceMethod);
         if(thisSequenceNum == thatSequenceNum &&
            thisSequenceMethod.compareTo(thatSequenceMethod) == 0)
         {
            isPair = TRUE;
         }

      }

   }

   return(isPair);
}

UtlBoolean SipMessage::isAckFor(const SipMessage* inviteResponse) const
{
   UtlBoolean isPair = FALSE;
   UtlString thisMethod;
   int thisSequenceNum, thatSequenceNum;
   UtlString thisSequenceMethod, thatSequenceMethod;

   // If this is an ACK request and that is an INVITE response
   if(inviteResponse && inviteResponse->isResponse() && !isResponse())
   {
      getRequestMethod(&thisMethod);
      // Compare the To, From, CallId, Sequence number and  sequence method
      if(thisMethod.compareTo(SIP_ACK_METHOD) == 0 && isSameSession(inviteResponse))
      {
         getCSeqField(&thisSequenceNum, &thisSequenceMethod);
         inviteResponse->getCSeqField(&thatSequenceNum, &thatSequenceMethod);
         if(thisSequenceNum == thatSequenceNum &&
            thatSequenceMethod.compareTo(SIP_INVITE_METHOD) == 0)
         {
            isPair = TRUE;
         }
      }
   }

   return(isPair);
}
//SDUA
UtlBoolean SipMessage::isInviteFor(const SipMessage* cancelRequest) const
{
   UtlBoolean isPair = FALSE;
   UtlString thisMethod;
   // If this is an CANCEL request and that is an INVITE response
   if(cancelRequest && !isResponse())
   {
      getRequestMethod(&thisMethod);
      // Compare the To, From, CallId, Sequence number and  sequence method
      if(thisMethod.compareTo( SIP_INVITE_METHOD) == 0 && isSameTransaction(cancelRequest))
         isPair = TRUE;
   }
   return(isPair);
}

UtlBoolean SipMessage::isSameTransaction(const SipMessage* message) const
{
   // Compare the To, From, CallId, Sequence number and  sequence method
   UtlBoolean isPair = FALSE;
   int thisSequenceNum, thatSequenceNum;
   UtlString thisSequenceMethod, thatSequenceMethod;

   if( isSameSession(message))
   {
      getCSeqField(&thisSequenceNum, &thisSequenceMethod);
      message->getCSeqField(&thatSequenceNum, &thatSequenceMethod);
      if(thisSequenceNum == thatSequenceNum )
      {
         isPair = TRUE;
      }
   }
   return(isPair);
}


UtlBoolean SipMessage::isRequestDispositionSet(const char* dispositionToken) const
{
    UtlString field;
    int tokenIndex = 0;
    UtlBoolean matchFound = FALSE;
    while(getRequestDisposition(tokenIndex, &field))
    {
        field.toUpper();
        if(field.compareTo(dispositionToken) == 0)
        {
            matchFound = TRUE;
            break;
        }
    }

    return(matchFound);
}

UtlBoolean SipMessage::isRequireExtensionSet(const char* extension) const
{
    UtlString extensionString;
    UtlBoolean alreadySet = FALSE;
    int extensionIndex = 0;
    while(getRequireExtension(extensionIndex++, &extensionString))
    {
        extensionString.toLower();
        if(extensionString.compareTo(extension) == 0)
        {
            alreadySet = TRUE;
        }

    }
    return(alreadySet);
}

UtlBoolean SipMessage::isRecordRouteAccepted( void ) const
{ 
   UtlBoolean isRecordRoutable;

   // RFC3261 suggests record route may come even in response in "12.1.2 UAC Behavior"
   // "The route set MUST be set to the list of URIs in the Record-Route
   //  header field from the response"

   // We are dealing with a request, check if it can
   // accept a Record-Route header.  If the request
   // is not REGISTER, MESSAGE or PUBLISH, the request
   // is assumed to accept Record-Route headers.
   UtlString method;
   getRequestMethod(&method);

   if (method.compareTo(SIP_MESSAGE_METHOD) == 0 ||
      method.compareTo(SIP_REGISTER_METHOD) == 0 ||
      method.compareTo(SIP_PUBLISH_METHOD) == 0 )
   {
      isRecordRoutable = FALSE;        
   }
   else
   {
      isRecordRoutable = TRUE;
   }
   return isRecordRoutable;
}

UtlBoolean SipMessage::isUrlHeaderAllowed(const char* headerFieldName)
{
    UtlString name(headerFieldName);
    name.toUpper();

    return (!sSipMessageFieldProps.mDisallowedUrlHeaders.contains(&name));
}

UtlBoolean SipMessage::isUrlHeaderUnique(const char* headerFieldName)
{
    UtlString name(headerFieldName);
    name.toUpper();
    
    return (sSipMessageFieldProps.mUniqueUrlHeaders.contains(&name));
}

//SDUA
UtlBoolean SipMessage::getDNSField( UtlString * Protocol , UtlString * Address, UtlString * Port) const
{

   //protocol can be empty by default
   if( !m_dnsAddress.isNull() && !m_dnsPort.isNull())
   {
      Protocol->remove(0);
      Address->remove(0);
      Port->remove(0);

      Protocol->append(m_dnsProtocol);
      Address->append(m_dnsAddress);
      Port->append(m_dnsPort);
      return (true);
   }
   else
   {
      return (false);
   }
}

void SipMessage::setDNSField( const char* Protocol , const char* Address, const char* Port)
{
   m_dnsProtocol.remove(0);
   m_dnsAddress.remove(0);
   m_dnsPort.remove(0);

   m_dnsProtocol.append(Protocol);
   m_dnsAddress.append(Address);
   m_dnsPort.append(Port);
}

void SipMessage::clearDNSField()
{
   m_dnsProtocol.remove(0);
   m_dnsAddress.remove(0);
   m_dnsPort.remove(0);
}

void SipMessage::setTransaction(SipTransaction* transaction)
{
    mpSipTransaction = transaction;
}

SipTransaction* SipMessage::getSipTransaction() const
{
    return(mpSipTransaction);
}

const UtlString SipMessage::getTransportName() const
{
    UtlString transport;

    UtlString toField;
    getToField(&toField);
    Url toUrl(toField);
    UtlString topRoute ;
    getRouteUri(0, &topRoute) ;
    Url route(topRoute) ;
    
    route.getUrlParameter("transport", transport);
    if (transport.isNull())
    {
       toUrl.getUrlParameter("transport", transport);
    }

    return transport;
}


void SipMessage::ParseContactFields(const SipMessage *registerResponse,
                                    const SipMessage *SipRequest,
                                    const UtlString &subField,
                                    int& subFieldRetVal)
{
   //get the request contact value ...so that we can find out the expires subfield value
   // for this contact from the list of contacts returned byt the Rgister server
   UtlString RequestContactValue;
   SipRequest->getContactEntry(0 , &RequestContactValue);

   UtlString contactField;
   int indexContactField = 0;

   while (registerResponse->getContactEntry(indexContactField , &contactField))
   {
      if ( strstr(contactField, RequestContactValue ) != NULL)
      {
         UtlString subfieldText;
         int subfieldIndex = 0;
         UtlString subfieldName;
         UtlString subfieldValue;
         NameValueTokenizer::getSubField(contactField.data(), subfieldIndex, ";", &subfieldText);
         while(!subfieldText.isNull())
         {
            NameValueTokenizer::getSubField(subfieldText.data(), 0, "=", &subfieldName);
            NameValueTokenizer::getSubField(subfieldText.data(), 1, "=", &subfieldValue);
#               ifdef TEST_PRINT
            osPrintf("ipMessage::ParseContactFields found contact parameter[%d]: \"%s\" value: \"%s\"\n",
               subfieldIndex, subfieldName.data(), subfieldValue.data());
#               endif
            subfieldName.toUpper();
            if(subfieldName.compareTo(subField, UtlString::ignoreCase) == 0 &&
               subField.compareTo(SIP_EXPIRES_FIELD, UtlString::ignoreCase)== 0)
            {

               //see if more than one token in the expire value
               NameValueTokenizer::getSubField(subfieldValue, 1,
               " \t:;,", &subfieldText);

               // if not ...time is in seconds
               if(subfieldText.isNull())
               {
                  subFieldRetVal = atoi(subfieldValue);
               }
               // If there is more than one token assume it is a text date
               else
               {
                  // Get the expiration date
                  long dateExpires = OsDateTime::convertHttpDateToEpoch(subfieldValue);
                  long dateSent = 0;
                  // If the date was not set in the message
                  if(!registerResponse->getDateField(&dateSent))
                  {
                     #ifdef TEST_PRINT
                     osPrintf("Date field not set\n");
                     #endif
                     // Assume date sent is now
                     dateSent = OsDateTime::getSecsSinceEpoch();
                  }
                  #ifdef TEST_PRINT
                  osPrintf("Contact expires date: %ld\n", dateExpires); osPrintf("Current time: %ld\n", dateSent);
                  #endif
                  subFieldRetVal = dateExpires - dateSent;
               }
               break;
            }//any other field
            else if(subfieldName.compareTo(subField, UtlString::ignoreCase) == 0)
            {
               subFieldRetVal = atoi(subfieldValue);
            }

            subfieldIndex++;
            NameValueTokenizer::getSubField(contactField.data(), subfieldIndex, ";", &subfieldText);
         }
      }
      indexContactField ++;
   }
   return ;

}

const UtlString& SipMessage::getLocalIp() const
{
    return mLocalIp;
}

void SipMessage::setLocalIp(const UtlString& localIp)
{
    mLocalIp = localIp;
}

void SipMessage::setLocalIp(const SipMessage* pMsg) 
{
    assert(pMsg != NULL) ;
    if (pMsg)
    {
        setLocalIp(pMsg->getLocalIp()) ;
    }
}

void SipMessage::setPreferredTransport(OsSocket::IpProtocolSocketType transport)
{
   mPreferredTransport = transport;
}

OsSocket::IpProtocolSocketType SipMessage::getPreferredTransport() const
{
   return mPreferredTransport;
}

/// Get the name/value pairs for a Via field
///
///
void SipMessage::parseViaParameters( const char* viaField
                                    ,UtlContainer& viaParamList
                                    )

{
    const char* pairSeparator = ";";
    const char* namValueSeparator = "=";

    const char* nameAndValuePtr;
    int nameAndValueLength;
    const char* namePtr;
    int nameLength;
    int nameValueIndex = 0;
   UtlString value;
    int lastCharIndex = 0;
    int relativeIndex;
    int nameValueRelativeIndex;
    int viaFieldLength = strlen(viaField);

    do
    {
#       ifdef  TEST_PRINT
        osPrintf("SipMessage::parseViaParameters: \"%s\" lastCharIndex: %d",
                 &(viaField[lastCharIndex]), lastCharIndex);
#       endif
        // Pull out a name value pair
        NameValueTokenizer::getSubField(&(viaField[lastCharIndex]),
                                        viaFieldLength - lastCharIndex,
                                        0,
                                        pairSeparator,
                                        nameAndValuePtr,
                                        nameAndValueLength,
                                        &relativeIndex);
        lastCharIndex += relativeIndex;

        if(nameAndValuePtr && nameAndValueLength > 0)
        {
            // Separate the name and value
            NameValueTokenizer::getSubField(nameAndValuePtr,
                                            nameAndValueLength,
                                            0,
                                            namValueSeparator,
                                            namePtr,
                                            nameLength,
                                            &nameValueRelativeIndex);

            // Get rid of leading white space in the name
            while(nameLength > 0 &&
                  (*namePtr == ' ' ||
                   *namePtr == '\t'))
            {
                nameLength--;
                namePtr++;
            }

            if(nameLength > 0)
            {
                int valueSeparatorOffset = strspn(&(namePtr[nameLength]),
                                                  namValueSeparator);
                const char* valuePtr = &(namePtr[nameLength]) + valueSeparatorOffset;
                int valueLength = nameAndValueLength -
                    (valuePtr - nameAndValuePtr);

                // If there is a value
                if(valueSeparatorOffset <= 0 ||
                   *valuePtr == '\0' ||
                   valueLength <= 0)
                {
                    valuePtr = NULL;
                    valueLength = 0;
                }

                NameValuePair* newNvPair = new NameValuePair("");
                newNvPair->append(namePtr, nameLength);
                if(valuePtr)
                {
                    value.remove(0);
                    value.append(valuePtr, valueLength);
                    NameValueTokenizer::frontBackTrim(&value, " \t\n\r");
                    newNvPair->setValue(value);
                }
                else
                {
                    newNvPair->setValue("");
                }

                NameValueTokenizer::frontBackTrim(newNvPair, " \t\n\r");

                // Add a name, value pair to the list
                viaParamList.insert(newNvPair);

                nameValueIndex++;
            }
        }
    } while(   nameAndValuePtr
            && nameAndValueLength > 0
            && viaField[lastCharIndex] != '\0'
            );
}

void SipMessage::setSipIfMatchField(const char* sipIfMatchField)
{
    setHeaderValue(SIP_IF_MATCH_FIELD, sipIfMatchField, 0);
}

UtlBoolean SipMessage::getSipIfMatchField(UtlString& sipIfMatchField) const
{
    const char* fieldValue = getHeaderValue(0, SIP_IF_MATCH_FIELD);
    sipIfMatchField.remove(0);
    if(fieldValue) sipIfMatchField.append(fieldValue);
    return(fieldValue != NULL);
}

void SipMessage::setSipETagField(const char* sipETagField)
{
    setHeaderValue(SIP_ETAG_FIELD, sipETagField, 0);
}

UtlBoolean SipMessage::getSipETagField(UtlString& sipETagField) const
{
    const char* fieldValue = getHeaderValue(0, SIP_ETAG_FIELD);
    sipETagField.remove(0);
    if(fieldValue) sipETagField.append(fieldValue);
    return(fieldValue != NULL);
}


/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

SipMessage::SipMessageFieldProps::SipMessageFieldProps()
{
   // Call the initializer functions.
   initNames();
   initDisallowedUrlHeaders();
   initUniqueUrlHeaders();
}

SipMessage::SipMessageFieldProps::~SipMessageFieldProps()
{
   mLongFieldNames.destroyAll();
   mShortFieldNames.destroyAll();
   mDisallowedUrlHeaders.destroyAll();
   mUniqueUrlHeaders.destroyAll();
}

void SipMessage::SipMessageFieldProps::initNames()
{
   // Load the table to translate long header names to short names.

   mLongFieldNames.insert(new NameValuePair(SIP_CONTENT_TYPE_FIELD, SIP_SHORT_CONTENT_TYPE_FIELD));
   mLongFieldNames.insert(new NameValuePair(SIP_CONTENT_ENCODING_FIELD, SIP_SHORT_CONTENT_ENCODING_FIELD));
   mLongFieldNames.insert(new NameValuePair(SIP_FROM_FIELD, SIP_SHORT_FROM_FIELD));
   mLongFieldNames.insert(new NameValuePair(SIP_CALLID_FIELD, SIP_SHORT_CALLID_FIELD));
   mLongFieldNames.insert(new NameValuePair(SIP_CONTACT_FIELD, SIP_SHORT_CONTACT_FIELD));
   mLongFieldNames.insert(new NameValuePair(SIP_CONTENT_LENGTH_FIELD, SIP_SHORT_CONTENT_LENGTH_FIELD));
   mLongFieldNames.insert(new NameValuePair(SIP_REFERRED_BY_FIELD, SIP_SHORT_REFERRED_BY_FIELD));
   mLongFieldNames.insert(new NameValuePair(SIP_REFER_TO_FIELD, SIP_SHORT_REFER_TO_FIELD));
   mLongFieldNames.insert(new NameValuePair(SIP_SUBJECT_FIELD, SIP_SHORT_SUBJECT_FIELD));
   mLongFieldNames.insert(new NameValuePair(SIP_SUPPORTED_FIELD, SIP_SHORT_SUPPORTED_FIELD));
   mLongFieldNames.insert(new NameValuePair(SIP_TO_FIELD, SIP_SHORT_TO_FIELD));
   mLongFieldNames.insert(new NameValuePair(SIP_VIA_FIELD, SIP_SHORT_VIA_FIELD));
   mLongFieldNames.insert(new NameValuePair(SIP_EVENT_FIELD, SIP_SHORT_EVENT_FIELD));
   mLongFieldNames.insert(new NameValuePair(SIP_SESSION_EXPIRES_FIELD, SIP_SHORT_SESSION_EXPIRES_FIELD));

   // Reverse the pairs to load the table to translate short header names to
   // long ones.

   UtlHashBagIterator iterator(mLongFieldNames);
   NameValuePair* nvPair;
   while ((nvPair = (NameValuePair*) iterator()))
   {
      mShortFieldNames.insert(new NameValuePair(nvPair->getValue(),
                                                 nvPair->data()));
   }
}

void SipMessage::SipMessageFieldProps::initDisallowedUrlHeaders()
{
   // These headers may NOT be passed through in a URL to
   // be set in a message

   mDisallowedUrlHeaders.insert(new UtlString(SIP_CONTACT_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_SHORT_CONTACT_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_CONTENT_LENGTH_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_SHORT_CONTENT_LENGTH_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_CONTENT_TYPE_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_SHORT_CONTENT_TYPE_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_CONTENT_ENCODING_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_SHORT_CONTENT_ENCODING_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_CSEQ_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_RECORD_ROUTE_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_REFER_TO_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_REFERRED_BY_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_TO_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_SHORT_TO_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_USER_AGENT_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_VIA_FIELD));
   mDisallowedUrlHeaders.insert(new UtlString(SIP_SHORT_VIA_FIELD));
}

void SipMessage::SipMessageFieldProps::initUniqueUrlHeaders()
{
   // These headers may occur only once in a message, so a URI header
   // parameter overrides the existing header in the message.

   mUniqueUrlHeaders.insert(new UtlString(SIP_EXPIRES_FIELD));
   mUniqueUrlHeaders.insert(new UtlString(SIP_ROUTE_FIELD));
}

bool SipMessage::smimeEncryptSdp(const void *pEventData)
{
    if (pEventData)
    {
        mpEventData = (void*)pEventData;
    }
    const HttpBody* pOriginalBody = NULL;
    bool bRet = false;
    if (mpSecurity && getSdpBody(mpSecurity) != NULL)
    {
        UtlString bodyBytes;
        int bodyLength;
        
        pOriginalBody = getBody() ;
        
        if (pOriginalBody)
        {
            pOriginalBody->getBytes(&bodyBytes, &bodyLength);
            SdpBody* pSdpBodyCopy = new SdpBody(bodyBytes, bodyLength); // this should be destroyed by
                                                                // the smime destructor,
                                                                // so, we wont destroy it here
              
            SIPXTACK_SECURITY_ATTRIBUTES* pSecurityAttrib = (SIPXTACK_SECURITY_ATTRIBUTES*)mpSecurity;
            if (pSdpBodyCopy && pSecurityAttrib->getSecurityLevel() > 0)
            {
                SmimeBody* newlyEncryptedBody = new SmimeBody(NULL, 0, NULL);
                UtlString der(pSecurityAttrib->getSmimeKey(), pSecurityAttrib->getSmimeKeyLength());
                const char* derPublicKeyCert[1];
                int certLength[1];
                
                derPublicKeyCert[0] = der.data();   
                certLength[0] = der.length();
                if (newlyEncryptedBody->encrypt(pSdpBodyCopy,
                                                1,
                                                derPublicKeyCert,
                                                certLength,
                                                mpSecurity->szMyCertNickname,
                                                mpSecurity->szCertDbPassword,
                                                this) )
                {
                    setBody(newlyEncryptedBody);
                    setContentType(CONTENT_SMIME_PKCS7);
                    setContentLength(newlyEncryptedBody->getLength());
                    bRet = true;
                }
            }
        }
    }
    else
    {
        OnError(SIP_SECURITY_ENCRYPT, SIP_SECURITY_CAUSE_ENCRYPT_FAILURE_INVALID_PARAMETER);
    }

    return bRet;
}


bool SipMessage::fireSecurityEvent(const void* pEventData,
                        const enum SIP_SECURITY_EVENT event,
                        const enum SIP_SECURITY_CAUSE cause,
                        SIPXTACK_SECURITY_ATTRIBUTES* const pSecurity,
                        void* pCert,
                        char* szSubjectAltName) const
{
    /*SIPX_SECURITY_INFO info;
    memset(&info, 0, sizeof(SIPX_SECURITY_INFO));
    info.nSize = sizeof(SIPX_SECURITY_INFO);
    info.event = event;
    info.cause = cause;
    UtlString callId;
    getCallIdField(&callId);

    if (pSecurity)
    {
        info.nCertificateSize = pSecurity->getSmimeKeyLength();

        info.callId = (char*)callId.data();
        // determine if the remoteAddress is the ToField or the FromField
        Url urlIdentity;
        UtlString identity;
        if (mbFromThisSide && !isResponse())
        {
            if  (!isResponse())
            {
                getToUrl(urlIdentity);
            }
            else
            {
                getFromUrl(urlIdentity);
            }
        }
        else
        {
            if  (!isResponse())
            {
                getFromUrl(urlIdentity);
            }
            else
            {
                getToUrl(urlIdentity);
            }
        }
        identity = urlIdentity.toString();
        if (pCert)
        {

            info.pCertificate = pCert;
            info.szSubjAltName = szSubjectAltName;
        }
        else
        {
            info.pCertificate = (void*)pSecurity->getSmimeKey();
        }
        info.remoteAddress = (char*)identity.data();
        info.szSRTPkey = (char*)pSecurity->getSrtpKey();

    }
    return TapiMgr::getInstance().fireEvent(pEventData ? pEventData : mpEventData,
                                            EVENT_CATEGORY_SECURITY,
                                            &info);*/
    // fix event callback!!, no way we use TapiMgr here

    return true;
}

void SipMessage::OnError(SIP_SECURITY_EVENT event, SIP_SECURITY_CAUSE cause)
{
    fireSecurityEvent(mpEventData, event, cause, mpSecurity);
}

bool SipMessage::OnSignature(void* pCert, char* szSubjAltName)
{
    return fireSecurityEvent(mpEventData,
                             SIP_SECURITY_DECRYPT, 
                             SIP_SECURITY_CAUSE_SIGNATURE_NOTIFY,
                             mpSecurity,
                             pCert,
                             szSubjAltName);
}

void SipMessage::normalizeProxyRoutes(const SipUserAgent* sipUA,
                                      Url& requestUri,
                                      UtlSList* removedRoutes
                                      )
{
   UtlString requestUriString;
   Url topRouteUrl;
   UtlString topRouteValue;

   /*
    * Check the request URI and the topmost route
    *   - Detect and correct for any strict router upstream
    *     as specified by RFC 3261 section 16.4 Route Information Preprocessing:
    *
    *       The proxy MUST inspect the Request-URI of the request.  If the
    *       Request-URI of the request contains a value this proxy previously
    *       placed into a Record-Route header field (see Section 16.6 item 4),
    *       the proxy MUST replace the Request-URI in the request with the last
    *       value from the Route header field, and remove that value from the
    *       Route header field.  The proxy MUST then proceed as if it received
    *       this modified request.
    *
    *   - Pop off the topmost route until it is not me
    *
    * Note that this loop always executes at least once, and that:
    *   - it leaves requestUri set correctly
    */
   bool doneNormalizingRouteSet = false;
   while (! doneNormalizingRouteSet)
   {
      // Check the request URI.
      //    If it has 'lr' parameter that is me, then the sender was a 
      //    strict router (it didn't recognize the loose route indication)
      getRequestUri(&requestUriString);
      requestUri.fromString(requestUriString, TRUE /* is a request uri */);

      UtlString noValue;
      if (   requestUri.getUrlParameter("lr", noValue, 0)
          && sipUA->isMyHostAlias(requestUri)
          )
      {
         /*
          * We need to fix it (convert it back to a loose route)..
          * - pop the last route and put it in the request URI
          *   see RFC 3261 section 16.4
          *
          * For example:
          *   INVITE sip:mydomain.com;lr SIP/2.0
          *   Route: <sip:proxy.example.com;lr>, <sip:user@elsewhere.example.com>
          * becomes:
          *   INVITE sip:user@elsewhere.example.com SIP/2.0
          *   Route: <sip:proxy.example.com;lr>
          */
         UtlString lastRouteValue;
         int lastRouteIndex;
         if ( getLastRouteUri(lastRouteValue, lastRouteIndex) )
         {
            removeRouteUri(lastRouteIndex, &lastRouteValue);

            // Put the last route in as the request URI
            changeUri(lastRouteValue); // this strips appropriately
                     
            OsSysLog::add(FAC_SIP, PRI_DEBUG,
                          "SipMessage::normalizeProxyRoutes "
                          "strict route '%s' replaced with uri from '%s'",
                          requestUriString.data(), lastRouteValue.data());
            if (removedRoutes)
            {
               // save a copy of the route we're removing for the caller.
               // this looks just like this was properly loose routed.
               // use the output from Url::toString so that all are normalized.
               UtlString* removedRoute = new UtlString;
               requestUri.toString(*removedRoute);
               removedRoutes->append(removedRoute);
               // caller is responsible for deleting the savedRoute
            }
         }
         else
         {
            OsSysLog::add(FAC_SIP, PRI_WARNING, "SipMessage::normalizeProxyRoutes  "
                          "found 'lr' in Request-URI with no Route; stripped 'lr'"
                          );

            requestUri.removeUrlParameter("lr");
            UtlString newUri;
            requestUri.toString(newUri);
            changeRequestUri(newUri); // this strips appropriately
         }

         // note: we've changed the request uri and the route set,
         //       but not set doneNormalizingRouteSet, so we go around again...
      }
      else // topmost route was not a loose route uri we put there... 
      {
         if ( getRouteUri(0, &topRouteValue) )
         {
            /*
             * There is a Route header... if it is a route to this proxy, pop it off.
             * For example:
             *   INVITE sip:user@elsewhere.example.com SIP/2.0
             *   Route: <sip:mydomain.com;lr>, <sip:proxy.example.com;lr>
             * becomes:
             *   INVITE sip:user@elsewhere.example.com SIP/2.0
             *   Route: <sip:proxy.example.com;lr>
             */
            topRouteUrl.fromString(topRouteValue,FALSE /* not a request uri */);
            if ( sipUA->isMyHostAlias(topRouteUrl) )
            {
               if (removedRoutes)
               {
                  // save a copy of the route we're removing for the caller.
                  // use the output from Url::toString so that all are normalized.
                  UtlString* savedRoute = new UtlString();
                  topRouteUrl.toString(*savedRoute);
                  removedRoutes->append(savedRoute);
                  // caller is responsible for deleting the savedRoute
               }
               UtlString removedRoute;
               removeRouteUri(0, &removedRoute);
                           
               OsSysLog::add(FAC_SIP, PRI_DEBUG,
                             "SipMessage::normalizeProxyRoutes popped route to self '%s'",
                             removedRoute.data()
                             );
            }
            else // topmost route is someone else
            {
               doneNormalizingRouteSet = true;
            }
         }
         else // no more routes
         {
            doneNormalizingRouteSet = true;
         }
      }
   } // while ! doneNormalizingRouteSet
}
