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
#include <os/OsLock.h>
#include <os/OsMsgQ.h>
#include <mi/CpMediaInterfaceFactory.h>
#include <mi/CpMediaInterface.h>
#include <cp/XCpAbstractCall.h>
#include <cp/CpMessageTypes.h>
#include <cp/AcCommandMsg.h>
#include <cp/AcNotificationMsg.h>
#include <cp/CmGainFocusMsg.h>
#include <cp/CmYieldFocusMsg.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
const int XCpAbstractCall::CALL_MAX_REQUEST_MSGS = 200;
const UtlContainableType XCpAbstractCall::TYPE = "XCpAbstractCall";

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

XCpAbstractCall::XCpAbstractCall(const UtlString& sId,
                                 SipUserAgent& rSipUserAgent,
                                 CpMediaInterfaceFactory& rMediaInterfaceFactory,
                                 OsMsgQ& rCallManagerQueue)
: OsServerTask("XCpAbstractCall-%d", NULL, CALL_MAX_REQUEST_MSGS)
, m_memberMutex(OsMutex::Q_FIFO)
, m_sId(sId)
, m_rSipUserAgent(rSipUserAgent)
, m_rMediaInterfaceFactory(rMediaInterfaceFactory)
, m_rCallManagerQueue(rCallManagerQueue)
, m_pMediaInterface(NULL)
, m_bIsFocused(FALSE)
{

}

XCpAbstractCall::~XCpAbstractCall()
{
   waitUntilShutDown();
}

/* ============================ MANIPULATORS ============================== */

UtlBoolean XCpAbstractCall::handleMessage(OsMsg& rRawMsg)
{
   UtlBoolean bResult = FALSE;

   switch (rRawMsg.getMsgType())
   {
   case CpMessageTypes::AC_COMMAND:
      {
         AcCommandMsg* pAcCommandMsg = dynamic_cast<AcCommandMsg*>(&rRawMsg);
         if (pAcCommandMsg)
         {
            return handleCommandMessage(*pAcCommandMsg);
         }
         break;
      }
   case CpMessageTypes::AC_NOTIFICATION:
      {
         AcNotificationMsg* pAcNotificationMsg = dynamic_cast<AcNotificationMsg*>(&rRawMsg);
         if (pAcNotificationMsg)
         {
            return handleNotificationMessage(*pAcNotificationMsg);
         }
         break;
      }
   default:
      break;
   }

   return bResult;
}

OsStatus XCpAbstractCall::audioToneStart(int iToneId,
                                         UtlBoolean bLocal,
                                         UtlBoolean bRemote)
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioToneStop()
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioFilePlay(const UtlString& audioFile,
                                        UtlBoolean bRepeat,
                                        UtlBoolean bLocal,
                                        UtlBoolean bRemote,
                                        UtlBoolean bMixWithMic /*= FALSE*/,
                                        int iDownScaling /*= 100*/,
                                        void* pCookie /*= NULL*/)
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioBufferPlay(const void* pAudiobuf,
                                          size_t iBufSize,
                                          int iType,
                                          UtlBoolean bRepeat,
                                          UtlBoolean bLocal,
                                          UtlBoolean bRemote,
                                          void* pCookie /*= NULL*/)
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioStop()
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::pauseAudioPlayback()
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::resumeAudioPlayback()
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioRecordStart(const UtlString& sFile)
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::audioRecordStop()
{
   return OS_FAILED;
}

OsStatus XCpAbstractCall::acquire(const OsTime& rTimeout /*= OsTime::OS_INFINITY*/)
{
   return m_memberMutex.acquire(rTimeout);
}

OsStatus XCpAbstractCall::tryAcquire()
{
   return m_memberMutex.tryAcquire();
}

OsStatus XCpAbstractCall::release()
{
   return m_memberMutex.release();
}

/* ============================ ACCESSORS ================================= */

unsigned XCpAbstractCall::hash() const
{
   return (unsigned)this;
}

UtlContainableType XCpAbstractCall::getContainableType() const
{
   return XCpAbstractCall::TYPE;
}

UtlString XCpAbstractCall::getId() const
{
   return m_sId;
}

/* ============================ INQUIRY =================================== */

int XCpAbstractCall::compareTo(UtlContainable const* inVal) const
{
   int result;

   if (inVal->isInstanceOf(XCpAbstractCall::TYPE))
   {
      result = hash() - inVal->hash();
   }
   else
   {
      result = -1; 
   }

   return result;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

UtlBoolean XCpAbstractCall::handleCommandMessage(AcCommandMsg& rRawMsg)
{
   switch ((AcCommandMsg::SubTypesEnum)rRawMsg.getMsgSubType())
   {
   case AcCommandMsg::AC_GAIN_FOCUS:
      handleGainFocus();
      return TRUE;
   case AcCommandMsg::AC_YIELD_FOCUS:
      handleDefocus();
      return TRUE;
   default:
      break;
   }

   return FALSE;
}

UtlBoolean XCpAbstractCall::handleNotificationMessage(AcNotificationMsg& rRawMsg)
{
   return TRUE;
}

OsStatus XCpAbstractCall::gainFocus()
{
#ifndef DISABLE_LOCAL_AUDIO
   CmGainFocusMsg gainFocusMsg(m_sId);
   return m_rCallManagerQueue.send(gainFocusMsg);
#else
   return OS_SUCCESS;
#endif
}

OsStatus XCpAbstractCall::yieldFocus()
{
#ifndef DISABLE_LOCAL_AUDIO
   CmYieldFocusMsg yieldFocusMsg(m_sId);
   return m_rCallManagerQueue.send(yieldFocusMsg);
#else
   return OS_SUCCESS;
#endif
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

OsStatus XCpAbstractCall::handleGainFocus()
{
#ifndef DISABLE_LOCAL_AUDIO
   OsLock lock(m_memberMutex);

   if (m_pMediaInterface && !m_bIsFocused)
   {
      OsStatus resFocus = m_pMediaInterface->giveFocus();
      if (resFocus == OS_SUCCESS)
      {
         m_bIsFocused = TRUE;
         return OS_SUCCESS;
      }
   }

   return OS_FAILED;
#else
   return OS_SUCCESS;
#endif
}

OsStatus XCpAbstractCall::handleDefocus()
{
#ifndef DISABLE_LOCAL_AUDIO
   OsLock lock(m_memberMutex);

   if (m_pMediaInterface && m_bIsFocused)
   {
      OsStatus resFocus = m_pMediaInterface->defocus();
      if (resFocus == OS_SUCCESS)
      {
         m_bIsFocused = FALSE;
         return OS_SUCCESS;
      }
   }

   return OS_FAILED;
#else
   return OS_SUCCESS;
#endif
}

/* ============================ FUNCTIONS ================================= */

