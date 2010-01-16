//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
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

// SYSTEM INCLUDES
#include <assert.h>

// APPLICATION INCLUDES
#include <net/SipTransport.h>
#include <net/Url.h>

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

SIP_TRANSPORT_TYPE SipTransport::getSipTransport(const Url& url)
{
   return getSipTransport(url.toString());
}

SIP_TRANSPORT_TYPE SipTransport::getSipTransport(const UtlString& strUrl)
{
   SIP_TRANSPORT_TYPE transport = SIP_TRANSPORT_UDP;

   UtlString lowerStrUrl(strUrl);
   lowerStrUrl.toLower();

   if (lowerStrUrl.contains("sips:") || lowerStrUrl.contains("transport=tls"))
   {
      transport = SIP_TRANSPORT_TLS;
   }
   if (lowerStrUrl.contains("transport=tcp"))
   {
      transport = SIP_TRANSPORT_TCP;
   }

   return transport;
}

SIP_TRANSPORT_TYPE SipTransport::getSipTransport(const char* szUrl)
{
   UtlString strUrl(szUrl);
   return getSipTransport(strUrl);
}

SIP_TRANSPORT_TYPE SipTransport::getSipTransport(OsSocket::IpProtocolSocketType protocolType)
{
   switch (protocolType)
   {
      case OsSocket::UDP:
         return SIP_TRANSPORT_UDP;
      case OsSocket::TCP:
         return SIP_TRANSPORT_TCP;
      case OsSocket::SSL_SOCKET:
         return SIP_TRANSPORT_TLS;
   }

   return SIP_TRANSPORT_UDP; // default is UDP
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */


