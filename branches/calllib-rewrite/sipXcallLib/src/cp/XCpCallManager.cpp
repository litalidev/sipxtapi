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
#include <os/OsDefs.h>
#include <os/OsLock.h>
#include <os/OsPtrLock.h>
#include <utl/UtlHashMapIterator.h>
#include <utl/UtlSList.h>
#include <utl/UtlInt.h>
#include <net/SipUserAgent.h>
#include <cp/XCpCallManager.h>
#include <cp/XCpAbstractCall.h>
#include <cp/XCpCall.h>
#include <cp/XCpConference.h>
#include <cp/XCpCallIdUtil.h>
#include <cp/CpMessageTypes.h>
#include <cp/msg/AcCommandMsg.h>
#include <net/SipDialog.h>
#include <net/SipMessageEvent.h>
#include <net/SipLineProvider.h>

// DEFINES
//#define PRINT_SIP_MESSAGE
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
const char* sipCallIdPrefix = "s"; // prefix of sip call-id sent in sip messages.

// STATIC VARIABLE INITIALIZATIONS
const int XCpCallManager::CALLMANAGER_MAX_REQUEST_MSGS = 2000;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

XCpCallManager::XCpCallManager(CpCallStateEventListener* pCallEventListener,
                               SipInfoStatusEventListener* pInfoStatusEventListener,
                               SipSecurityEventListener* pSecurityEventListener,
                               CpMediaEventListener* pMediaEventListener,
                               SipUserAgent& rSipUserAgent,
                               const SdpCodecList& rSdpCodecList,
                               SipLineProvider* pSipLineProvider,
                               UtlBoolean bDoNotDisturb,
                               UtlBoolean bEnableICE,
                               UtlBoolean bEnableSipInfo,
                               UtlBoolean bIsRequiredLineMatch,
                               int rtpPortStart,
                               int rtpPortEnd,
                               int maxCalls,
                               int inviteExpireSeconds,
                               CpMediaInterfaceFactory& rMediaInterfaceFactory)
: OsServerTask("XCallManager-%d", NULL, CALLMANAGER_MAX_REQUEST_MSGS)
, m_pCallEventListener(pCallEventListener)
, m_pInfoStatusEventListener(pInfoStatusEventListener)
, m_pSecurityEventListener(pSecurityEventListener)
, m_pMediaEventListener(pMediaEventListener)
, m_rSipUserAgent(rSipUserAgent)
, m_rDefaultSdpCodecList(rSdpCodecList)
, m_pSipLineProvider(pSipLineProvider)
, m_sipCallIdGenerator(sipCallIdPrefix)
, m_bDoNotDisturb(bDoNotDisturb)
, m_bEnableICE(bEnableICE)
, m_bEnableSipInfo(bEnableSipInfo)
, m_bIsRequiredLineMatch(bIsRequiredLineMatch)
, m_rtpPortStart(rtpPortStart)
, m_rtpPortEnd(rtpPortEnd)
, m_memberMutex(OsMutex::Q_FIFO)
, m_maxCalls(maxCalls)
, m_rMediaInterfaceFactory(rMediaInterfaceFactory)
, m_inviteExpireSeconds(inviteExpireSeconds)
{
   startSipMessageObserving();

   // Allow the "replaces" extension, because CallManager
   // implements the INVITE-with-Replaces logic.
   m_rSipUserAgent.allowExtension(SIP_REPLACES_EXTENSION);

   int defaultInviteExpireSeconds = m_rSipUserAgent.getDefaultExpiresSeconds();
   if (m_inviteExpireSeconds > defaultInviteExpireSeconds) m_inviteExpireSeconds = defaultInviteExpireSeconds;
}

XCpCallManager::~XCpCallManager()
{
   stopSipMessageObserving();
   waitUntilShutDown();
}

/* ============================ MANIPULATORS ============================== */

UtlBoolean XCpCallManager::handleMessage(OsMsg& rRawMsg)
{
   UtlBoolean bResult = FALSE;
   int msgType = rRawMsg.getMsgType();
   int msgSubType = rRawMsg.getMsgSubType();

   switch (msgType)
   {
   case OsMsg::PHONE_APP:
      return handlePhoneAppMessage(rRawMsg);
   case OsMsg::OS_EVENT: // timer event
   default:
      {
         OsSysLog::add(FAC_CP, PRI_ERR, "Unknown TYPE %d of XCpCallManager message subtype: %d\n", msgType, msgSubType);
         bResult = TRUE;
         break;
      }
   }

   return bResult;
}

void XCpCallManager::requestShutdown(void)
{
   m_callStack.shutdownAllAbstractCallThreads();

   OsServerTask::requestShutdown();
}

OsStatus XCpCallManager::createCall(UtlString& sCallId)
{
   OsStatus result = OS_FAILED;

   sCallId.remove(0); // clear string

   // always allow creation of new call, check for limit only when establishing
   if (sCallId.isNull())
   {
      sCallId = getNewCallId();
   }

   XCpCall *pCall = new XCpCall(sCallId, m_rSipUserAgent, m_rMediaInterfaceFactory, m_rDefaultSdpCodecList, *getMessageQueue(),
      &m_callStack, m_pCallEventListener, m_pInfoStatusEventListener, m_pSecurityEventListener, m_pMediaEventListener);

   UtlBoolean resStart = pCall->start();
   if (resStart)
   {
      UtlBoolean resPush = m_callStack.push(*pCall);
      if (resPush)
      {
         result = OS_SUCCESS;
      }
      else
      {
         delete pCall; // also shuts down thread
         pCall = NULL;
      }
   }

   return result;
}

OsStatus XCpCallManager::createConference(UtlString& sConferenceId)
{
   OsStatus result = OS_FAILED;

   sConferenceId.remove(0); // clear string

   // always allow creation of new conference, check for limit only when establishing
   if (sConferenceId.isNull())
   {
      sConferenceId = getNewConferenceId();
   }
   XCpConference *pConference = new XCpConference(sConferenceId, m_rSipUserAgent, m_rMediaInterfaceFactory, m_rDefaultSdpCodecList,
      *getMessageQueue(), &m_callStack, m_pCallEventListener, m_pInfoStatusEventListener, m_pSecurityEventListener,
      m_pMediaEventListener);

   UtlBoolean resStart = pConference->start();
   if (resStart)
   {
      UtlBoolean resPush = m_callStack.push(*pConference);
      if (resPush)
      {
         result = OS_SUCCESS;
      }
      else
      {
         delete pConference; // also shuts down thread
         pConference = NULL;
      }
   }

   return result;
}

OsStatus XCpCallManager::connectCall(const UtlString& sCallId,
                                     SipDialog& sSipDialog,
                                     const UtlString& toAddress,
                                     const UtlString& fullLineUrl,
                                     const UtlString& sSipCallId,
                                     const UtlString& locationHeader,
                                     CP_CONTACT_ID contactId)
{
   OsStatus result = OS_NOT_FOUND;
   sSipDialog = SipDialog();

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findCall(sCallId, ptrLock);
   if (resFind)
   {
      UtlString sTmpSipCallId = sSipCallId;
      if (sTmpSipCallId.isNull())
      {
         sTmpSipCallId = getNewSipCallId();
      }
      // we found call and have a lock on it
      return ptrLock->connect(sTmpSipCallId, sSipDialog, toAddress, fullLineUrl, locationHeader, contactId);
   }

   return result;
}

OsStatus XCpCallManager::connectConferenceCall(const UtlString& sConferenceId,
                                               SipDialog& sSipDialog,
                                               const UtlString& toAddress,
                                               const UtlString& fullLineUrl,
                                               const UtlString& sSipCallId,
                                               const UtlString& locationHeader,
                                               CP_CONTACT_ID contactId)
{
   OsStatus result = OS_NOT_FOUND;
   sSipDialog = SipDialog();

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      UtlString sTmpSipCallId = sSipCallId;
      if (sTmpSipCallId.isNull())
      {
         sTmpSipCallId = getNewSipCallId();
      }
      // we found call and have a lock on it
      return ptrLock->connect(sTmpSipCallId, sSipDialog, toAddress, fullLineUrl, locationHeader, contactId);
   }

   return result;
}

OsStatus XCpCallManager::acceptCallConnection(const UtlString& sCallId,
                                              const UtlString& locationHeader,
                                              CP_CONTACT_ID contactId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->acceptConnection(locationHeader, contactId);
   }

   return result;
}

OsStatus XCpCallManager::rejectCallConnection(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->rejectConnection();
   }

   return result;
}

OsStatus XCpCallManager::redirectCallConnection(const UtlString& sCallId,
                                                const UtlString& sRedirectSipUrl)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->redirectConnection(sRedirectSipUrl);
   }

   return result;
}

OsStatus XCpCallManager::answerCallConnection(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->answerConnection();
   }

   return result;
}

OsStatus XCpCallManager::dropAbstractCallConnection(const UtlString& sAbstractCallId,
                                                    const SipDialog& sSipDialog)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->dropConnection(sSipDialog);
   }

   return result;
}

OsStatus XCpCallManager::dropAllAbstractCallConnections(const UtlString& sAbstractCallId)
{
   if (XCpCallIdUtil::isCallId(sAbstractCallId))
   {
      // call has only 1 connection
      return dropCallConnection(sAbstractCallId);
   }
   else
   {
      // conference has many connections
      return dropAllConferenceConnections(sAbstractCallId);
   }
}

OsStatus XCpCallManager::dropCallConnection(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->dropConnection();
   }

   return result;
}

OsStatus XCpCallManager::dropConferenceConnection(const UtlString& sConferenceId, const SipDialog& sSipDialog)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->dropConnection(sSipDialog);
   }

   return result;
}

OsStatus XCpCallManager::dropAllConferenceConnections(const UtlString& sConferenceId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      // we found conference and have a lock on it
      return ptrLock->dropAllConnections();
   }

   return result;
}

OsStatus XCpCallManager::dropAbstractCall(const UtlString& sAbstractCallId)
{
   if (XCpCallIdUtil::isCallId(sAbstractCallId))
   {
      return dropCall(sAbstractCallId);
   }
   else
   {
      return dropConference(sAbstractCallId);
   }
}

OsStatus XCpCallManager::dropCall(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->dropConnection(TRUE);
   }

   return result;
}

OsStatus XCpCallManager::dropConference(const UtlString& sConferenceId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      // we found conference and have a lock on it
      return ptrLock->dropAllConnections(TRUE);
   }

   return result;
}

OsStatus XCpCallManager::destroyAbstractCall(const UtlString& sAbstractCallId)
{
   OsStatus result = OS_NOT_FOUND;

   // this deletes it safely, shutting down thread and media resources
   UtlBoolean resDelete = m_callStack.deleteAbstractCall(sAbstractCallId);
   if (resDelete)
   {
      result = OS_SUCCESS;
   }

   return result;
}

OsStatus XCpCallManager::destroyCall(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   // this deletes it safely, shutting down thread and media resources
   UtlBoolean resDelete = m_callStack.deleteCall(sCallId);
   if (resDelete)
   {
      result = OS_SUCCESS;
   }

   return result;
}

OsStatus XCpCallManager::destroyConference(const UtlString& sConferenceId)
{
   OsStatus result = OS_NOT_FOUND;

   // this deletes it safely, shutting down thread and media resources
   UtlBoolean resDelete = m_callStack.deleteConference(sConferenceId);
   if (resDelete)
   {
      result = OS_SUCCESS;
   }

   return result;
}

OsStatus XCpCallManager::transferBlindAbstractCall(const UtlString& sAbstractCallId,
                                                   const SipDialog& sSipDialog,
                                                   const UtlString& sTransferSipUrl)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->transferBlind(sSipDialog, sTransferSipUrl);
   }

   return result;
}

OsStatus XCpCallManager::transferConsultativeAbstractCall(const UtlString& sSourceAbstractCallId,
                                                          const SipDialog& sSourceSipDialog,
                                                          const UtlString& sTargetAbstractCallId,
                                                          const SipDialog& sTargetSipDialog)
{
   OsStatus result = OS_FAILED;

   return result;
}

OsStatus XCpCallManager::audioToneStart(const UtlString& sAbstractCallId,
                                        int iToneId,
                                        UtlBoolean bLocal,
                                        UtlBoolean bRemote)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioToneStart(iToneId, bLocal, bRemote);
   }

   return result;
}

OsStatus XCpCallManager::audioToneStop(const UtlString& sAbstractCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioToneStop();
   }

   return result;
}

OsStatus XCpCallManager::audioFilePlay(const UtlString& sAbstractCallId,
                                       const UtlString& audioFile,
                                       UtlBoolean bRepeat,
                                       UtlBoolean bLocal,
                                       UtlBoolean bRemote,
                                       UtlBoolean bMixWithMic /*= FALSE*/,
                                       int iDownScaling /*= 100*/,
                                       void* pCookie /*= NULL*/)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioFilePlay(audioFile, bRepeat, bLocal, bRemote, bMixWithMic, iDownScaling, pCookie);
   }

   return result;
}

OsStatus XCpCallManager::audioBufferPlay(const UtlString& sAbstractCallId,
                                         const void* pAudiobuf,
                                         size_t iBufSize,
                                         int iType,
                                         UtlBoolean bRepeat,
                                         UtlBoolean bLocal,
                                         UtlBoolean bRemote,
                                         UtlBoolean bMixWithMic /*= FALSE*/,
                                         int iDownScaling /*= 100*/,
                                         void* pCookie /*= NULL*/)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioBufferPlay(pAudiobuf, iBufSize, iType, bRepeat, bLocal, bRemote,
         bMixWithMic, iDownScaling, pCookie);
   }

   return result;
}

OsStatus XCpCallManager::audioStopPlayback(const UtlString& sAbstractCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioStopPlayback();
   }

   return result;
}

OsStatus XCpCallManager::pauseAudioPlayback(const UtlString& sAbstractCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->pauseAudioPlayback();
   }

   return result;
}

OsStatus XCpCallManager::resumeAudioPlayback(const UtlString& sAbstractCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->resumeAudioPlayback();
   }

   return result;
}

OsStatus XCpCallManager::audioRecordStart(const UtlString& sAbstractCallId,
                                          const UtlString& sFile)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioRecordStart(sFile);
   }

   return result;
}

OsStatus XCpCallManager::audioRecordStop(const UtlString& sAbstractCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->audioRecordStop();
   }

   return result;
}

OsStatus XCpCallManager::holdAbstractCallConnection(const UtlString& sAbstractCallId,
                                                    const SipDialog& sSipDialog)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->holdConnection(sSipDialog);
   }

   return result;
}

OsStatus XCpCallManager::holdCallConnection(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->holdConnection();
   }

   return result;
}

OsStatus XCpCallManager::holdAllConferenceConnections(const UtlString& sConferenceId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->holdAllConnections();
   }

   return result;
}

OsStatus XCpCallManager::holdLocalAbstractCallConnection(const UtlString& sAbstractCallId)
{
   return m_callStack.doYieldFocus(sAbstractCallId, TRUE);
}

OsStatus XCpCallManager::unholdLocalAbstractCallConnection(const UtlString& sAbstractCallId)
{
   return m_callStack.doGainFocus(sAbstractCallId, FALSE);
}

OsStatus XCpCallManager::unholdAllConferenceConnections(const UtlString& sConferenceId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->unholdAllConnections();
   }

   return result;
}

OsStatus XCpCallManager::unholdAbstractCallConnection(const UtlString& sAbstractCallId,
                                                      const SipDialog& sSipDialog)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->unholdConnection(sSipDialog);
   }

   return result;
}

OsStatus XCpCallManager::unholdCallConnection(const UtlString& sCallId)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->unholdConnection();
   }

   return result;
}

OsStatus XCpCallManager::muteInputAbstractCallConnection(const UtlString& sAbstractCallId,
                                                         const SipDialog& sSipDialog)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->muteInputConnection(sSipDialog);
   }

   return result;
}

OsStatus XCpCallManager::unmuteInputAbstractCallConnection(const UtlString& sAbstractCallId,
                                                           const SipDialog& sSipDialog)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->unmuteInputConnection(sSipDialog);
   }

   return result;
}

OsStatus XCpCallManager::limitAbstractCallCodecPreferences(const UtlString& sAbstractCallId,
                                                           const UtlString& sAudioCodecs,
                                                           const UtlString& sVideoCodecs)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->limitCodecPreferences(sAudioCodecs, sVideoCodecs);
   }

   return result;
}

OsStatus XCpCallManager::renegotiateCodecsAbstractCallConnection(const UtlString& sAbstractCallId,
                                                                 const SipDialog& sSipDialog,
                                                                 const UtlString& sAudioCodecs,
                                                                 const UtlString& sVideoCodecs)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->renegotiateCodecsConnection(sSipDialog,
         sAudioCodecs, sVideoCodecs);
   }

   return result;
}

OsStatus XCpCallManager::renegotiateCodecsAllConferenceConnections(const UtlString& sConferenceId,
                                                                   const UtlString& sAudioCodecs,
                                                                   const UtlString& sVideoCodecs)
{
   OsStatus result = OS_NOT_FOUND;

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->renegotiateCodecsAllConnections(sAudioCodecs, sVideoCodecs);
   }

   return result;
}

void XCpCallManager::enableStun(const UtlString& sStunServer,
                                int iServerPort,
                                int iKeepAlivePeriodSecs /*= 0*/,
                                OsNotification* pNotification /*= NULL*/)
{
   OsLock lock(m_memberMutex); // use wide lock to make sure we enable stun for the correct server

   m_sStunServer = sStunServer;
   m_iStunPort = iServerPort;
   m_iStunKeepAlivePeriodSecs = iKeepAlivePeriodSecs;

   m_rSipUserAgent.enableStun(sStunServer, iServerPort, iKeepAlivePeriodSecs, pNotification);
}

void XCpCallManager::enableTurn(const UtlString& sTurnServer,
                                int iTurnPort,
                                const UtlString& sTurnUsername,
                                const UtlString& sTurnPassword,
                                int iKeepAlivePeriodSecs /*= 0*/)
{
   OsLock lock(m_memberMutex);

   bool bEnabled = false;
   m_sTurnServer = sTurnServer;
   m_iTurnPort = iTurnPort;
   m_sTurnUsername = sTurnUsername;
   m_sTurnPassword = sTurnPassword;
   m_iTurnKeepAlivePeriodSecs = iKeepAlivePeriodSecs;
   bEnabled = (m_sTurnServer.length() > 0) && portIsValid(m_iTurnPort);

   m_rSipUserAgent.getContactDb().enableTurn(bEnabled);
}

OsStatus XCpCallManager::sendInfo(const UtlString& sAbstractCallId,
                                  const SipDialog& sSipDialog,
                                  const UtlString& sContentType,
                                  const char* pContent,
                                  const size_t nContentLength)
{
   OsStatus result = OS_FAILED;
   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->sendInfo(sSipDialog, sContentType, pContent, nContentLength);
   }

   return result;
}

UtlString XCpCallManager::getNewSipCallId()
{
   return m_sipCallIdGenerator.getNewCallId();
}

UtlString XCpCallManager::getNewCallId()
{
   return XCpCallIdUtil::getNewCallId();
}

UtlString XCpCallManager::getNewConferenceId()
{
   return XCpCallIdUtil::getNewConferenceId();
}

/* ============================ ACCESSORS ================================= */

CpMediaInterfaceFactory* XCpCallManager::getMediaInterfaceFactory() const
{
   return &m_rMediaInterfaceFactory;
}

/* ============================ INQUIRY =================================== */

int XCpCallManager::getCallCount() const
{
   return m_callStack.getCallCount();
}

OsStatus XCpCallManager::getAbstractCallIds(UtlSList& idList) const
{
   // first append call ids
   getCallIds(idList);
   // then append conference ids
   getConferenceIds(idList);

   return OS_SUCCESS;
}

OsStatus XCpCallManager::getCallIds(UtlSList& callIdList) const
{
   return m_callStack.getCallIds(callIdList);
}

OsStatus XCpCallManager::getConferenceIds(UtlSList& conferenceIdList) const
{
   return m_callStack.getConferenceIds(conferenceIdList);
}

OsStatus XCpCallManager::getCallSipCallId(const UtlString& sCallId,
                                          UtlString& sSipCallId) const
{
   OsStatus result = OS_NOT_FOUND;
   sSipCallId.remove(0);

   OsPtrLock<XCpCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findCall(sCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getCallSipCallId(sSipCallId);
   }

   return result;
}

OsStatus XCpCallManager::getConferenceSipCallIds(const UtlString& sConferenceId,
                                                 UtlSList& sipCallIdList) const
{
   OsStatus result = OS_NOT_FOUND;
   sipCallIdList.destroyAll();

   OsPtrLock<XCpConference> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findConference(sConferenceId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getConferenceSipCallIds(sipCallIdList);
   }

   return result;
}

OsStatus XCpCallManager::getRemoteUserAgent(const UtlString& sAbstractCallId,
                                            const SipDialog& sSipDialog,
                                            UtlString& userAgent) const
{
   OsStatus result = OS_NOT_FOUND;
   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getRemoteUserAgent(sSipDialog, userAgent);
   }

   return result;
}

OsStatus XCpCallManager::getMediaConnectionId(const UtlString& sAbstractCallId,
                                              const SipDialog& sSipDialog,
                                              int& mediaConnID) const
{
   OsStatus result = OS_NOT_FOUND;
   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getMediaConnectionId(sSipDialog, mediaConnID);
   }

   return result;
}

OsStatus XCpCallManager::getSipDialog(const UtlString& sAbstractCallId,
                                      const SipDialog& sSipDialog,
                                      SipDialog& sOutputSipDialog) const
{
   OsStatus result = OS_NOT_FOUND;
   OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
   UtlBoolean resFind = m_callStack.findAbstractCall(sAbstractCallId, ptrLock);
   if (resFind)
   {
      // we found call and have a lock on it
      return ptrLock->getSipDialog(sSipDialog, sOutputSipDialog);
   }

   return result;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

UtlBoolean XCpCallManager::checkCallLimit()
{
   if (m_maxCalls == -1)
   {
      return TRUE;
   }

   {
      OsLock lock(m_memberMutex);
      int callCount = getCallCount();
      if (callCount >= m_maxCalls)
      {
         return FALSE;
      }
   }

   return TRUE;
}

// handles OsMsg::PHONE_APP messages
UtlBoolean XCpCallManager::handlePhoneAppMessage(const OsMsg& rRawMsg)
{
   UtlBoolean bResult = FALSE;
   int msgSubType = rRawMsg.getMsgSubType();

   switch (msgSubType)
   {
   case SipMessage::NET_SIP_MESSAGE:
      return handleSipMessageEvent((const SipMessageEvent&)rRawMsg);
   default:
      {
         OsSysLog::add(FAC_CP, PRI_ERR, "Unknown PHONE_APP CallManager message subtype: %d\n", msgSubType);
         break;
      }
   }

   return bResult;
}

UtlBoolean XCpCallManager::handleSipMessageEvent(const SipMessageEvent& rSipMsgEvent)
{
   const SipMessage* pSipMessage = rSipMsgEvent.getMessage();
   if (pSipMessage)
   {
#ifdef PRINT_SIP_MESSAGE
      osPrintf("\nXCpCallManager::handleSipMessageEvent\n%s\n-----------------------------------\n", pSipMessage->toString().data());
#endif
      OsPtrLock<XCpAbstractCall> ptrLock; // auto pointer lock
      UtlBoolean resFind = m_callStack.findHandlingAbstractCall(*pSipMessage, ptrLock);
      if (resFind)
      {
         // post message to call
         return ptrLock->postMessage(rSipMsgEvent);
      }
      else
      {
         return handleUnknownSipMessageEvent(rSipMsgEvent);
      }
   }

   return TRUE;
}

UtlBoolean XCpCallManager::handleUnknownSipMessageEvent(const SipMessageEvent& rSipMsgEvent)
{
   int messageType = rSipMsgEvent.getMessageStatus();

   switch(messageType)
   {
   case SipMessageEvent::APPLICATION:
      {
         const SipMessage* pSipMessage = rSipMsgEvent.getMessage();
         if (pSipMessage)
         {
            // Its a SIP Request
            if(pSipMessage->isRequest())
            {
               return handleUnknownSipRequest(*pSipMessage);
            }
         }
      }
   default:
      ;
   }

   return FALSE;
}

// Handler for inbound INVITE SipMessage, for which there is no existing call or conference.
UtlBoolean XCpCallManager::handleUnknownInviteRequest(const SipMessage& rSipMessage)
{
   UtlString toTag;
   rSipMessage.getToFieldTag(toTag);

   if (toTag.isNull())
   {
      // handle INVITE without to tag
      // maybe check if line exists
      if(m_pSipLineProvider && m_bIsRequiredLineMatch)
      {
         UtlBoolean isLineValid = m_pSipLineProvider->lineExists(rSipMessage);
         if (!isLineValid)
         {
            // no such user - return 404
            SipMessage noSuchUserResponse;
            noSuchUserResponse.setResponseData(&rSipMessage,
               SIP_NOT_FOUND_CODE,
               SIP_NOT_FOUND_TEXT);
            m_rSipUserAgent.send(noSuchUserResponse);
            return TRUE;
         }
      }
      // check if we didn't reach call limit
      if (checkCallLimit())
      {
         if (!m_bDoNotDisturb)
         {
            // all checks passed, create new call
            createNewInboundCall(rSipMessage);
         }
         else
         {
            UtlString requestUri;
            rSipMessage.getRequestUri(&requestUri);
            OsSysLog::add(FAC_CP, PRI_DEBUG, "XCpCallManager::handleSipMessage - Rejecting inbound call to %s due to DND mode", requestUri.data());
            // send 486 Busy here
            SipMessage busyHereResponse;
            busyHereResponse.setInviteBusyData(&rSipMessage);
            m_rSipUserAgent.send(busyHereResponse);
         }
      }
      else
      {
         OsSysLog::add(FAC_CP, PRI_WARNING, "XCpCallManager::handleSipMessage - The call stack size as reached it's limit of %d", m_maxCalls);
         // send 486 Busy here
         SipMessage busyHereResponse;
         busyHereResponse.setInviteBusyData(&rSipMessage);
         m_rSipUserAgent.send(busyHereResponse);
      }

      return TRUE;
   }
   else
   {
      // to tag present but dialog not found -> doesn't exist
      sendBadTransactionError(rSipMessage);
      return TRUE;
   }
}

// Handler for inbound OPTIONS SipMessage, for which there is no existing call or conference. 
UtlBoolean XCpCallManager::handleUnknownOptionsRequest(const SipMessage& rSipMessage)
{
   UtlString fromTag;
   rSipMessage.getFromFieldTag(fromTag);
   UtlString toTag;
   rSipMessage.getToFieldTag(toTag);

   if (!toTag.isNull())
   {
      // to tag present but dialog not found -> doesn't exist
      sendBadTransactionError(rSipMessage);
      return TRUE;
   }
   else if (fromTag.isNull())
   {
      // both tags are NULL
      // TODO: Investigate if this is the correct place to handle OPTIONS out of dialog
   }
   return FALSE;
}

UtlBoolean XCpCallManager::handleUnknownReferRequest(const SipMessage& rSipMessage)
{
   UtlString toTag;
   rSipMessage.getToFieldTag(toTag);

   if (!toTag.isNull())
   {
      // to tag present but dialog not found -> doesn't exist
      sendBadTransactionError(rSipMessage);
      return TRUE;
   }
   else
   {
      // handle out of dialog REFER
      // TODO: Investigate how this should be handled
      return TRUE;
   }
}

UtlBoolean XCpCallManager::handleUnknownCancelRequest(const SipMessage& rSipMessage)
{
   // always send 481 Call/Transaction Does Not Exist for unknown CANCEL requests
   sendBadTransactionError(rSipMessage);
   return TRUE;
}

// called for inbound request SipMessages, for which calls weren't found
UtlBoolean XCpCallManager::handleUnknownSipRequest(const SipMessage& rSipMessage)
{
   UtlString requestMethod;
   rSipMessage.getRequestMethod(&requestMethod);

   // Dangling or delayed ACK
   if(requestMethod.compareTo(SIP_ACK_METHOD) == 0)
   {
      return TRUE;
   }
   else if(requestMethod.compareTo(SIP_INVITE_METHOD) == 0)
   {
      return handleUnknownInviteRequest(rSipMessage);
   }
   else if(requestMethod.compareTo(SIP_OPTIONS_METHOD) == 0)
   {
      return handleUnknownOptionsRequest(rSipMessage);
   }
   else if(requestMethod.compareTo(SIP_REFER_METHOD) == 0)
   {
      return handleUnknownReferRequest(rSipMessage);
   }
   else if(requestMethod.compareTo(SIP_CANCEL_METHOD) == 0)
   {
      return handleUnknownCancelRequest(rSipMessage);
   }

   // 481 Call/Transaction Does Not Exist must be sent automatically by transaction layer for other messages (INFO, NOTIFY)
   // multiple observers may receive the same SipMessage
   return FALSE;
}

void XCpCallManager::createNewInboundCall(const SipMessage& rSipMessage)
{
   UtlString sSipCallId = getNewSipCallId();

   XCpCall* pCall = new XCpCall(sSipCallId, m_rSipUserAgent, m_rMediaInterfaceFactory, m_rDefaultSdpCodecList, 
      *getMessageQueue(), &m_callStack, m_pCallEventListener, m_pInfoStatusEventListener, m_pSecurityEventListener,
      m_pMediaEventListener);

   UtlBoolean resStart = pCall->start(); // start thread
   if (resStart)
   {
      SipMessageEvent sipMessageEvent(rSipMessage);
      pCall->postMessage(sipMessageEvent); // repost message into thread
      UtlBoolean resPush = m_callStack.push(*pCall); // inserts call into list of calls
      if (resPush)
      {
         if (OsSysLog::willLog(FAC_CP, PRI_DEBUG))
         {
            UtlString requestUri;
            rSipMessage.getRequestUri(&requestUri);
            UtlString fromField;
            rSipMessage.getFromField(&fromField);
            OsSysLog::add(FAC_CP, PRI_DEBUG, "XCpCallManager::createNewCall - Creating new call destined to %s from %s", requestUri.data(), fromField.data());
         }
      }
      else
      {
         OsSysLog::add(FAC_CP, PRI_ERR, "XCpCallManager::createNewCall - Couldn't push call on stack");
         pCall->requestShutdown();
         delete pCall;
      }
   }
   else
   {
      OsSysLog::add(FAC_CP, PRI_ERR, "XCpCallManager::createNewCall - Call thread could not be started");
   }
}

UtlBoolean XCpCallManager::sendBadTransactionError(const SipMessage& rSipMessage)
{
   SipMessage badTransactionMessage;
   badTransactionMessage.setBadTransactionData(&rSipMessage);
   return m_rSipUserAgent.send(badTransactionMessage);
}

void XCpCallManager::startSipMessageObserving()
{
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_INVITE_METHOD,
      TRUE, // want to get requests
      TRUE, // and responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_BYE_METHOD,
      TRUE, // want to get requests
      TRUE, // and responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_CANCEL_METHOD,
      TRUE, // want to get requests
      TRUE, // and responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_ACK_METHOD,
      TRUE, // want to get requests
      FALSE, // no such thing as a ACK response
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_REFER_METHOD,
      TRUE, // want to get requests
      TRUE, // and responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_OPTIONS_METHOD,
      FALSE, // don't want to get requests
      TRUE, // do want responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_NOTIFY_METHOD,
      TRUE, // do want to get requests
      TRUE, // do want responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
   m_rSipUserAgent.addMessageObserver(*(this->getMessageQueue()),
      SIP_INFO_METHOD,
      TRUE, // do want to get requests
      TRUE, // do want responses
      TRUE, // Incoming messages
      FALSE); // Don't want to see out going messages
}

void XCpCallManager::stopSipMessageObserving()
{
   m_rSipUserAgent.removeMessageObserver(*(this->getMessageQueue()));
}

/* ============================ FUNCTIONS ================================= */
