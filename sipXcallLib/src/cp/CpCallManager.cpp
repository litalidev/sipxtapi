//
// Copyright (C) 2005-2006 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// Author: Daniel Petrie dpetrie AT SIPez DOT com

// SYSTEM INCLUDES
#include "os/OsDefs.h"
#include <assert.h>

// APPLICATION INCLUDES
#include <os/OsLock.h>
#include <os/OsDateTime.h>
#include <os/OsSocket.h>
#include <os/OsReadLock.h>
#include <os/OsWriteLock.h>
#include <os/OsProcess.h>
#include <cp/CpCallManager.h>
#include <cp/CpCall.h>
#include <net/NetMd5Codec.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
#ifdef __pingtel_on_posix__ /* [ */
const int    CpCallManager::CALLMANAGER_MAX_REQUEST_MSGS = 6000;
#else
const int    CpCallManager::CALLMANAGER_MAX_REQUEST_MSGS = 1000;
#endif  

Int64 CpCallManager::mCallNum = 0;
OsMutex CpCallManager::mCallNumMutex(OsMutex::Q_FIFO);

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
CpCallManager::CpCallManager(const char* taskName,
                             const char* callIdPrefix,
                             int rtpPortStart,
                             int rtpPortEnd,
                             const char* localAddress,
                             const char* publicAddress) :
OsServerTask(taskName, NULL, CALLMANAGER_MAX_REQUEST_MSGS),
mManagerMutex(OsMutex::Q_FIFO),
mCallListMutex(OsMutex::Q_FIFO),
mCallIndices()
{
    mDoNotDisturbFlag = FALSE;
    mMsgWaitingFlag = FALSE;
    mOfferedTimeOut = 0;
    
   if(callIdPrefix)
   {
       mCallIdPrefix.append(callIdPrefix);
   }

   mRtpPortStart = rtpPortStart;
   mRtpPortEnd = rtpPortEnd;

   if(localAddress && *localAddress)
   {
       mLocalAddress.append(localAddress);
   }
   else
   {
       OsSocket::getHostIp(&mLocalAddress);
   }

   if(publicAddress && *publicAddress)
   {
       mPublicAddress.append(publicAddress);
   }
   else
   {
       OsSocket::getHostIp(&mPublicAddress);
   }

   mLastMetaEventId = 0;
   mbEnableICE = false ;

}

// Copy constructor
CpCallManager::CpCallManager(const CpCallManager& rCpCallManager) :
OsServerTask("badCallManagerCopy"),
mManagerMutex(OsMutex::Q_FIFO),
mCallListMutex(OsMutex::Q_FIFO)
{
    mDoNotDisturbFlag = rCpCallManager.mDoNotDisturbFlag;
    mMsgWaitingFlag = rCpCallManager.mMsgWaitingFlag;
    mOfferedTimeOut = rCpCallManager.mOfferedTimeOut;

    mLastMetaEventId = 0;
    mbEnableICE = rCpCallManager.mbEnableICE ; 
}

// Destructor
CpCallManager::~CpCallManager()
{
}

/* ============================ MANIPULATORS ============================== */

void CpCallManager::getEventSubTypeString(EventSubTypes type,
                                          UtlString& typeString)
{
    switch(type)
    {
      case CP_UNSPECIFIED:
        typeString = "CP_UNSPECIFIED";
        break;

      case CP_SIP_MESSAGE:
        typeString = "CP_SIP_MESSAGE";
        break;

      case CP_CALL_EXITED:
        typeString = "CP_CALL_EXITED";
        break;

      case CP_DIAL_STRING:
        typeString = "CP_DIAL_STRING";
        break;

      case CP_YIELD_FOCUS:
        typeString = "CP_YIELD_FOCUS";
        break;

      case CP_GET_FOCUS:
        typeString = "CP_GET_FOCUS";
        break;

      case CP_CREATE_CALL:
        typeString = "CP_CREATE_CALL";
        break;

      case CP_CONNECT:
        typeString = "CP_CONNECT";
        break;

      case CP_BLIND_TRANSFER:
        typeString = "CP_BLIND_TRANSFER";
        break;

      case CP_CONSULT_TRANSFER:
        typeString = "CP_CONSULT_TRANSFER";
        break;

      case CP_CONSULT_TRANSFER_ADDRESS:
          typeString = "CP_CONSULT_TRANSFER_ADDRESS";
        break;

      case CP_TRANSFER_CONNECTION:
        typeString = "CP_TRANSFER_CONNECTION";
        break;

      case CP_TRANSFER_CONNECTION_STATUS:
        typeString = "CP_TRANSFER_CONNECTION_STATUS";
        break;

      case CP_TRANSFEREE_CONNECTION:
        typeString = "CP_TRANSFEREE_CONNECTION";
        break;

      case CP_TRANSFEREE_CONNECTION_STATUS:
        typeString = "CP_TRANSFEREE_CONNECTION_STATUS";
        break;

      case CP_DROP:
        typeString = "CP_DROP";
        break;

      case CP_DROP_CONNECTION:
        typeString = "CP_DROP_CONNECTION";
        break;

      case CP_FORCE_DROP_CONNECTION:
        typeString = "CP_FORCE_DROP_CONNECTION";
        break;

      case CP_ANSWER_CONNECTION:
        typeString = "CP_ANSWER_CONNECTION";
        break;

      case CP_ACCEPT_CONNECTION:
        typeString = "CP_ACCEPT_CONNECTION";
        break;

      case CP_REJECT_CONNECTION:
        typeString = "CP_REJECT_CONNECTION";
        break;

      case CP_REDIRECT_CONNECTION:
        typeString = "CP_REDIRECT_CONNECTION";
        break;

      case CP_GET_NUM_CONNECTIONS:
        typeString = "CP_GET_NUM_CONNECTIONS";
        break;

      case CP_GET_CONNECTIONS:
        typeString = "CP_GET_CONNECTIONS";
        break;

      case CP_GET_CALLED_ADDRESSES:
        typeString = "CP_GET_CALLED_ADDRESSES";
        break;
 
      case CP_GET_CALLING_ADDRESSES:
        typeString = "CP_GET_CALLING_ADDRESSES";
        break;

      case CP_PLAY_AUDIO_TERM_CONNECTION:
        typeString = "CP_PLAY_AUDIO_TERM_CONNECTION";
        break;

      case CP_STOP_AUDIO_TERM_CONNECTION:
        typeString = "CP_STOP_AUDIO_TERM_CONNECTION";
        break;

      case CP_CREATE_PLAYER:
        typeString = "CP_CREATE_PLAYER";
        break;

      case CP_DESTROY_PLAYER:
        typeString = "CP_DESTROY_PLAYER";
        break;

      case CP_IS_LOCAL_TERM_CONNECTION:
        typeString = "CP_IS_LOCAL_TERM_CONNECTION";
        break;

      case CP_HOLD_TERM_CONNECTION:
        typeString = "CP_HOLD_TERM_CONNECTION";
        break;

      case CP_UNHOLD_TERM_CONNECTION:
        typeString = "CP_UNHOLD_TERM_CONNECTION";
        break;

      case CP_UNHOLD_LOCAL_TERM_CONNECTION:
        typeString = "CP_UNHOLD_LOCAL_TERM_CONNECTION";
        break;

      case CP_HOLD_LOCAL_TERM_CONNECTION:
        typeString = "CP_HOLD_LOCAL_TERM_CONNECTION";
        break;
 
      case CP_OFFERING_EXPIRED:
        typeString = "CP_OFFERING_EXPIRED";
        break;

      case CP_RINGING_EXPIRED:
        typeString = "CP_RINGING_EXPIRED";
        break;

      case CP_GET_SESSION:
        typeString = "CP_GET_SESSION";
        break;

     case CP_HOLD_ALL_TERM_CONNECTIONS:
        typeString = "CP_HOLD_ALL_TERM_CONNECTIONS";
        break;

     case CP_UNHOLD_ALL_TERM_CONNECTIONS:
        typeString = "CP_HOLD_ALL_TERM_CONNECTIONS";
        break ;

      case CP_CANCEL_TIMER:
        typeString = "CP_CANCEL_TIMER";
        break;
 
      default:
        typeString = "?";
        break;
    }
}

// Assignment operator
CpCallManager& 
CpCallManager::operator=(const CpCallManager& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

    mDoNotDisturbFlag = rhs.mDoNotDisturbFlag;
    mMsgWaitingFlag = rhs.mMsgWaitingFlag;
    mOfferedTimeOut = rhs.mOfferedTimeOut;
    mbEnableICE = rhs.mbEnableICE ;

    return *this;
}

void CpCallManager::getNewCallId(UtlString* callId)
{
    getNewCallId(mCallIdPrefix, callId);
}

void CpCallManager::getNewSessionId(UtlString* callId)
{
    getNewCallId("s", callId);
}

// This implements a new strategy for generating Call-IDs after the
// problems described in http://track.sipfoundry.org/browse/XCL-51.
//
// The Call-ID is composed of several fields:
// - a prefix supplied by the caller
// - a counter
// - the Process ID
// - a start time, to microsecond resolution
//   (when getNewCallId was first called)
// - the host's name or IP address
// The last three fields are hashed, and the first 16 characters
// are used to represent them.
//
// The host name, process ID, start time, and counter together ensure
// uniqueness.  The start time is used to microsecond resolution
// because on Windows process IDs can be recycled quickly.  The
// counter is a long-long-int because at a high ID generation rate
// (1000 per second), an int counter can roll over in less than a
// month.
//
// Replacing the final three fields with the first 16 characters of
// their MD5 hash shortens the generated Call-IDs, as the last three
// fields can easily exceed 30 characters.  Retaining 16 characters
// (64 bits) should ensure no collisions until the number of
// concurrent calls reaches 2^32.
//
// The sections of the Call-ID are separated with "/" because those
// cannot otherwise appear in the final components, and so a generated
// Call-ID can be unambiguously parsed into its components from back
// to front, regardless of the user-supplied prefix, which ensures
// that regardless of the prefix, this function can never generate
// duplicate Call-IDs.
//
// The generated Call-IDs are "words" according to the syntax of RFC
// 3261.  We do not append "@host" for simplicity.  The callIdPrefix
// is assumed to contain only "word" characters, but we check to
// ensure that "@" does not appear because the earlier version of this
// routine checked for "@" and replaced it with "_".  The earlier
// version checked whether the host name contained "@" and replaced it
// with "_", but this version replaces it with "*".  The earlier
// version also checked whether the host name contained ":" and
// replaced it with "_", but I do not see why, as ":" is a "word"
// character.  This version does not.
//
// The counter mCallNum is incremented by 19560001 rather than 1 so
// that successive Call-IDs differ in more than 1 character, and so
// hash better.  This does not reduce the cycle time of the counter,
// since 19560001 is relatively prime to the limit of a long-long-int,
// 2^64.  Because the increment ends in "0001", the final four digits
// of this field of the Call-ID count in a human-readable way.
//
// Ideally, Call-IDs would have crypto-quality randomness (as
// recommended in RFC 3261 section 8.1.1.4), but the previous version
// did not either.
void CpCallManager::getNewCallId(const char* callIdPrefix, UtlString* callId)
{
   // Lock to protect mCallNum.
   OsLock lock(mCallNumMutex);

   // Buffer in which we will compose the new Call-ID.
   char buffer[256];

   // Static information that is initialized once.
   static UtlString suffix;

   // Flag to record if the data has been initialized.
   static UtlBoolean initialized = FALSE;

   // Increment the call number.
#ifdef USE_LONG_CALL_IDS
   mCallNum += 19560001;
#else
   mCallNum += 1201;
#endif
    
   // callID prefix shouldn't contain an @.
   if (strchr(callIdPrefix, '@') != NULL)
   {
      OsSysLog::add(FAC_CP, PRI_ERR,
                    "CpCallManager::getNewCallId callIdPrefix='%s' contains '@'",
                    callIdPrefix);
   }

   // If we haven't initialized yet, do so.
   if (!initialized)
   {
      // Get the start time.
      OsTime current_time;
      OsDateTime::getCurTime(current_time);
      Int64 start_time =
         ((Int64) current_time.seconds()) * 1000000 + current_time.usecs();

      // Get the process ID.
      int process_id;
      process_id = OsProcess::getCurrentPID();

      // Get the host identity.
      UtlString thisHost;
      OsSocket::getHostIp(&thisHost);
      // Ensure it does not contain @.
      thisHost.replace('@','*');

      // Compose the static fields.
      sprintf(buffer, "%d_%" FORMAT_INTLL "d_%s",
              process_id, start_time, thisHost.data());
      // Hash them.
      NetMd5Codec encoder;
      encoder.encode(buffer, suffix);
#ifdef USE_LONG_CALL_IDS
      // Truncate the hash to 16 characters.
      suffix.remove(16);
#else
      // Truncate the hash to 16 characters.
      suffix.remove(12);
#endif

      // Note initialization is done.
      initialized = TRUE;
   }

   // Compose the new Call-Id.
   sprintf(buffer, "%s_%" FORMAT_INTLL "d_%s",
           callIdPrefix, mCallNum, suffix.data());

   // Copy it to the destination.
   *callId = buffer;
}

void CpCallManager::appendCall(CpCall* call)
{
    OsWriteLock lock(mCallListMutex);
    UtlInt* callCollectable = new UtlInt((int)call);
    mCallList.append(callCollectable);
}

void CpCallManager::pushCall(CpCall* call)
{
    OsWriteLock lock(mCallListMutex);
    UtlInt* callCollectable = new UtlInt((int)call);
    mCallList.insertAt(0, callCollectable);
}


void CpCallManager::setDoNotDisturb(int flag)
{
    mDoNotDisturbFlag = flag;
}

void CpCallManager::setMessageWaiting(int flag)
{
    mMsgWaitingFlag = flag;
}

void CpCallManager::setOfferedTimeout(int milisec)
{
    mOfferedTimeOut = milisec;
}

void CpCallManager::enableIce(UtlBoolean bEnable) 
{
    mbEnableICE = bEnable ;
}


void CpCallManager::setVoiceQualityReportTarget(const char* szTargetSipUrl) 
{
    mVoiceQualityReportTarget = szTargetSipUrl ;
}


/* ============================ ACCESSORS ================================= */

int CpCallManager::getNewMetaEventId()
{
    mLastMetaEventId++;
    return(mLastMetaEventId);
}

UtlBoolean CpCallManager::isIceEnabled() const
{
    return mbEnableICE ;
}


UtlBoolean CpCallManager::getVoiceQualityReportTarget(UtlString& reportSipUrl) 
{
    UtlBoolean bRC = false ;

    if (!mVoiceQualityReportTarget.isNull())
    {
        reportSipUrl = mVoiceQualityReportTarget ;
        bRC = true ;
    }

    return bRC ;
}

void CpCallManager::getLocalAddress(UtlString& address) 
{
    address = mLocalAddress ;
}



/* ============================ INQUIRY =================================== */
UtlBoolean CpCallManager::isCallStateLoggingEnabled()
{
    return(mCallStateLogEnabled);
}


/* //////////////////////////// PROTECTED ///////////////////////////////// */

int CpCallManager::aquireCallIndex()
{
   int index = 0;
   UtlInt matchCallIndexColl;

   // Find the first unused slot
   UtlInt* existingCallIndex = NULL;
   do
   {
      index++;
      matchCallIndexColl.setValue(index);
      existingCallIndex = (UtlInt*) mCallIndices.find(&matchCallIndexColl);

   }
   while(existingCallIndex);

   // Insert the new one
   mCallIndices.insert(new UtlInt(matchCallIndexColl));
   return(index);
}

void CpCallManager::releaseCallIndex(int callIndex)
{
   if(callIndex > 0)
   {
      UtlInt matchCallIndexColl(callIndex);
      UtlInt* callIndexColl = NULL;
      callIndexColl = (UtlInt*) mCallIndices.remove(&matchCallIndexColl);

      if(callIndexColl) delete callIndexColl;
      callIndexColl = NULL;
   }
}


/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

