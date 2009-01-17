//  
// Copyright (C) 2006-2007 SIPez LLC. 
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
#include <mp/MpMediaTask.h>
#include <mp/MpRtpOutputAudioConnection.h>
#include <mp/MpFlowGraphBase.h>
#include <mp/MprEncode.h>
#include <mp/MprToNet.h>
#include <mp/MprRtpStartSendMsg.h>
#include <mp/MpResourceMsg.h>
#include <sdp/SdpCodec.h>
#include <os/OsLock.h>
#include "os/OsIntPtrMsg.h"
#ifdef RTL_ENABLED
#   include <rtl_macro.h>
#endif

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
MpRtpOutputAudioConnection::MpRtpOutputAudioConnection(const UtlString& resourceName,
                                                       MpConnectionID myID, 
                                                       OsMsgQ* pConnectionNotificationQueue,
                                                       int samplesPerFrame, 
                                                       int samplesPerSec)
: MpRtpOutputConnection(resourceName,
                        myID, 
                        pConnectionNotificationQueue,
#ifdef INCLUDE_RTCP // [
                        NULL // TODO: pParent->getRTCPSessionPtr()
#else // INCLUDE_RTCP ][
                        NULL
#endif // INCLUDE_RTCP ]
                       )
, mpEncode(NULL)
{
   char         name[50];

   SNPRINTF(name, sizeof(name), "Encode-%d", myID);
   mpEncode    = new MprEncode(name, samplesPerFrame, samplesPerSec);

   // encoder does not get added to the flowgraph, this connection
   // gets added to do the encoding frame processing.

   //////////////////////////////////////////////////////////////////////////
   // connect Encode -> ToNet (Non synchronous resources)
   mpEncode->setMyToNet(mpToNet);

   //  This got moved to the call flowgraph when the connection is
   // added to the flowgraph.  Not sure it is still needed there either
   //pParent->synchronize("new Connection, before enable(), %dx%X\n");
   //enable();
   //pParent->synchronize("new Connection, after enable(), %dx%X\n");
}

// Destructor
MpRtpOutputAudioConnection::~MpRtpOutputAudioConnection()
{
   if (mpEncode != NULL)
      delete mpEncode;
}

/* ============================ MANIPULATORS ============================== */

UtlBoolean MpRtpOutputAudioConnection::processFrame(void)
{
    UtlBoolean result;

#ifdef RTL_ENABLED
    RTL_BLOCK((UtlString)*this);
#endif

    assert(mpEncode);
    if(mpEncode)
    {
        // call doProcessFrame to do any "real" work
        result = doProcessFrame(mpInBufs, 
                                mpOutBufs,
                                mMaxInputs, 
                                mMaxOutputs, 
                                mpEncode->mIsEnabled,
                                mpEncode->getSamplesPerFrame(), 
                                mpEncode->getSamplesPerSec());
    }

   // release the input buffer, we are done with it
   mpInBufs[0].release();
   assert(mMaxInputs == 1);

   // no outputs to release
   assert(mMaxOutputs == 0);

   return(result);
}

UtlBoolean MpRtpOutputAudioConnection::doProcessFrame(MpBufPtr inBufs[],
                                                     MpBufPtr outBufs[],
                                                     int inBufsSize,
                                                     int outBufsSize,
                                                     UtlBoolean isEnabled,
                                                     int samplesPerFrame,
                                                     int samplesPerSecond)
{
    UtlBoolean result = FALSE;
    assert(mpEncode);
    if(mpEncode)
    {
        result = mpEncode->doProcessFrame(inBufs,
                                          outBufs,
                                          inBufsSize,
                                          outBufsSize,
                                          isEnabled,
                                          samplesPerFrame,
                                          samplesPerSecond);
    }

    return(result);
}

UtlBoolean MpRtpOutputAudioConnection::handleMessage(MpResourceMsg& rMsg)
{
    UtlBoolean result = FALSE;
    unsigned char messageSubtype = rMsg.getMsgSubType();
    switch(messageSubtype)
    {
    case MpResourceMsg::MPRM_START_SEND_RTP:
        {
            MprRtpStartSendMsg* startMessage = (MprRtpStartSendMsg*) &rMsg;
            SdpCodec* audioCodec = NULL;
            SdpCodec* dtmfCodec = NULL;
            startMessage->getCodecs(audioCodec, dtmfCodec);
            OsSocket* rtpSocket = startMessage->getRtpSocket();
            OsSocket* rtcpSocket = startMessage->getRtcpSocket();

            assert(rtpSocket);
            assert(rtcpSocket);
            handleStartSendRtp(*rtpSocket,
                               *rtcpSocket,
                               audioCodec,
                               dtmfCodec);
            result = TRUE;
        }
        break;

    case MpResourceMsg::MPRM_STOP_SEND_RTP:
        handleStopSendRtp();
        result = TRUE;
        break;

    default:
        result = MpResource::handleMessage(rMsg);
        break;
    }
    return(result);
}

// Disables the output path of the connection.
// Resources on the path(s) will also be disabled by these calls.
// If the flow graph is not "started", this call takes effect
// immediately.  Otherwise, the call takes effect at the start of the
// next frame processing interval.

UtlBoolean MpRtpOutputAudioConnection::handleDisable()
{
   mpEncode->disable();
   return MpResource::handleDisable();
}

// Enables the output path of the connection.
// Resources on the path(s) will also be enabled by these calls.
// Resources may allocate needed data (e.g. output path reframe buffer)
//  during this operation.
// If the flow graph is not "started", this call takes effect
// immediately.  Otherwise, the call takes effect at the start of the
// next frame processing interval.

UtlBoolean MpRtpOutputAudioConnection::handleEnable()
{
   mpEncode->enable();
   return(MpResource::handleEnable());
}

void MpRtpOutputAudioConnection::sendConnectionNotification(MpNotificationMsgType type, intptr_t data)
{
   if (m_pConnectionNotificationQueue  && areNotificationsEnabled())
   {
      // create message and send it to connection notification queue
      OsIntPtrMsg connectionMsg(OsMsg::MP_CONNECTION_NOTF_MSG,
         (unsigned char)MP_NOTIFICATION_AUDIO,
         (int)type,
         mMyID,
         data);

      m_pConnectionNotificationQueue->send(connectionMsg, OsTime::NO_WAIT_TIME);
   }
}

// Start sending RTP and RTCP packets.
/// Queues a message to start sending RTP and RTCP packets.
OsStatus MpRtpOutputAudioConnection::startSendRtp(OsMsgQ& messageQueue,
                                                  const UtlString& resourceName,
                                                  OsSocket& rRtpSocket, 
                                                  OsSocket& rRtcpSocket,
                                                  SdpCodec* audioCodec, 
                                                  SdpCodec* dtmfCodec)
{
    OsStatus result = OS_INVALID_ARGUMENT;
    if(audioCodec || dtmfCodec)
    {
        // Create a message to contain the startRecieveRtp data
        MprRtpStartSendMsg msg(resourceName, audioCodec,
                               dtmfCodec, rRtpSocket, rRtcpSocket);

        // Send the message in the queue.
        result = messageQueue.send(msg);
    }
    return(result);
}

     /// Queues a message to stop sending RTP and RTCP packets.
OsStatus MpRtpOutputAudioConnection::stopSendRtp(OsMsgQ& messageQueue,
                                                 const UtlString& resourceName)
{
    MpResourceMsg stopSendMsg(MpResourceMsg::MPRM_STOP_SEND_RTP, 
                              resourceName);

    // Send the message in the queue.
    OsStatus result = messageQueue.send(stopSendMsg);
    return(result);
}

OsStatus MpRtpOutputAudioConnection::handleStartSendRtp(OsSocket& rRtpSocket,
                                                    OsSocket& rRtcpSocket,
                                                    SdpCodec* pPrimaryCodec,
                                                    SdpCodec* pDtmfCodec)
{
   prepareStartSendRtp(rRtpSocket, rRtcpSocket);

   OsStatus result = OS_NOT_FOUND;
   if(mpEncode)
   {
       result = OS_SUCCESS;
       mpEncode->selectCodecs(pPrimaryCodec, pDtmfCodec);
       mpEncode->enable();
       sendConnectionNotification(MP_NOTIFICATION_START_RTP_SEND, 0);
   }
   return(result);
}

// Stop sending RTP and RTCP packets.
OsStatus MpRtpOutputAudioConnection::handleStopSendRtp()
{
   OsStatus result = OS_NOT_FOUND;
   prepareStopSendRtp();

   if(mpEncode)
   {
       result = OS_SUCCESS;
       mpEncode->deselectCodecs();
       mpEncode->disable();
       sendConnectionNotification(MP_NOTIFICATION_STOP_RTP_SEND, 0);
   }
   return(result);
}

void MpRtpOutputAudioConnection::startTone(int toneId)
{
   mpEncode->startTone(toneId);
}

void MpRtpOutputAudioConnection::stopTone(void)
{
   mpEncode->stopTone();
}


/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
