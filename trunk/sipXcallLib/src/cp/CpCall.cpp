//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


// SYSTEM INCLUDES
#include <assert.h>
#include <stdlib.h>

// APPLICATION INCLUDES
#include <utl/UtlInit.h>
#include <os/OsReadLock.h>
#include <os/OsWriteLock.h>
#include <os/OsQueuedEvent.h>
#include <os/OsEventMsg.h>
#include "os/OsSysLog.h"
#include <cp/CpCall.h>
#include <mi/CpMediaInterface.h>
#include <cp/CpMultiStringMessage.h>
#include <cp/CpIntMessage.h>
#include "ptapi/PtCall.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define CALL_STACK_SIZE (24*1024)    // 24K stack for the call task
#       define LOCAL_ONLY 0
#       define LOCAL_AND_REMOTE 1
#define UI_TERMINAL_CONNECTION_STATE "TerminalConnectionState"
#define UI_CONNECTION_STATE "ConnectionState"

// STATIC VARIABLE INITIALIZATIONS
OsLockingList CpCall::sCallTrackingList;

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
CpCall::CpCall(CpCallManager* manager,
               CpMediaInterface* callMediaInterface,
               int callIndex,
               const char* callId,
               int holdType) :
OsServerTask("Call-%d", NULL, DEF_MAX_MSGS, DEF_PRIO, DEF_OPTIONS, CALL_STACK_SIZE),
mCallIdMutex(OsMutex::Q_FIFO)
{
    // add the call task name to a list so we can track leaked calls.
    UtlString strCallTaskName = getName();
    addToCallTrackingList(strCallTaskName);

    mCallInFocus = FALSE;
    mRemoteDtmf = FALSE;

    mpManager = manager;

    mDropping = FALSE;
    mLocalHeld = FALSE;

    mCallIndex = callIndex;
    if(callId && callId[0])
    {
        setCallId(callId);
    }
    mHoldType = holdType;
    if(mHoldType < CallManager::NEAR_END_HOLD ||
        mHoldType > CallManager::FAR_END_HOLD)
    {
        mHoldType = CallManager::NEAR_END_HOLD;
    }

    mToneListenerCnt = 0;
    nMaxNumToneListeners = MAX_NUM_TONE_LISTENERS;

    mpToneListeners = (TaoListenerDb**) malloc(sizeof(TaoListenerDb *)*nMaxNumToneListeners);

    if (!mpToneListeners)
    {
       osPrintf("***** ERROR ALLOCATING mpToneListeners IN CPCALL **** \n");
       return;
    }

    int i;

    for (i = 0; i < MAX_NUM_TONE_LISTENERS; i++)
        mpToneListeners[i] = 0;

    // Create the media processing channel
    mpMediaInterface = callMediaInterface;

    mCallState = PtCall::IDLE;
    mLocalConnectionState = PtEvent::CONNECTION_IDLE;
    mLocalTermConnectionState = PtTerminalConnection::IDLE;

    // Meta event intitialization
    mMetaEventId = 0;
    mMetaEventType = PtEvent::META_EVENT_NONE;
    mNumMetaEventCalls = 0;
    mpMetaEventCallIds = NULL;
    mMessageEventCount = -1;

    UtlString name = getName();
#ifdef TEST_PRINT
    OsSysLog::add(FAC_CP, PRI_DEBUG, "%s Call constructed: %s\n", name.data(), mCallId.data());
    osPrintf("%s constructed: %s\n", name.data(), mCallId.data());
#endif
}

// Destructor
CpCall::~CpCall()
{
    if (isStarted())
    {
        waitUntilShutDown();
    }
    // remove the call task name from the list (for tracking leaked calls)
    UtlString strCallTaskName = getName();
    removeFromCallTrackingList(strCallTaskName);

    if(mpMediaInterface)
    {
        mpMediaInterface->release();
        mpMediaInterface = NULL;
    }

    if (mToneListenerCnt > 0)  // check if listener exists.
    {
        for (int i = 0; i < mToneListenerCnt; i++)
        {
            if (mpToneListeners[i])
            {
                OsQueuedEvent *pEv = (OsQueuedEvent *) mpToneListeners[i]->mIntData;
                if (pEv)
                    delete pEv;
                delete mpToneListeners[i];
                mpToneListeners[i] = 0;
            }
        }
    }

    if (mpToneListeners)
    {
       free(mpToneListeners);
       mpToneListeners = NULL;
    }

    if(mpMetaEventCallIds)
    {
        //for(int i = 0; i < mNumMetaEventCalls; i++)
        //{
        //    if(mpMetaEventCallIds[i]) delete mpMetaEventCallIds[i];
        //    mpMetaEventCallIds[1] = NULL;
        //}
        delete[] mpMetaEventCallIds;
        mpMetaEventCallIds = NULL;
    }

    UtlString name = getName();
#ifdef TEST_PRINT
    OsSysLog::add(FAC_CP, PRI_DEBUG, "%s destructed: %s\n", name.data(), mCallId.data());
    osPrintf("%s destructed: %s\n", name.data(), mCallId.data());
#endif
    name.remove(0);
    mCallId.remove(0);
    mOriginalCallId.remove(0);
    mTargetCallId.remove(0);

}

/* ============================ MANIPULATORS ============================== */

void CpCall::setDropState(UtlBoolean state)
{
    mDropping = state;
}

void CpCall::setCallState(int responseCode, UtlString responseText, int state, int casue)
{
    if (state != mCallState)
    {
        switch(state)
        {
        case PtCall::INVALID:
            postTaoListenerMessage(responseCode, responseText, PtEvent::CALL_INVALID, CALL_STATE, casue);
            break;

        case PtCall::ACTIVE:
            postTaoListenerMessage(responseCode, responseText, PtEvent::CALL_ACTIVE, CALL_STATE, casue);
            break;

        default:
            break;
        }
    }

    mCallState = state;
}

UtlBoolean CpCall::handleMessage(OsMsg& eventMessage)
{
    int msgType = eventMessage.getMsgType();
    int msgSubType = eventMessage.getMsgSubType();
    //int key;
    //int hookState;
    CpMultiStringMessage* multiStringMessage = (CpMultiStringMessage*)&eventMessage;

    UtlBoolean processedMessage = TRUE;
    OsSysLog::add(FAC_CP, PRI_DEBUG, "CpCall::handleMessage message type: %d subtype %d\n", msgType, msgSubType);

    switch(msgType)
    {
    case OsMsg::PHONE_APP:

        switch(msgSubType)
        {
        case CallManager::CP_PLAY_AUDIO_TERM_CONNECTION:
            addHistoryEvent(msgSubType, multiStringMessage);
            {
                int repeat = ((CpMultiStringMessage&)eventMessage).getInt1Data();
                UtlBoolean local = ((CpMultiStringMessage&)eventMessage).getInt2Data();
                UtlBoolean remote = ((CpMultiStringMessage&)eventMessage).getInt3Data();
                UtlBoolean mixWithMic = ((CpMultiStringMessage&)eventMessage).getInt4Data();
                int downScaling = ((CpMultiStringMessage&)eventMessage).getInt5Data();
                UtlString url;
                ((CpMultiStringMessage&)eventMessage).getString2Data(url);

                if(mpMediaInterface)
                {
                    mpMediaInterface->playAudio(url.data(), repeat,
                        local, remote, mixWithMic, downScaling) ;
                }
            }
            break;

        case CallManager::CP_PLAY_BUFFER_TERM_CONNECTION:
            addHistoryEvent(msgSubType, multiStringMessage);
            {
                int repeat = ((CpMultiStringMessage&)eventMessage).getInt2Data();
                UtlBoolean local = ((CpMultiStringMessage&)eventMessage).getInt3Data();
                UtlBoolean remote = ((CpMultiStringMessage&)eventMessage).getInt4Data();
                int buffer = ((CpMultiStringMessage&)eventMessage).getInt5Data();
                int bufSize = ((CpMultiStringMessage&)eventMessage).getInt6Data();
                int type = ((CpMultiStringMessage&)eventMessage).getInt7Data();
                OsProtectedEvent* ev = (OsProtectedEvent*) ((CpMultiStringMessage&)eventMessage).getInt1Data();

                if(mpMediaInterface)
                {
                    mpMediaInterface->playBuffer((char*)buffer,
                    bufSize, type, repeat, local, remote, ev);
                }
            }
            break;

        case CallManager::CP_STOP_AUDIO_TERM_CONNECTION:
            addHistoryEvent(msgSubType, multiStringMessage);
            if(mpMediaInterface)
            {
                mpMediaInterface->stopAudio();
            }
            break;
        case CallManager::CP_CREATE_PLAYLIST_PLAYER:
            {
                UtlString callId;

                MpStreamPlaylistPlayer** ppPlayer = (MpStreamPlaylistPlayer **) ((CpMultiStringMessage&)eventMessage).getInt2Data();
                assert(ppPlayer != NULL);

                OsProtectedEvent* ev = (OsProtectedEvent*) ((CpMultiStringMessage&)eventMessage).getInt1Data();
#ifdef TEST_PRINT
                OsSysLog::add(FAC_CP, PRI_DEBUG,
                    "CpCall::handle creating MpStreamPlaylistPlayer ppPlayer 0x%08x ev 0x%08x",
                    (int)ppPlayer, (int)ev);
#endif

                addHistoryEvent(msgSubType, multiStringMessage);

                getCallId(callId);

                if (mpMediaInterface)
                {
                    mpMediaInterface->createPlaylistPlayer(ppPlayer, mpManager->getMessageQueue(), callId.data()) ;
                }

                if(OS_ALREADY_SIGNALED == ev->signal(0))
                {
                    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
                    eventMgr->release(ev);
                }
            }
            break;

        case CallManager::CP_CREATE_PLAYER:
            {
                UtlString callId;
                UtlString streamId ;

                MpStreamPlayer** ppPlayer = (MpStreamPlayer **) ((CpMultiStringMessage&)eventMessage).getInt2Data();

                assert(ppPlayer != NULL);

                OsProtectedEvent* ev = (OsProtectedEvent*) ((CpMultiStringMessage&)eventMessage).getInt1Data();
                int flags = ((CpMultiStringMessage&)eventMessage).getInt3Data();
#ifdef TEST_PRINT
                OsSysLog::add(FAC_CP, PRI_DEBUG,
                    "CpCall::handle creating MpStreamPlayer ppPlayer 0x%08x ev 0x%08x flags %d",
                    (int)ppPlayer, (int)ev, flags);
#endif

                addHistoryEvent(msgSubType, multiStringMessage);

                ((CpMultiStringMessage&)eventMessage).getString2Data(streamId);
                getCallId(callId);

                if (mpMediaInterface)
                {
                    mpMediaInterface->createPlayer((MpStreamPlayer**)ppPlayer, streamId, flags, mpManager->getMessageQueue(), callId.data()) ;
                }
                if(OS_ALREADY_SIGNALED == ev->signal(0))
                {
                    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
                    eventMgr->release(ev);
                }
            }
            break;

        case CallManager::CP_CREATE_QUEUE_PLAYER:
            {
                UtlString callId;

                MpStreamPlayer** ppPlayer = (MpStreamPlayer **) ((CpMultiStringMessage&)eventMessage).getInt2Data();

                assert(ppPlayer != NULL);

                OsProtectedEvent* ev = (OsProtectedEvent*) ((CpMultiStringMessage&)eventMessage).getInt1Data();
#ifdef TEST_PRINT
                OsSysLog::add(FAC_CP, PRI_DEBUG,
                    "CpCall::handle creating MpStreamQueuePlayer ppPlayer 0x%08x ev 0x%08x",
                    (int)ppPlayer, (int)ev);
#endif

                addHistoryEvent(msgSubType, multiStringMessage);

                getCallId(callId);

                if (mpMediaInterface)
                {
                    mpMediaInterface->createQueuePlayer((MpStreamQueuePlayer**)ppPlayer, mpManager->getMessageQueue(), callId.data()) ;
                }

                if(OS_ALREADY_SIGNALED == ev->signal(0))
                {
                    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
                    eventMgr->release(ev);
                }
            }
            break;

        case CallManager::CP_DESTROY_PLAYLIST_PLAYER:
            {
                MpStreamPlaylistPlayer* pPlayer ;

                addHistoryEvent(msgSubType, multiStringMessage);

                // Redispatch Request to flowgraph
                if(mpMediaInterface)
                {
                    pPlayer = (MpStreamPlaylistPlayer*) ((CpMultiStringMessage&)eventMessage).getInt2Data();
                    mpMediaInterface->destroyPlaylistPlayer(pPlayer) ;
                }

                // Signal Event so that the caller knows the work is done
                OsProtectedEvent* ev = (OsProtectedEvent*) ((CpMultiStringMessage&)eventMessage).getInt1Data();
                if(OS_ALREADY_SIGNALED == ev->signal(0))
                {
                    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
                    eventMgr->release(ev);
                }
            }
            break;

        case CallManager::CP_DESTROY_PLAYER:
            {
                MpStreamPlayer* pPlayer ;

                addHistoryEvent(msgSubType, multiStringMessage);

                // Redispatch Request to flowgraph
                if(mpMediaInterface)
                {
                    pPlayer = (MpStreamPlayer*) ((CpMultiStringMessage&)eventMessage).getInt2Data();
                    mpMediaInterface->destroyPlayer(pPlayer) ;
                }

                // Signal Event so that the caller knows the work is done
                OsProtectedEvent* ev = (OsProtectedEvent*) ((CpMultiStringMessage&)eventMessage).getInt1Data();
                if(OS_ALREADY_SIGNALED == ev->signal(0))
                {
                    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
                    eventMgr->release(ev);
                }
            }
            break;

        case CallManager::CP_DESTROY_QUEUE_PLAYER:
            {
                MpStreamPlayer* pPlayer ;

                addHistoryEvent(msgSubType, multiStringMessage);

                // Redispatch Request to flowgraph
                if(mpMediaInterface)
                {
                    pPlayer = (MpStreamPlayer*) ((CpMultiStringMessage&)eventMessage).getInt2Data();
                    mpMediaInterface->destroyQueuePlayer((MpStreamQueuePlayer*)pPlayer) ;
                }

                // Signal Event so that the caller knows the work is done
                OsProtectedEvent* ev = (OsProtectedEvent*) ((CpMultiStringMessage&)eventMessage).getInt1Data();
                if(OS_ALREADY_SIGNALED == ev->signal(0))
                {
                    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
                    eventMgr->release(ev);
                }
            }
            break;

        case CallManager::CP_DROP:
            addHistoryEvent(msgSubType, multiStringMessage);
            {
                UtlString callId;
                int metaEventId = ((CpMultiStringMessage&)eventMessage).getInt1Data();
                ((CpMultiStringMessage&)eventMessage).getString1Data(callId);

                hangUp(callId, metaEventId);                                         
            }
            break;

        default:
            processedMessage = handleCallMessage(eventMessage);
            break;
        }

        break;

    case OsMsg::OS_EVENT:
        {
            switch(msgSubType)
            {
            case OsEventMsg::NOTIFY:
                {
                        int eventData;
                        int pListener;
                        ((OsEventMsg&)eventMessage).getEventData(eventData);
                        ((OsEventMsg&)eventMessage).getUserData(pListener);
                        if (pListener)
                        {
                            char    buf[128];
                            UtlString arg;
                            int argCnt = 2;
                            int i;

                            getCallId(arg);
                            arg.append(TAOMESSAGE_DELIMITER);
                            SNPRINTF(buf, sizeof(buf), "%d", eventData);
                            arg.append(buf);

                            for (i = 0; i < mToneListenerCnt; i++)
                            {
                                if (mpToneListeners[i] && (mpToneListeners[i]->mpListenerPtr == pListener))
                                {
                                    arg.append(TAOMESSAGE_DELIMITER);
                                    arg.append(mpToneListeners[i]->mName);
                                    argCnt = 3;

                                    // post the dtmf event
                                    int eventId = TaoMessage::BUTTON_PRESS;

                                    TaoMessage msg(TaoMessage::EVENT,
                                        0,
                                        0,
                                        eventId,
                                        0,
                                        argCnt,
                                        arg);

                                    ((OsServerTask*)pListener)->postMessage((OsMsg&)msg);
                                }
                            }
                    }
                }
                break;

            default:
                processedMessage = FALSE;
                osPrintf("Unknown TYPE %d of Call message subtype: %d\n", msgType, msgSubType);
                break;
            }
        }
        break ;

    case OsMsg::STREAMING_MSG:
        if (mpMediaInterface)
        {
            mpMediaInterface->getMsgQ()->send(eventMessage) ;
        }
        break ;
    case OsMsg::MP_CONNECTION_NOTF_MSG:
       handleConnectionNotfMessage(eventMessage);
       break;
    case OsMsg::MP_INTERFACE_NOTF_MSG:
       handleInterfaceNotfMessage(eventMessage);
       break;
    default:
        processedMessage = FALSE;
        osPrintf("Unknown TYPE %d of Call message subtype: %d\n", msgType, msgSubType);
        break;
    }

    //    osPrintf("exiting CpCall::handleMessage\n");
    return(processedMessage);
}

void CpCall::inFocus(int talking)
{
    mCallInFocus = TRUE;

    mLocalConnectionState = PtEvent::CONNECTION_ESTABLISHED;
    if (talking)
        mLocalTermConnectionState = PtTerminalConnection::TALKING;
    else
        mLocalTermConnectionState = PtTerminalConnection::IDLE;

    if(mpMediaInterface)
    {
        mpMediaInterface->giveFocus();
    }


}

void CpCall::outOfFocus()
{
    mCallInFocus = FALSE;

    if(mpMediaInterface)
    {
        mpMediaInterface->defocus();
    }
}

void CpCall::localHold()
{
    if(!mLocalHeld)
    {
        mLocalHeld = TRUE;
        mpManager->yieldFocus(this);
        /*
        // Post a message to the callManager to change focus
        CpIntMessage localHoldMessage(CallManager::CP_YIELD_FOCUS,
            (int)this);
        mLocalTermConnectionState = PtTerminalConnection::HELD;
        mpManager->postMessage(localHoldMessage);
        */
    }
}

void CpCall::hangUp(UtlString callId, int metaEventId)
{
#ifdef TEST_PRINT
    osPrintf("CpCall::hangUp\n");
#endif
    mDropping = TRUE;
    mLocalConnectionState = PtEvent::CONNECTION_DISCONNECTED;
    mLocalTermConnectionState = PtTerminalConnection::DROPPED;

    if (metaEventId > 0)
        setMetaEvent(metaEventId, PtEvent::META_CALL_ENDING, 0, 0);
    else
        startMetaEvent(mpManager->getNewMetaEventId(), PtEvent::META_CALL_ENDING, 0, 0);

    onHook();
}

void CpCall::setLocalConnectionState(int newState)
{
    mLocalConnectionState = newState;
}

/* ============================ ACCESSORS ================================= */

int CpCall::getCallIndex()
{
    return(mCallIndex);
}

int CpCall::getCallState()
{
    return(mCallState);
}

void CpCall::getCallId(UtlString& callId)
{
    OsReadLock lock(mCallIdMutex);
    callId = mCallId;
}

void CpCall::setCallId(const char* callId)
{
    OsWriteLock lock(mCallIdMutex);
    mCallId.remove(0);
    if(callId) mCallId.append(callId);
}

int CpCall::getLocalConnectionState(int state)
{
    int newState;

    switch(state)
    {
    case PtEvent::CONNECTION_CREATED:
    case PtEvent::CONNECTION_INITIATED:
        newState = Connection::CONNECTION_INITIATED;
        break;

    case PtEvent::CONNECTION_ALERTING:
        newState = Connection::CONNECTION_ALERTING;
        break;

    case PtEvent::CONNECTION_DISCONNECTED:
        newState = Connection::CONNECTION_DISCONNECTED;
        break;

    case PtEvent::CONNECTION_FAILED:
        newState = Connection::CONNECTION_FAILED;
        break;

    case PtEvent::CONNECTION_DIALING:
        newState = Connection::CONNECTION_DIALING;
        break;

    case PtEvent::CONNECTION_ESTABLISHED:
        newState = Connection::CONNECTION_ESTABLISHED;
        break;

    case PtEvent::CONNECTION_NETWORK_ALERTING:
        newState = Connection::CONNECTION_NETWORK_ALERTING;
        break;

    case PtEvent::CONNECTION_NETWORK_REACHED:
        newState = Connection::CONNECTION_NETWORK_REACHED;
        break;

    case PtEvent::CONNECTION_OFFERED:
        newState = Connection::CONNECTION_OFFERING;
        break;

    case PtEvent::CONNECTION_QUEUED:
        newState = Connection::CONNECTION_QUEUED;
        break;

    default:
        newState = Connection::CONNECTION_UNKNOWN;
        break;

    }

    return newState;
}

void CpCall::getStateString(int state, UtlString* stateLabel)
{
    stateLabel->remove(0);

    switch(state)
    {
    case PtEvent::CONNECTION_CREATED:
        stateLabel->append("CONNECTION_CREATED");
        break;

    case PtEvent::CONNECTION_ALERTING:
        stateLabel->append("CONNECTION_ALERTING");
        break;

    case PtEvent::CONNECTION_DISCONNECTED:
        stateLabel->append("CONNECTION_DISCONNECTED");
        break;

    case PtEvent::CONNECTION_FAILED:
        stateLabel->append("CONNECTION_FAILED");
        break;

    case PtEvent::CONNECTION_DIALING:
        stateLabel->append("CONNECTION_DIALING");
        break;

    case PtEvent::CONNECTION_ESTABLISHED:
        stateLabel->append("CONNECTION_ESTABLISHED");
        break;

    case PtEvent::CONNECTION_INITIATED:
        stateLabel->append("CONNECTION_INITIATED");
        break;

    case PtEvent::CONNECTION_NETWORK_ALERTING:
        stateLabel->append("CONNECTION_NETWORK_ALERTING");
        break;

    case PtEvent::CONNECTION_NETWORK_REACHED:
        stateLabel->append("CONNECTION_NETWORK_REACHED");
        break;

    case PtEvent::CONNECTION_OFFERED:
        stateLabel->append("CONNECTION_OFFERED");
        break;

    case PtEvent::CONNECTION_QUEUED:
        stateLabel->append("CONNECTION_QUEUED");
        break;

    case PtEvent::TERMINAL_CONNECTION_CREATED:
        stateLabel->append("TERMINAL_CONNECTION_CREATED");
        break;

    case PtEvent::TERMINAL_CONNECTION_RINGING:
        stateLabel->append("TERMINAL_CONNECTION_RINGING");
        break;

    case PtEvent::TERMINAL_CONNECTION_DROPPED:
        stateLabel->append("TERMINAL_CONNECTION_DROPPED");
        break;

    case PtEvent::TERMINAL_CONNECTION_UNKNOWN:
        stateLabel->append("TERMINAL_CONNECTION_UNKNOWN");
        break;

    case PtEvent::TERMINAL_CONNECTION_HELD:
        stateLabel->append("TERMINAL_CONNECTION_HELD");
        break;

    case PtEvent::TERMINAL_CONNECTION_IDLE:
        stateLabel->append("TERMINAL_CONNECTION_IDLE");
        break;

    case PtEvent::TERMINAL_CONNECTION_IN_USE:
        stateLabel->append("TERMINAL_CONNECTION_IN_USE");
        break;

    case PtEvent::TERMINAL_CONNECTION_TALKING:
        stateLabel->append("TERMINAL_CONNECTION_TALKING");
        break;

    case PtEvent::CALL_ACTIVE:
        stateLabel->append("CALL_ACTIVE");
        break;

    case PtEvent::CALL_INVALID:
        stateLabel->append("CALL_INVALID");
        break;

    case PtEvent::EVENT_INVALID:
        stateLabel->append("!! INVALID_STATE !!");
        break;

    case PtEvent::CALL_META_CALL_STARTING_STARTED:
        stateLabel->append("CALL_META_CALL_STARTING_STARTED");
        break;

    case PtEvent::CALL_META_CALL_STARTING_ENDED:
        stateLabel->append("CALL_META_CALL_STARTING_ENDED");
        break;

    case PtEvent::SINGLECALL_META_PROGRESS_STARTED:
        stateLabel->append("SINGLECALL_META_PROGRESS_STARTED");
        break;

    case PtEvent::SINGLECALL_META_PROGRESS_ENDED:
        stateLabel->append("SINGLECALL_META_PROGRESS_ENDED");
        break;

    case PtEvent::CALL_META_ADD_PARTY_STARTED:
        stateLabel->append("CALL_META_ADD_PARTY_STARTED");
        break;

    case PtEvent::CALL_META_ADD_PARTY_ENDED:
        stateLabel->append("CALL_META_ADD_PARTY_ENDED");
        break;

    case PtEvent::CALL_META_REMOVE_PARTY_STARTED:
        stateLabel->append("CALL_META_REMOVE_PARTY_STARTED");
        break;

    case PtEvent::CALL_META_REMOVE_PARTY_ENDED:
        stateLabel->append("CALL_META_REMOVE_PARTY_ENDED");
        break;

    case PtEvent::CALL_META_CALL_ENDING_STARTED:
        stateLabel->append("CALL_META_CALL_ENDING_STARTED");
        break;

    case PtEvent::CALL_META_CALL_ENDING_ENDED:
        stateLabel->append("CALL_META_CALL_ENDING_ENDED");
        break;

    case PtEvent::MULTICALL_META_MERGE_STARTED:
        stateLabel->append("MULTICALL_META_MERGE_STARTED");
        break;

    case PtEvent::MULTICALL_META_MERGE_ENDED:
        stateLabel->append("MULTICALL_META_MERGE_ENDED");
        break;

    case PtEvent::MULTICALL_META_TRANSFER_STARTED:
        stateLabel->append("MULTICALL_META_TRANSFER_STARTED");
        break;

    case PtEvent::MULTICALL_META_TRANSFER_ENDED:
        stateLabel->append("MULTICALL_META_TRANSFER_ENDED");
        break;

    case PtEvent::SINGLECALL_META_SNAPSHOT_STARTED:
        stateLabel->append("SINGLECALL_META_SNAPSHOT_STARTED");
        break;

    case PtEvent::SINGLECALL_META_SNAPSHOT_ENDED:
        stateLabel->append("SINGLECALL_META_SNAPSHOT_ENDED");
        break;

    default:
        stateLabel->append("STATE_UNKNOWN");
        break;

    }

}

void CpCall::setMetaEvent(int metaEventId, int metaEventType,
                          int numCalls, const char* metaEventCallIds[])
{
    if (mMetaEventId != 0 || mMetaEventType != PtEvent::META_EVENT_NONE)
        stopMetaEvent();

    mMetaEventId = metaEventId;
    mMetaEventType = metaEventType;

    if(mpMetaEventCallIds)
    {
        delete[] mpMetaEventCallIds;
        mpMetaEventCallIds = NULL;
    }

    if (numCalls > 0)
    {
        mNumMetaEventCalls = numCalls;
        mpMetaEventCallIds = new UtlString[numCalls];
        for(int i = 0; i < numCalls; i++)
        {
            if (metaEventCallIds)
                mpMetaEventCallIds[i] = metaEventCallIds[i];
            else
                mpMetaEventCallIds[i] = mCallId.data();
        }
    }
}

void CpCall::startMetaEvent(int metaEventId,
                            int metaEventType,
                            int numCalls,
                            const char* metaEventCallIds[],
                            int remoteIsCallee)
{
    setMetaEvent(metaEventId, metaEventType, numCalls, metaEventCallIds);
}

void CpCall::getMetaEvent(int& metaEventId, int& metaEventType,
                          int& numCalls, const UtlString* metaEventCallIds[]) const
{
    metaEventId = mMetaEventId;
    metaEventType = mMetaEventType;
    numCalls = mNumMetaEventCalls;
    *metaEventCallIds = mpMetaEventCallIds;
}

void CpCall::stopMetaEvent(int remoteIsCallee)
{
    // Clear the event info
    mMetaEventId = 0;
    mMetaEventType = PtEvent::META_EVENT_NONE;

    if(mpMetaEventCallIds)
    {
        delete[] mpMetaEventCallIds;
        mpMetaEventCallIds = NULL;
    }
}

void CpCall::setCallType(int callType)
{
    mCallType = callType;
}

int CpCall::getCallType() const
{
    return(mCallType);
}

void CpCall::setTargetCallId(const char* targetCallId)
{
    if(targetCallId && * targetCallId) mTargetCallId = targetCallId;
}

void CpCall::getTargetCallId(UtlString& targetCallId) const
{
    targetCallId = mTargetCallId;
}

void CpCall::setOriginalCallId(const char* originalCallId)
{
    if(originalCallId && * originalCallId) mOriginalCallId = originalCallId;
}

void CpCall::getOriginalCallId(UtlString& originalCallId) const
{
    originalCallId = mOriginalCallId;
}
/* ============================ INQUIRY =================================== */
UtlBoolean CpCall::isQueued()
{
    return(FALSE);
}

void CpCall::getLocalAddress(char* address, int maxLen)
{
   // :TODO: Not yet implemented.
   assert(FALSE);
}

void CpCall::getLocalTerminalId(char* terminal, int maxLen)
{
   // :TODO: Not yet implemented.
   assert(FALSE);
}

UtlBoolean CpCall::isCallIdSet()
{
    OsReadLock lock(mCallIdMutex);
    return(!mCallId.isNull());
}

UtlBoolean CpCall::isLocalHeld()
{
    return mLocalHeld;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

void CpCall::addHistoryEvent(const char* messageLogString)
{
    mMessageEventCount++;
    mCallHistory[mMessageEventCount % CP_CALL_HISTORY_LENGTH] =
        messageLogString;
}

void CpCall::addHistoryEvent(const int msgSubType,
                             const CpMultiStringMessage* multiStringMessage)
{
    char eventDescription[100];
    UtlString subTypeString;
    CpCallManager::getEventSubTypeString((enum CpCallManager::EventSubTypes)msgSubType,
        subTypeString);
    UtlString msgDump;
    if(multiStringMessage) multiStringMessage->toString(msgDump, ", ");
    SNPRINTF(eventDescription, sizeof(eventDescription), " (%d) \n\t", msgSubType);
    addHistoryEvent(subTypeString + eventDescription + msgDump);
}

void CpCall::postTaoListenerMessage(int responseCode,
                                    UtlString responseText,
                                    int eventId,
                                    int type,
                                    int cause,
                                    int remoteIsCallee,
                                    UtlString remoteAddress,
                                    int isRemote,
                                    UtlString targetCallId)
{
    if (type == CONNECTION_STATE /* && !PtEvent::isStateTransitionAllowed(eventId, mLocalConnectionState) */)
    {
        osPrintf("Connection state change from %d to %d is illegal\n", mLocalConnectionState, eventId);
        return;
    }

    if (type == CONNECTION_STATE)
        mLocalConnectionState = eventId;
    else if (type == TERMINAL_CONNECTION_STATE)
        mLocalTermConnectionState = tcStateFromEventId(eventId);

}

int CpCall::tcStateFromEventId(int eventId)
{
    int state;

    switch(eventId)
    {
    case PtEvent::TERMINAL_CONNECTION_CREATED:
    case PtEvent::TERMINAL_CONNECTION_IDLE:
        state = PtTerminalConnection::IDLE;
        break;

    case PtEvent::TERMINAL_CONNECTION_HELD:
        state = PtTerminalConnection::HELD;
        break;

    case PtEvent::TERMINAL_CONNECTION_RINGING:
        state = PtTerminalConnection::RINGING;
        break;

    case PtEvent::TERMINAL_CONNECTION_TALKING:
        state = PtTerminalConnection::TALKING;
        break;

    case PtEvent::TERMINAL_CONNECTION_IN_USE:
        state = PtTerminalConnection::IN_USE;
        break;

    case PtEvent::TERMINAL_CONNECTION_DROPPED:
        state = PtTerminalConnection::DROPPED;
        break;

    default:
        state = PtTerminalConnection::UNKNOWN;
        break;
    }

    return state;
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

OsStatus CpCall::addToCallTrackingList(UtlString &rCallTaskName)
{
    OsStatus retval = OS_FAILED;

    UtlString *tmpTaskName = new UtlString(rCallTaskName);
    sCallTrackingList.push((void *)tmpTaskName);
    retval = OS_SUCCESS; //push doesn't have a return value

    return retval;
}



//Removes a call task from the tracking list, then deletes it.
//
OsStatus CpCall::removeFromCallTrackingList(UtlString &rCallTaskName)
{
    OsStatus retval = OS_FAILED;


    UtlString *pStrFoundTaskName;

    //get an iterator handle for safe traversal
    int iteratorHandle = sCallTrackingList.getIteratorHandle();

    pStrFoundTaskName = (UtlString *)sCallTrackingList.next(iteratorHandle);
    while (pStrFoundTaskName)
    {
        // we found a Call task name that matched.  Lets remove it
        if (*pStrFoundTaskName == rCallTaskName)
        {
            sCallTrackingList.remove(iteratorHandle);
            delete pStrFoundTaskName;
            retval = OS_SUCCESS;
        }

        pStrFoundTaskName = (UtlString *)sCallTrackingList.next(iteratorHandle);
    }

    sCallTrackingList.releaseIteratorHandle(iteratorHandle);

    return retval;
}

//returns number of call tasks being tracked
int CpCall::getCallTrackingListCount()
{
    int numCalls = 0;

    numCalls = sCallTrackingList.getCount();

    return numCalls;
}

