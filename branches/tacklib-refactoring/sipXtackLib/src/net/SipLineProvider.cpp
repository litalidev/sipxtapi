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

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <net/SipLine.h>
#include <net/SipLineProvider.h>
#include <net/SipLineCredential.h>
#include <net/SipMessage.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

UtlBoolean SipLineProvider::getCredentialForMessage(const SipMessage& sipResponse, // message with authentication request
                                                    const SipMessage& sipRequest, // about to be sent by us
                                                    SipLineCredential& lineCredential) const
{
   UtlBoolean result = FALSE;
   UtlString lineId;
   Url lineUri;
   UtlString userId;

   // get LINEID, lineUri, userId from SipMessage
   extractLineData(sipRequest, lineId, lineUri, userId);

   // get copy of SipLine
   SipLine sipLine;
   UtlBoolean lineFound = findLineCopy(lineId, lineUri, userId, sipLine);
   if (lineFound)
   {
      // get authentication details from response
      HttpMessage::HttpEndpointEnum authorizationEntity = 
         HttpMessage::getAuthorizationEntity(sipResponse.getResponseStatusCode());
      UtlString nonce;
      UtlString opaque;
      UtlString realm;
      UtlString scheme;
      UtlString algorithm;
      UtlString qop;
      UtlString callId;
      UtlString stale;

      sipResponse.getAuthenticationData(
         &scheme, &realm, &nonce, &opaque,
         &algorithm, &qop, authorizationEntity, &stale);

      SipLineCredential sipCredential;
      UtlBoolean foundCredential = sipLine.getCredential(scheme, realm, sipCredential);
      if (foundCredential)
      {
         // check that credentials are for required userid
         if (!sipCredential.getUserId().compareTo(userId, UtlString::matchCase))
         {
            // userid match, return this credential
            lineCredential = sipCredential;
            result = TRUE;
         }
      }
   }

   return result;
}

UtlBoolean SipLineProvider::getProxyServersForMessage(const SipMessage& sipRequest,
                                                      UtlString& proxyServer) const
{
   UtlString lineId;
   Url lineUri;
   UtlString userId;

   // get LINEID, lineUri, userId from SipMessage
   extractLineData(sipRequest, lineId, lineUri, userId);

   return getLineProxyServers(lineUri, proxyServer);
}

void SipLineProvider::extractLineData(const SipMessage& sipMsg,
                                      UtlString& lineId,
                                      Url& lineUri,
                                      UtlString& userId)
{
   // get LINEID, lineUri, userId
   if (!sipMsg.isFromThisSide())
   {
      // use requestUri & toUrl
      UtlString requestMethod;
      sipMsg.getRequestMethod(&requestMethod);
      if (sipMsg.isResponse() || requestMethod.compareTo(SIP_REGISTER_METHOD))
      {
         // this is REGISTER, RequestUri is different from toUri, use toUri
         Url toUrl;
         sipMsg.getToUrl(toUrl);
         lineUri = SipLine::getLineUri(toUrl); // get lineUri from toUrl
      }
      else
      {
         // INVITE, INFO... These may also have different RequestUri from toUri, if there is redirection 3xx.
         // See RFC-2833 - 8.2.2.1 To and Request-URI
         UtlString requestUri;
         sipMsg.getRequestUri(&requestUri);
         lineUri = SipLine::getLineUri(requestUri); // RequestUri is definitely us, To header field not necessarily..
      }
      lineUri.getUrlParameter(SIP_LINE_IDENTIFIER , lineId); // get LINEID from lineUri
      lineUri.getUserId(userId); // get userId from lineUri
   }
   else
   {
      // outbound message
      Url fromUrl;
      UtlString contact;
      sipMsg.getContactEntry(0, &contact);
      Url contactUrl(contact);
      contactUrl.getUrlParameter(SIP_LINE_IDENTIFIER, lineId);
      contactUrl.getUserId(userId);
      sipMsg.getFromUrl(fromUrl);
      lineUri = SipLine::getLineUri(fromUrl); // get lineUri from fromUrl
   }
}

/* ============================ INQUIRY =================================== */

UtlBoolean SipLineProvider::lineExists(const SipMessage& sipMsg) const
{
   UtlString lineId;
   Url lineUri;
   UtlString userId;

   // get LINEID, lineUri, userId
   extractLineData(sipMsg, lineId, lineUri, userId);

   return lineExists(lineId, lineUri, userId);
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */


