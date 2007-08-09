//
// Copyright (C) 2007 Jaroslav Libak
// Licensed to SIPfoundry under a Contributor Agreement.
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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif /* _WIN32 */

// APPLICATION INCLUDES
#include "tapi/SipXUtil.h"
#include "tapi/sipXtapi.h"
#include "net/Url.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

// CHECKED
/**
* Simple utility function to parse the username, host, and port from
* a URL.  All url, field, and header parameters are ignored.  You may also 
* specify NULL for any parameters (except szUrl) which are not needed.  
* Lastly, the caller must allocate enough storage space for each url
* component -- if in doubt use the length of the supplied szUrl.
*/
SIPXTAPI_API SIPX_RESULT sipxUtilUrlParse(const char* szUrl,
                                          char* szUsername,
                                          char* szHostname,
                                          int* iPort) 
{
   SIPX_RESULT rc = SIPX_RESULT_FAILURE;    

   if (szUrl && strlen(szUrl))
   {
      Url url(szUrl);
      UtlString temp;

      if (szUsername)
      {
         url.getUserId(temp);
         strcpy(szUsername, temp);
      }
      if (szHostname)
      {
         url.getHostAddress(temp);
         strcpy(szHostname, temp);
      }
      if (iPort)
      {
         *iPort = url.getHostPort();
      }

      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxUtilUrlGetDisplayName(const char* szUrl,
                                                   char* szDisplayName,
                                                   size_t nDisplayName) 
{
   SIPX_RESULT rc = SIPX_RESULT_FAILURE;

   if (szUrl && strlen(szUrl))
   {
      Url url(szUrl);
      UtlString temp;

      if (szDisplayName && nDisplayName)
      {
         url.getDisplayName(temp);
         temp.strip(UtlString::both, '\"');
         strncpy(szDisplayName, temp, nDisplayName);
      }

      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
/**
* Simple utility function to update a URL.  If the szUrl isn't large enough,
* this function will fail.  Specify a NULL szUrl to request required length.
* To leave an existing component unchanged, use NULL for strings and -1 for 
* ports.
*/
SIPXTAPI_API SIPX_RESULT sipxUtilUrlUpdate(char* szUrl,
                                           size_t* nUrl,
                                           const char* szNewUsername,
                                           const char* szNewHostname,
                                           const int iNewPort) 
{
   SIPX_RESULT rc = SIPX_RESULT_FAILURE;
   UtlString results;

   if (szUrl)
   { 
      Url url(szUrl);

      if (szNewUsername)
      {
         url.setUserId(szNewUsername);
      }
      if (szNewHostname)
      {
         url.setHostAddress(szNewHostname);
      }
      if (iNewPort != -1)
      {
         url.setHostPort(iNewPort);
      }

      url.toString(results);

      if (szUrl && results.length() < *nUrl) 
      {
         strcpy(szUrl, results);
         rc = SIPX_RESULT_SUCCESS;
      }
   }
   *nUrl = results.length() + 1;

   return rc;
}

// CHECKED
static bool findUrlParameter(Url* pUrl,
                             const char* szName,
                             size_t index,
                             char* szValue,
                             size_t nValue)
{
   bool bRC = false;
   int paramCount = 0;
   UtlString name;
   UtlString value;

   while (bRC == false && pUrl->getUrlParameter(paramCount, name, value))
   {
      if (name.compareTo(szName, UtlString::ignoreCase) == 0)
      {
         if (index == 0)
         {
            bRC = true;
            strncpy(szValue, value.data(), nValue);
         }
         else
         {
            index--;
         }
      }
      paramCount++;
   }

   return bRC;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxUtilUrlGetUrlParam(const char* szUrl,
                                                const char* szParamName,
                                                size_t nParamIndex,
                                                char* szParamValue,
                                                size_t nParamValue)
{
   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   UtlString results;

   if (szUrl && szParamName && szParamValue)
   {
      UtlString value;

      //  Assume addr-spec, but allow name-addr
      Url addrSpec(szUrl, true);
      Url nameAddr(szUrl, false);

      if (findUrlParameter(&addrSpec, szParamName, nParamIndex,
            szParamValue, nParamValue) ||
          findUrlParameter(&nameAddr, szParamName, nParamIndex, 
            szParamValue, nParamValue))
      {
         rc = SIPX_RESULT_SUCCESS;
      }
      else
      {
         rc = SIPX_RESULT_FAILURE;
      }
   }

   return rc;
}

