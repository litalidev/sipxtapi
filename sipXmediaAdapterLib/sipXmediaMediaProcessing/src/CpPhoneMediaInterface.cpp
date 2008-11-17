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

// Author: Dan Petrie (dpetrie AT SIPez DOT com)
 
// SYSTEM INCLUDES
#include <assert.h>

// APPLICATION INCLUDES
#include <utl/UtlDListIterator.h>
#include <utl/UtlHashMap.h>
#include <os/OsDatagramSocket.h>
#include <os/OsNatDatagramSocket.h>
#include <os/OsMulticastSocket.h>
#include <os/OsProtectEventMgr.h>
#include "include/CpPhoneMediaInterface.h"
#include "mi/CpMediaInterfaceFactory.h"
#include <mp/MpMediaTask.h>
#include <mp/MpCallFlowGraph.h>
#include <mp/MpStreamPlayer.h>
#include <mp/MpStreamPlaylistPlayer.h>
#include <mp/MpStreamQueuePlayer.h>
#include <mp/dtmflib.h>

#include <sdp/SdpCodec.h>

#if defined(_VXWORKS)
#   include <socket.h>
#   include <resolvLib.h>
#   include <netinet/ip.h>
#elif defined(__pingtel_on_posix__)
#   include <netinet/in.h>
#   include <netinet/tcp.h>
#   include <sys/types.h>
#   include <sys/socket.h>
#endif

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define MINIMUM_DTMF_LENGTH 60
#define MAX_RTP_PORTS 1000

//#define TEST_PRINT

// STATIC VARIABLE INITIALIZATIONS

class CpPhoneMediaConnection : public UtlInt
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
    CpPhoneMediaConnection(int connectionId = -1)
    : UtlInt(connectionId)
    , mRtpSendHostAddress()
    , mDestinationSet(FALSE)
    , mIsMulticast(FALSE)
    , mpRtpAudioSocket(NULL)
    , mpRtcpAudioSocket(NULL)
    , mRtpAudioSendHostPort(0)
    , mRtcpAudioSendHostPort(0)
    , mRtpAudioReceivePort(0)
    , mRtcpAudioReceivePort(0)
    , mRtpAudioSending(FALSE)
    , mRtpAudioReceiving(FALSE)
    , mpAudioCodec(NULL)
    , mpCodecFactory(NULL)
    , mContactType(CONTACT_AUTO)
    , mbAlternateDestinations(FALSE)
    {
    };

    virtual ~CpPhoneMediaConnection()
    {
        if(mpRtpAudioSocket)
        {

#ifdef TEST_PRINT
            OsSysLog::add(FAC_CP, PRI_DEBUG, 
                "~CpPhoneMediaConnection deleting RTP socket: %p descriptor: %d",
                mpRtpAudioSocket, mpRtpAudioSocket->getSocketDescriptor());
#endif
            delete mpRtpAudioSocket;
            mpRtpAudioSocket = NULL;
        }

        if(mpRtcpAudioSocket)
        {
#ifdef TEST_PRINT
            OsSysLog::add(FAC_CP, PRI_DEBUG, 
                "~CpPhoneMediaConnection deleting RTCP socket: %p descriptor: %d",
                mpRtcpAudioSocket, mpRtcpAudioSocket->getSocketDescriptor());
#endif
            delete mpRtcpAudioSocket;
            mpRtcpAudioSocket = NULL;
        }

        if(mpCodecFactory)
        {
            OsSysLog::add(FAC_CP, PRI_DEBUG, 
                "~CpPhoneMediaConnection deleting SdpCodecList %p",
                mpCodecFactory);
            delete mpCodecFactory;
            mpCodecFactory = NULL;
        }

        if (mpAudioCodec)
        {
            delete mpAudioCodec;
            mpAudioCodec = NULL; 
        }              

        mConnectionProperties.destroyAll();
    }

    UtlString mRtpSendHostAddress;
    UtlBoolean mDestinationSet;
    UtlBoolean mIsMulticast;
    OsDatagramSocket* mpRtpAudioSocket;
    OsDatagramSocket* mpRtcpAudioSocket;
    int mRtpAudioSendHostPort;
    int mRtcpAudioSendHostPort;
    int mRtpAudioReceivePort;
    int mRtcpAudioReceivePort;
    UtlBoolean mRtpAudioSending;
    UtlBoolean mRtpAudioReceiving;
    SdpCodec* mpAudioCodec;
    SdpCodecList* mpCodecFactory;
    SIPX_CONTACT_TYPE mContactType ;
    UtlString mLocalAddress ;
    UtlBoolean mbAlternateDestinations ;
    UtlHashMap mConnectionProperties;
};

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
CpPhoneMediaInterface::CpPhoneMediaInterface(CpMediaInterfaceFactory* pFactoryImpl,
											            OsMsgQ* pInterfaceNotificationQueue,
                                             const SdpCodecList* pCodecList,
                                             const char* publicAddress,
                                             const char* localAddress,
                                             const char* locale,
                                             int expeditedIpTos,
                                             const char* szStunServer,
                                             int iStunPort,
                                             int iStunKeepAlivePeriodSecs,
                                             const char* szTurnServer,
                                             int iTurnPort,
                                             const char* szTurnUsername,
                                             const char* szTurnPassword,
                                             int iTurnKeepAlivePeriodSecs,
                                             UtlBoolean bEnableICE)
: CpMediaInterface(pFactoryImpl)
, m_pInterfaceNotificationQueue(pInterfaceNotificationQueue)
{
   OsSysLog::add(FAC_CP, PRI_DEBUG, "CpPhoneMediaInterface::CpPhoneMediaInterface creating a new CpMediaInterface %p",
                 this);

   mpFlowGraph = new MpCallFlowGraph(locale, pInterfaceNotificationQueue);
   OsSysLog::add(FAC_CP, PRI_DEBUG, "CpPhoneMediaInterface::CpPhoneMediaInterface creating a new MpCallFlowGraph %p",
                 mpFlowGraph);
   
   mStunServer = szStunServer ;
   mStunPort = iStunPort ;
   mStunRefreshPeriodSecs = iStunKeepAlivePeriodSecs ;
   mTurnServer = szTurnServer ;
   mTurnPort = iTurnPort ;
   mTurnRefreshPeriodSecs = iTurnKeepAlivePeriodSecs ;
   mTurnUsername = szTurnUsername ;
   mTurnPassword = szTurnPassword ;
   mEnableIce = bEnableICE ;

   if(localAddress && *localAddress)
   {
       mRtpReceiveHostAddress = localAddress;
       mLocalAddress = localAddress;
   }
   else
   {
       OsSocket::getHostIp(&mLocalAddress);
   }

   if(pCodecList && pCodecList->getCodecCount() > 0)
   {
       mSdpCodecList.addCodecs(*pCodecList);
       // Assign any unset payload types
       mSdpCodecList.bindPayloadIds();
   }
   else
   {
      if (pFactoryImpl)
      {
         pFactoryImpl->buildAllCodecList(mSdpCodecList);
      }
   }

   mExpeditedIpTos = expeditedIpTos;
}


// Destructor
CpPhoneMediaInterface::~CpPhoneMediaInterface()
{
   OsSysLog::add(FAC_CP, PRI_DEBUG, "CpPhoneMediaInterface::~CpPhoneMediaInterface deleting the CpMediaInterface %p",
                 this);

    CpPhoneMediaConnection* mediaConnection = NULL;
    while ((mediaConnection = (CpPhoneMediaConnection*) mMediaConnections.get()))
    {
        doDeleteConnection(mediaConnection);
        delete mediaConnection;
        mediaConnection = NULL;
    }

    if(mpFlowGraph)
    {
      // Free up the resources used by tone generation ASAP
      stopTone();

        // Stop the net in/out stuff before the sockets are deleted
        //mpMediaFlowGraph->stopReceiveRtp();
        //mpMediaFlowGraph->stopSendRtp();

        MpMediaTask* mediaTask = MpMediaTask::getMediaTask();

        // take focus away from the flow graph if it is focus
        if(mpFlowGraph == (MpCallFlowGraph*) mediaTask->getFocus())
        {
            mediaTask->setFocus(NULL);
        }

        OsSysLog::add(FAC_CP, PRI_DEBUG, "CpPhoneMediaInterface::~CpPhoneMediaInterface deleting the MpCallFlowGraph %p",
                      mpFlowGraph);
        delete mpFlowGraph;
        mpFlowGraph = NULL;
    }

    // Delete the properties and their values
    mInterfaceProperties.destroyAll();
}

/**
 * public interface for destroying this media interface
 */ 
void CpPhoneMediaInterface::release()
{
   delete this;
}

/* ============================ MANIPULATORS ============================== */

OsStatus CpPhoneMediaInterface::createConnection(int& connectionId,
                                                 const char* szLocalAddress,
                                                 int localPort,
                                                 void* videoWindowHandle, 
                                                 void* const pSecurityAttributes,
                                                 OsMsgQ* pConnectionNotificationQueue,
                                                 const RtpTransportOptions rtpTransportOptions)
{
   OsStatus retValue = OS_SUCCESS;
   CpPhoneMediaConnection* mediaConnection=NULL;

   connectionId = mpFlowGraph->createConnection(pConnectionNotificationQueue);
   if (connectionId == -1)
   {
      return OS_LIMIT_REACHED;
   }


   mediaConnection = new CpPhoneMediaConnection(connectionId);
   OsSysLog::add(FAC_CP, PRI_DEBUG,
                 "CpPhoneMediaInterface::createConnection "
                 "creating a new connection %d (%p)",
                 connectionId, mediaConnection);
   mMediaConnections.append(mediaConnection);

   // Set Local address
   if (szLocalAddress && strlen(szLocalAddress))
   {
      mediaConnection->mLocalAddress = szLocalAddress ;
   }
   else
   {
      mediaConnection->mLocalAddress = mLocalAddress ;
   }

   mediaConnection->mIsMulticast = OsSocket::isMcastAddr(mediaConnection->mLocalAddress);
   if (mediaConnection->mIsMulticast)
   {
      mediaConnection->mContactType = CONTACT_LOCAL;
   }

   // Create the sockets for audio stream
   retValue = createRtpSocketPair(mediaConnection->mLocalAddress, localPort,
                                  mediaConnection->mContactType,
                                  mediaConnection->mpRtpAudioSocket, mediaConnection->mpRtcpAudioSocket);
   if (retValue != OS_SUCCESS)
   {
       return retValue;
   }

   // Start the audio packet pump
   mpFlowGraph->startReceiveRtp(UtlSList(), // pass empty list
                                *mediaConnection->mpRtpAudioSocket,
                                *mediaConnection->mpRtcpAudioSocket,
                                connectionId);

   // Store audio stream settings
   mediaConnection->mRtpAudioReceivePort = mediaConnection->mpRtpAudioSocket->getLocalHostPort() ;
   mediaConnection->mRtcpAudioReceivePort = mediaConnection->mpRtcpAudioSocket->getLocalHostPort() ;

   OsSysLog::add(FAC_CP, PRI_DEBUG, 
            "CpPhoneMediaInterface::createConnection creating a new RTP socket: %p descriptor: %d",
            mediaConnection->mpRtpAudioSocket, mediaConnection->mpRtpAudioSocket->getSocketDescriptor());
   OsSysLog::add(FAC_CP, PRI_DEBUG, 
            "CpPhoneMediaInterface::createConnection creating a new RTCP socket: %p descriptor: %d",
            mediaConnection->mpRtcpAudioSocket, mediaConnection->mpRtcpAudioSocket->getSocketDescriptor());

   // Set codec factory
   UtlSList codecList;
   mSdpCodecList.getCodecs(codecList);
   mediaConnection->mpCodecFactory = new SdpCodecList(codecList);
   mediaConnection->mpCodecFactory->bindPayloadIds();
   OsSysLog::add(FAC_CP, PRI_DEBUG, 
            "CpPhoneMediaInterface::createConnection creating a new SdpCodecList %p",
            mediaConnection->mpCodecFactory);

    return retValue;
}

void CpPhoneMediaInterface::setInterfaceNotificationQueue(OsMsgQ* pInterfaceNotificationQueue)
{
	if (!m_pInterfaceNotificationQueue && mpFlowGraph)
	{
		m_pInterfaceNotificationQueue = pInterfaceNotificationQueue;
      mpFlowGraph->setInterfaceNotificationQueue(pInterfaceNotificationQueue);
	}	
}


OsStatus CpPhoneMediaInterface::getCapabilities(int connectionId,
                                                UtlString& rtpHostAddress,
                                                int& rtpAudioPort,
                                                int& rtcpAudioPort,
                                                int& rtpVideoPort,
                                                int& rtcpVideoPort,
                                                SdpCodecList& supportedCodecs,
                                                SdpSrtpParameters& srtpParams,
                                                int bandWidth,
                                                int& videoBandwidth,
                                                int& videoFramerate)
{
    OsStatus rc = OS_FAILED ;
    CpPhoneMediaConnection* pMediaConn = getMediaConnection(connectionId);
    rtpAudioPort = 0 ;
    rtcpAudioPort = 0 ;
    rtpVideoPort = 0 ;
    rtcpVideoPort = 0 ; 
    videoBandwidth = 0 ;

    if (pMediaConn)
    {
        // Audio RTP
        if (pMediaConn->mpRtpAudioSocket)
        {
            // The "rtpHostAddress" is used for the first RTP stream -- 
            // others are ignored.  They *SHOULD* be the same as the first.  
            // Possible exceptions: STUN worked for the first, but not the
            // others.  Not sure how to handle/recover from that case.
            if (pMediaConn->mContactType == CONTACT_RELAY)
            {
                assert(!pMediaConn->mIsMulticast);
                if (!((OsNatDatagramSocket*)pMediaConn->mpRtpAudioSocket)->
                                            getRelayIp(&rtpHostAddress, &rtpAudioPort))
                {
                    rtpAudioPort = pMediaConn->mRtpAudioReceivePort ;
                    rtpHostAddress = mRtpReceiveHostAddress ;
                }

            }
            else if (pMediaConn->mContactType == CONTACT_AUTO || pMediaConn->mContactType == CONTACT_NAT_MAPPED)
            {
                assert(!pMediaConn->mIsMulticast);
                if (!pMediaConn->mpRtpAudioSocket->getMappedIp(&rtpHostAddress, &rtpAudioPort))
                {
                    rtpAudioPort = pMediaConn->mRtpAudioReceivePort ;
                    rtpHostAddress = mRtpReceiveHostAddress ;
                }
            }
            else if (pMediaConn->mContactType == CONTACT_LOCAL)
            {
                 rtpHostAddress = pMediaConn->mpRtpAudioSocket->getLocalIp();
                 rtpAudioPort = pMediaConn->mpRtpAudioSocket->getLocalHostPort();
                 if (rtpAudioPort <= 0)
                 {
                     rtpAudioPort = pMediaConn->mRtpAudioReceivePort ;
                     rtpHostAddress = mRtpReceiveHostAddress ;
                 }
            }
            else
            {
              assert(0);
            }               
        }

        // Audio RTCP
        if (pMediaConn->mpRtcpAudioSocket)
        {
            if (pMediaConn->mContactType == CONTACT_RELAY)
            {
                UtlString tempHostAddress;
                assert(!pMediaConn->mIsMulticast);
                if (!((OsNatDatagramSocket*)pMediaConn->mpRtcpAudioSocket)->
                                            getRelayIp(&tempHostAddress, &rtcpAudioPort))
                {
                    rtcpAudioPort = pMediaConn->mRtcpAudioReceivePort ;
                }
                else
                {
                    // External address should match that of Audio RTP
                    assert(tempHostAddress.compareTo(rtpHostAddress) == 0) ;
                }
            }
            else if (pMediaConn->mContactType == CONTACT_AUTO || pMediaConn->mContactType == CONTACT_NAT_MAPPED)
            {
                UtlString tempHostAddress;
                assert(!pMediaConn->mIsMulticast);
                if (!pMediaConn->mpRtcpAudioSocket->getMappedIp(&tempHostAddress, &rtcpAudioPort))
                {
                    rtcpAudioPort = pMediaConn->mRtcpAudioReceivePort ;
                }
                else
                {
                    // External address should match that of Audio RTP
                    assert(tempHostAddress.compareTo(rtpHostAddress) == 0) ;
                }
            }
            else if (pMediaConn->mContactType == CONTACT_LOCAL)
            {
                rtcpAudioPort = pMediaConn->mpRtcpAudioSocket->getLocalHostPort();
                if (rtcpAudioPort <= 0)
                {
                    rtcpAudioPort = pMediaConn->mRtcpAudioReceivePort ;
                }
            }                
            else
            {
                assert(0);
            }
        }

        // Codecs
        if (bandWidth != AUDIO_MICODEC_BW_DEFAULT)
        {
            setAudioCodecBandwidth(connectionId, bandWidth);
        }
        supportedCodecs.clearCodecs();
        UtlSList codecList;
        pMediaConn->mpCodecFactory->getCodecs(codecList);
        supportedCodecs.addCodecs(codecList);
        supportedCodecs.bindPayloadIds();   

        // Setup SRTP parameters here
        memset((void*)&srtpParams, 0, sizeof(SdpSrtpParameters));

        rc = OS_SUCCESS ;
    }

    return rc ;
}


OsStatus CpPhoneMediaInterface::getCapabilitiesEx(int connectionId, 
                                                  int nMaxAddresses,
                                                  UtlString rtpHostAddresses[], 
                                                  int rtpAudioPorts[],
                                                  int rtcpAudioPorts[],
                                                  int rtpVideoPorts[],
                                                  int rtcpVideoPorts[],
                                                  RTP_TRANSPORT transportTypes[],
                                                  int& nActualAddresses,
                                                  SdpCodecList& supportedCodecs,
                                                  SdpSrtpParameters& srtpParameters,
                                                  int bandWidth,
                                                  int& videoBandwidth,
                                                  int& videoFramerate)
{   
    OsStatus rc = OS_FAILED ;
    CpPhoneMediaConnection* pMediaConn = getMediaConnection(connectionId);
    nActualAddresses = 0 ;

    // Clear input rtpAudioPorts, rtcpAudioPorts, rtpVideoPorts, rtcpVideoPorts
    // and transportTypes arrays. Do not suppose them to be cleaned by caller.
    memset(rtpAudioPorts, 0, nMaxAddresses*sizeof(int));
    memset(rtcpAudioPorts, 0, nMaxAddresses*sizeof(int));
    memset(rtpVideoPorts, 0, nMaxAddresses*sizeof(int));
    memset(rtcpVideoPorts, 0, nMaxAddresses*sizeof(int));
    for (int i = 0; i < nMaxAddresses; i++)
    {
        transportTypes[i] = RTP_TRANSPORT_UNKNOWN;
    }

    if (pMediaConn)
    {        
        switch (pMediaConn->mContactType)
        {
            case CONTACT_LOCAL:
                addLocalContacts(connectionId, nMaxAddresses, rtpHostAddresses,
                        rtpAudioPorts, rtcpAudioPorts, rtpVideoPorts, 
                        rtcpVideoPorts, nActualAddresses) ;
                addNatedContacts(connectionId, nMaxAddresses, rtpHostAddresses,
                        rtpAudioPorts, rtcpAudioPorts, rtpVideoPorts, 
                        rtcpVideoPorts, nActualAddresses) ;
                addRelayContacts(connectionId, nMaxAddresses, rtpHostAddresses,
                        rtpAudioPorts, rtcpAudioPorts, rtpVideoPorts, 
                        rtcpVideoPorts, nActualAddresses) ;
                break ;
            case CONTACT_RELAY:
                addRelayContacts(connectionId, nMaxAddresses, rtpHostAddresses,
                        rtpAudioPorts, rtcpAudioPorts, rtpVideoPorts, 
                        rtcpVideoPorts, nActualAddresses) ;
                addLocalContacts(connectionId, nMaxAddresses, rtpHostAddresses,
                        rtpAudioPorts, rtcpAudioPorts, rtpVideoPorts, 
                        rtcpVideoPorts, nActualAddresses) ;
                addNatedContacts(connectionId, nMaxAddresses, rtpHostAddresses,
                        rtpAudioPorts, rtcpAudioPorts, rtpVideoPorts, 
                        rtcpVideoPorts, nActualAddresses) ;
                break ;
            default:
                addNatedContacts(connectionId, nMaxAddresses, rtpHostAddresses,
                        rtpAudioPorts, rtcpAudioPorts, rtpVideoPorts, 
                        rtcpVideoPorts, nActualAddresses) ;
                addLocalContacts(connectionId, nMaxAddresses, rtpHostAddresses,
                        rtpAudioPorts, rtcpAudioPorts, rtpVideoPorts, 
                        rtcpVideoPorts, nActualAddresses) ;
                addRelayContacts(connectionId, nMaxAddresses, rtpHostAddresses,
                        rtpAudioPorts, rtcpAudioPorts, rtpVideoPorts, 
                        rtcpVideoPorts, nActualAddresses) ;
                break ;


        }

        // Codecs
        if (bandWidth != AUDIO_MICODEC_BW_DEFAULT)
        {
            setAudioCodecBandwidth(connectionId, bandWidth);
        }
        supportedCodecs.clearCodecs();
        UtlSList codecList;
        pMediaConn->mpCodecFactory->getCodecs(codecList);
        supportedCodecs.addCodecs(codecList);
        supportedCodecs.bindPayloadIds();

        memset((void*)&srtpParameters, 0, sizeof(SdpSrtpParameters));
        if (nActualAddresses > 0)
        {
            rc = OS_SUCCESS ;
        }

        // TODO: Need to get real transport types
        for (int i=0; i<nActualAddresses; i++)
        {
            transportTypes[i] = RTP_TRANSPORT_UDP ;
        }
    }

    return rc ;
}

OsStatus
CpPhoneMediaInterface::setMediaNotificationsEnabled(bool enabled, 
                                                    const UtlString& resourceName)
{
   return mpFlowGraph ? 
      mpFlowGraph->setNotificationsEnabled(enabled, resourceName) :
      OS_FAILED;
}

CpPhoneMediaConnection* CpPhoneMediaInterface::getMediaConnection(int connectionId)
{
   UtlInt matchConnectionId(connectionId);
   return((CpPhoneMediaConnection*) mMediaConnections.find(&matchConnectionId));
}

OsStatus CpPhoneMediaInterface::setConnectionDestination(int connectionId,
                                                         const char* remoteRtpHostAddress,
                                                         int remoteAudioRtpPort,
                                                         int remoteAudioRtcpPort,
                                                         int remoteVideoRtpPort,
                                                         int remoteVideoRtcpPort)
{
    OsStatus returnCode = OS_NOT_FOUND;
    CpPhoneMediaConnection* pMediaConnection = getMediaConnection(connectionId);

    if(pMediaConnection && remoteRtpHostAddress && *remoteRtpHostAddress)
    {
        /*
         * Common Setup
         */
        pMediaConnection->mDestinationSet = TRUE;
        pMediaConnection->mRtpSendHostAddress = remoteRtpHostAddress ;

        /*
         * Audio Setup
         */
        pMediaConnection->mRtpAudioSendHostPort = remoteAudioRtpPort;
        pMediaConnection->mRtcpAudioSendHostPort = remoteAudioRtcpPort;

        if(pMediaConnection->mpRtpAudioSocket)
        {
            if (!pMediaConnection->mIsMulticast)
            {
                OsNatDatagramSocket *pSocket = (OsNatDatagramSocket*)pMediaConnection->mpRtpAudioSocket;
                pSocket->readyDestination(remoteRtpHostAddress, remoteAudioRtpPort) ;
                pSocket->applyDestinationAddress(remoteRtpHostAddress, remoteAudioRtpPort, !mStunServer.isNull(), mStunRefreshPeriodSecs, !mTurnServer.isNull()) ;
            }
            else
            {
                pMediaConnection->mpRtpAudioSocket->doConnect(remoteAudioRtpPort,
                                                              remoteRtpHostAddress,
                                                              TRUE);
            }
        }

        if(pMediaConnection->mpRtcpAudioSocket && (remoteAudioRtcpPort > 0))
        {
            if (!pMediaConnection->mIsMulticast)
            {
                OsNatDatagramSocket *pSocket = (OsNatDatagramSocket*)pMediaConnection->mpRtcpAudioSocket;
                pSocket->readyDestination(remoteRtpHostAddress, remoteAudioRtcpPort) ;
                pSocket->applyDestinationAddress(remoteRtpHostAddress, remoteAudioRtcpPort, !mStunServer.isNull(), mStunRefreshPeriodSecs, !mTurnServer.isNull()) ;
            }
            else
            {
                pMediaConnection->mpRtcpAudioSocket->doConnect(remoteAudioRtpPort,
                                                               remoteRtpHostAddress,
                                                               TRUE);
            }
        }
        else
        {
            pMediaConnection->mRtcpAudioSendHostPort = 0 ;
        }

        /*
         * Video Setup
         */
#ifdef VIDEO
        if (pMediaConnection->mpRtpVideoSocket)
        {
            pMediaConnection->mRtpVideoSendHostPort = remoteVideoRtpPort ;                   
            if (!pMediaConnection->mIsMulticast)
            {
                OsNatDatagramSocket *pRtpSocket = (OsNatDatagramSocket*)pMediaConnection->mpRtpVideoSocket;
                pRtpSocket->readyDestination(remoteRtpHostAddress, remoteVideoRtpPort) ;
                pRtpSocket->applyDestinationAddress(remoteRtpHostAddress, remoteVideoRtpPort, !mStunServer.isNull(), mStunRefreshPeriodSecs, !mTurnServer.isNull()) ;
            }
            else
            {
                pMediaConnection->mpRtcpAudioSocket->doConnect(remoteAudioRtpPort,
                                                               remoteRtpHostAddress,
                                                               TRUE);
            }

            if(pMediaConnection->mpRtcpVideoSocket && (remoteVideoRtcpPort > 0))
            {
                pMediaConnection->mRtcpVideoSendHostPort = remoteVideoRtcpPort ;               
                if (!pMediaConnection->mIsMulticast)
                {
                   OsNatDatagramSocket *pRctpSocket = (OsNatDatagramSocket*)pMediaConnection->mpRtcpVideoSocket;
                   pRctpSocket->readyDestination(remoteRtpHostAddress, remoteVideoRtcpPort) ;
                   pRctpSocket->applyDestinationAddress(remoteRtpHostAddress, remoteVideoRtcpPort, !mStunServer.isNull(), mStunRefreshPeriodSecs, !mTurnServer.isNull()) ;
                }
                else
                {
                   pMediaConnection->mpRtcpAudioSocket->doConnect(remoteAudioRtpPort,
                                                                  remoteRtpHostAddress,
                                                                  TRUE);
                }
            }
            else
            {
                pMediaConnection->mRtcpVideoSendHostPort = 0 ;
            }
        }
        else
        {
            pMediaConnection->mRtpVideoSendHostPort = 0 ;
            pMediaConnection->mRtcpVideoSendHostPort = 0 ;
        }        
#endif

        returnCode = OS_SUCCESS;
    }

   return(returnCode);
}

OsStatus CpPhoneMediaInterface::addAudioRtpConnectionDestination(int         connectionId,
                                                                 int         iPriority,
                                                                 const char* candidateIp, 
                                                                 int         candidatePort) 
{
    OsStatus returnCode = OS_NOT_FOUND;

    CpPhoneMediaConnection* mediaConnection = getMediaConnection(connectionId);
    if (mediaConnection) 
    {
        // This is not applicable to multicast sockets
        assert(!mediaConnection->mIsMulticast);
        if (mediaConnection->mIsMulticast)
        {
            return OS_FAILED;
        }

        if (    (candidateIp != NULL) && 
                (strlen(candidateIp) > 0) && 
                (strcmp(candidateIp, "0.0.0.0") != 0) &&
                portIsValid(candidatePort) && 
                (mediaConnection->mpRtpAudioSocket != NULL))
        {
            OsNatDatagramSocket *pSocket = (OsNatDatagramSocket*)mediaConnection->mpRtpAudioSocket;
            mediaConnection->mbAlternateDestinations = TRUE;
            pSocket->addAlternateDestination(candidateIp, candidatePort, iPriority);
            pSocket->readyDestination(candidateIp, candidatePort);

            returnCode = OS_SUCCESS;
        }
        else
        {
            returnCode = OS_FAILED ;
        }
    }

    return returnCode ;
}

OsStatus CpPhoneMediaInterface::addAudioRtcpConnectionDestination(int         connectionId,
                                                                  int         iPriority,
                                                                  const char* candidateIp, 
                                                                  int         candidatePort) 
{
    OsStatus returnCode = OS_NOT_FOUND;

    CpPhoneMediaConnection* mediaConnection = getMediaConnection(connectionId);
    if (mediaConnection) 
    {        
        // This is not applicable to multicast sockets
        assert(!mediaConnection->mIsMulticast);
        if (mediaConnection->mIsMulticast)
        {
            return OS_FAILED;
        }

        if (    (candidateIp != NULL) && 
                (strlen(candidateIp) > 0) && 
                (strcmp(candidateIp, "0.0.0.0") != 0) &&
                portIsValid(candidatePort) && 
                (mediaConnection->mpRtcpAudioSocket != NULL))
        {
            OsNatDatagramSocket *pSocket = (OsNatDatagramSocket*)mediaConnection->mpRtcpAudioSocket;
            mediaConnection->mbAlternateDestinations = TRUE;
            pSocket->addAlternateDestination(candidateIp, candidatePort, iPriority);
            pSocket->readyDestination(candidateIp, candidatePort);

            returnCode = OS_SUCCESS;
        }
        else
        {
            returnCode = OS_FAILED ;
        }
    }

    return returnCode ;
}

OsStatus CpPhoneMediaInterface::addVideoRtpConnectionDestination(int         connectionId,
                                                                 int         iPriority,
                                                                 const char* candidateIp, 
                                                                 int         candidatePort) 
{
    OsStatus returnCode = OS_NOT_FOUND;
#ifdef VIDEO
    CpPhoneMediaConnection* mediaConnection = getMediaConnection(connectionId);
    if (mediaConnection) 
    {        
        // This is not applicable to multicast sockets
        assert(!mediaConnection->mIsMulticast);
        if (mediaConnection->mIsMulticast)
        {
            return OS_FAILED;
        }

        if (    (candidateIp != NULL) && 
                (strlen(candidateIp) > 0) && 
                (strcmp(candidateIp, "0.0.0.0") != 0) &&
                portIsValid(candidatePort) && 
                (mediaConnection->mpRtpVideoSocket != NULL))
        {
            OsNatDatagramSocket *pSocket = (OsNatDatagramSocket*)mediaConnection->mpRtpVideoSocket;
            mediaConnection->mbAlternateDestinations = TRUE;
            pSocket->addAlternateDestination(candidateIp, candidatePort, iPriority);
            pSocket->readyDestination(candidateIp, candidatePort);

            returnCode = OS_SUCCESS;
        }
        else
        {
            returnCode = OS_FAILED ;
        }
    }
#endif
    return returnCode ;    
}

OsStatus CpPhoneMediaInterface::addVideoRtcpConnectionDestination(int         connectionId,
                                                                  int         iPriority,
                                                                  const char* candidateIp, 
                                                                  int         candidatePort) 
{
    OsStatus returnCode = OS_NOT_FOUND;
#ifdef VIDEO
    CpPhoneMediaConnection* mediaConnection = getMediaConnection(connectionId);
    if (mediaConnection) 
    {        
        // This is not applicable to multicast sockets
        assert(!mediaConnection->mIsMulticast);
        if (mediaConnection->mIsMulticast)
        {
            return OS_FAILED;
        }

        if (    (candidateIp != NULL) && 
                (strlen(candidateIp) > 0) && 
                (strcmp(candidateIp, "0.0.0.0") != 0) &&
                portIsValid(candidatePort) && 
                (mediaConnection->mpRtcpVideoSocket != NULL))
        {
            OsNatDatagramSocket *pSocket = (OsNatDatagramSocket*)mediaConnection->mpRtcpVideoSocket;
            mediaConnection->mbAlternateDestinations = TRUE;
            pSocket->addAlternateDestination(candidateIp, candidatePort, iPriority);
            pSocket->readyDestination(candidateIp, candidatePort);

            returnCode = OS_SUCCESS;
        }
        else
        {
            returnCode = OS_FAILED ;
        }
    }
#endif
    return returnCode ;    
}


OsStatus CpPhoneMediaInterface::startRtpSend(int connectionId,
                                             const SdpCodecList& sdpCodecList)
{
   // need to set default payload types in get capabilities
   SdpCodec* audioCodec = NULL;
   SdpCodec* dtmfCodec = NULL;
   OsStatus returnCode = OS_NOT_FOUND;
   CpPhoneMediaConnection* mediaConnection = getMediaConnection(connectionId);

   if (mediaConnection == NULL)
      return returnCode;
   // find DTMF & primary audio codec
   UtlSList utlCodecList;
   sdpCodecList.getCodecs(utlCodecList);
   SdpCodec* pCodec = NULL;
   UtlSListIterator itor(utlCodecList);
   while (itor())
   {
      pCodec = dynamic_cast<SdpCodec*>(itor.item());
      if (pCodec)
      {
         UtlString codecMediaType;
         pCodec->getMediaType(codecMediaType);
         if (pCodec->getValue() == SdpCodec::SDP_CODEC_TONES)
         {
            dtmfCodec = pCodec;
         }
         else if (codecMediaType.compareTo(MIME_TYPE_AUDIO, UtlString::ignoreCase) == 0 && audioCodec == NULL)
         {
            audioCodec = pCodec;
         }
      }
   }

   // If we haven't set a destination and we have set alternate destinations
   if (!mediaConnection->mDestinationSet && mediaConnection->mbAlternateDestinations)
   {
      applyAlternateDestinations(connectionId) ;
   }

   // Make sure we use the same payload types as the remote
   // side.  Its the friendly thing to do.
   if (mediaConnection->mpCodecFactory)
   {
      mediaConnection->mpCodecFactory->copyPayloadIds(utlCodecList);
   }

   if (mpFlowGraph)
   {
       // Store the primary codec for cost calculations later
       if (mediaConnection->mpAudioCodec != NULL)
       {
           delete mediaConnection->mpAudioCodec ;
           mediaConnection->mpAudioCodec = NULL ;
       }
       if (audioCodec != NULL)
       {
           mediaConnection->mpAudioCodec = new SdpCodec();
           *mediaConnection->mpAudioCodec = *audioCodec ;
       }

       // Make sure we use the same payload types as the remote
       // side.  Its the friendly thing to do.
       if (mediaConnection->mpCodecFactory)
       {
           mediaConnection->mpCodecFactory->copyPayloadIds(utlCodecList);
       }

       if (mediaConnection->mRtpAudioSending)
       {
           mpFlowGraph->stopSendRtp(connectionId);
       }

      if (!mediaConnection->mRtpSendHostAddress.isNull() && mediaConnection->mRtpSendHostAddress.compareTo("0.0.0.0"))
      {
         // This is the new interface for parallel codecs
         mpFlowGraph->startSendRtp(*(mediaConnection->mpRtpAudioSocket),
                                   *(mediaConnection->mpRtcpAudioSocket),
                                   connectionId,
                                   audioCodec,
                                   dtmfCodec);

         mediaConnection->mRtpAudioSending = TRUE;
         returnCode = OS_SUCCESS;
      }
   }
   return returnCode;
}


OsStatus CpPhoneMediaInterface::startRtpReceive(int connectionId,
                                                const SdpCodecList& sdpCodecList)
{
   OsStatus returnCode = OS_NOT_FOUND;

   CpPhoneMediaConnection* mediaConnection = getMediaConnection(connectionId);

   if (mediaConnection == NULL)
      return OS_NOT_FOUND;

   UtlSList utlCodecList;
   sdpCodecList.getCodecs(utlCodecList);

   // Make sure we use the same payload types as the remote
   // side.  It's the friendly thing to do.
   if (mediaConnection->mpCodecFactory)
   {
         mediaConnection->mpCodecFactory->copyPayloadIds(utlCodecList);
   }

   if (mpFlowGraph)
   {
      if(mediaConnection->mRtpAudioReceiving)
      {
         // This is not supposed to be necessary and may be
         // causing an audible glitch when codecs are changed
         mpFlowGraph->stopReceiveRtp(connectionId);
      }

      mpFlowGraph->startReceiveRtp(sdpCodecList,
           *(mediaConnection->mpRtpAudioSocket), *(mediaConnection->mpRtcpAudioSocket),
           connectionId);
      mediaConnection->mRtpAudioReceiving = TRUE;

      returnCode = OS_SUCCESS;
   }
   return returnCode;
}

OsStatus CpPhoneMediaInterface::stopRtpSend(int connectionId)
{
   OsStatus returnCode = OS_NOT_FOUND;
   CpPhoneMediaConnection* mediaConnection =
       getMediaConnection(connectionId);

   if (mpFlowGraph && mediaConnection &&
       mediaConnection->mRtpAudioSending)
   {
      mpFlowGraph->stopSendRtp(connectionId);
      mediaConnection->mRtpAudioSending = FALSE;
      returnCode = OS_SUCCESS;
   }
   return(returnCode);
}

OsStatus CpPhoneMediaInterface::stopRtpReceive(int connectionId)
{
   OsStatus returnCode = OS_NOT_FOUND;
   CpPhoneMediaConnection* mediaConnection =
       getMediaConnection(connectionId);

   if (mpFlowGraph && mediaConnection &&
       mediaConnection->mRtpAudioReceiving)
   {
      mpFlowGraph->stopReceiveRtp(connectionId);
      mediaConnection->mRtpAudioReceiving = FALSE;
      returnCode = OS_SUCCESS;
   }
   return returnCode;
}

OsStatus CpPhoneMediaInterface::deleteConnection(int connectionId)
{
   OsStatus returnCode = OS_NOT_FOUND;
   CpPhoneMediaConnection* mediaConnection =
       getMediaConnection(connectionId);

   UtlInt matchConnectionId(connectionId);
   mMediaConnections.remove(&matchConnectionId) ;

   returnCode = doDeleteConnection(mediaConnection);

   delete mediaConnection ;

   return(returnCode);
}

OsStatus CpPhoneMediaInterface::doDeleteConnection(CpPhoneMediaConnection* mediaConnection)
{
   OsStatus returnCode = OS_NOT_FOUND;

   if(mediaConnection == NULL)
   {
      OsSysLog::add(FAC_CP, PRI_DEBUG, 
                  "CpPhoneMediaInterface::doDeleteConnection mediaConnection is NULL!");
      return OS_NOT_FOUND;
   }

   OsSysLog::add(FAC_CP, PRI_DEBUG, "CpPhoneMediaInterface::deleteConnection deleting the connection %p",
      mediaConnection);

   returnCode = OS_SUCCESS;
   mediaConnection->mDestinationSet = FALSE;

   returnCode = stopRtpSend(mediaConnection->getValue());
   returnCode = stopRtpReceive(mediaConnection->getValue());

   if(mediaConnection->getValue() >= 0)
   {
      mpFlowGraph->deleteConnection(mediaConnection->getValue());
      mediaConnection->setValue(-1);
      mpFlowGraph->synchronize();
   }

   mpFactoryImpl->releaseRtpPort(mediaConnection->mRtpAudioReceivePort) ;

   if(mediaConnection->mpRtpAudioSocket)
   {
      delete mediaConnection->mpRtpAudioSocket;
      mediaConnection->mpRtpAudioSocket = NULL;
   }
   if(mediaConnection->mpRtcpAudioSocket)
   {
      delete mediaConnection->mpRtcpAudioSocket;
      mediaConnection->mpRtcpAudioSocket = NULL;
   }

   return(returnCode);
}


OsStatus CpPhoneMediaInterface::playAudio(const char* url,
                                          UtlBoolean repeat,
                                          UtlBoolean local,
                                          UtlBoolean remote,
                                          UtlBoolean mixWithMic,
                                          int downScaling,
                                          void* pCookie)
{
    OsStatus returnCode = OS_NOT_FOUND;
    UtlString urlString;
    if(url) urlString.append(url);
    size_t fileIndex = urlString.index("file://");
    if(fileIndex == 0) urlString.remove(0, 6);

    if(mpFlowGraph && !urlString.isNull())
    {
         int toneOptions=0;

         if (local)
         {
            toneOptions |= MpCallFlowGraph::TONE_TO_SPKR;
         }                  
         
         if(remote)
         {
            toneOptions |= MpCallFlowGraph::TONE_TO_NET;
         }

        // Start playing the audio file
        returnCode = mpFlowGraph->playFile(urlString.data(), repeat, toneOptions, pCookie);
    }

    if(returnCode != OS_SUCCESS)
    {
        osPrintf("Cannot play audio file: %s\n", urlString.data());
    }

    return(returnCode);
}

OsStatus CpPhoneMediaInterface::playBuffer(void* buf,
                                           size_t bufSize,
                                           int type, 
                                           UtlBoolean repeat,
                                           UtlBoolean local,
                                           UtlBoolean remote,
                                           UtlBoolean mixWithMic,
                                           int downScaling,
                                           void* pCookie)
{
    OsStatus returnCode = OS_NOT_FOUND;
    if(mpFlowGraph && buf)
    {
         int toneOptions=0;

         if (local)
         {
            toneOptions |= MpCallFlowGraph::TONE_TO_SPKR;
         }                  
         
         if(remote)
         {
            toneOptions |= MpCallFlowGraph::TONE_TO_NET;
         }

        // Start playing the audio file
        returnCode = mpFlowGraph->playBuffer(buf, bufSize, type, repeat, toneOptions, pCookie);
    }

    if(returnCode != OS_SUCCESS)
    {
        osPrintf("Cannot play audio buffer: %10p\n", buf);
    }

    return(returnCode);
}


OsStatus CpPhoneMediaInterface::stopAudio()
{
    OsStatus returnCode = OS_NOT_FOUND;
    if(mpFlowGraph)
    {
        mpFlowGraph->stopFile();
        returnCode = OS_SUCCESS;
    }
    return(returnCode);
}

OsStatus CpPhoneMediaInterface::pausePlayback()
{
   OsStatus returnCode = OS_FAILED;

   if(mpFlowGraph)
   {
      returnCode = mpFlowGraph->pausePlayback();
   }
   return(returnCode);
}

OsStatus CpPhoneMediaInterface::resumePlayback()
{
   OsStatus returnCode = OS_FAILED;

   if(mpFlowGraph)
   {
      returnCode = mpFlowGraph->resumePlayback();
   }
   return(returnCode);
}

OsStatus CpPhoneMediaInterface::playChannelAudio(int connectionId,
                                                 const char* url,
                                                 UtlBoolean repeat,
                                                 UtlBoolean local,
                                                 UtlBoolean remote,
                                                 UtlBoolean mixWithMic,
                                                 int downScaling,
                                                 void* pCookie) 
{
    // TODO:: This API is designed to record the audio from a single channel.  
    // If the connectionId is -1, record all.

    return playAudio(url, repeat, local, remote, mixWithMic, downScaling, pCookie) ;
}


OsStatus CpPhoneMediaInterface::stopChannelAudio(int connectionId) 
{
    // TODO:: This API is designed to record the audio from a single channel.  
    // If the connectionId is -1, record all.

    return stopAudio() ;
}


OsStatus CpPhoneMediaInterface::recordChannelAudio(int connectionId,
                                                   const char* szFile) 
{
    // TODO:: This API is designed to record the audio from a single channel.  
    // If the connectionId is -1, record all.

    double duration = 0 ; // this means it will record for very long time
    int dtmf = 0 ;

    /* use new call recorder
       from now on, call recorder records both mic, speaker and local dtmf      
       we don't want raw pcm, but wav pcm, raw pcm should be passed to a callback
       meant for recording, for example for conversion to mp3 or other format */
    return mpFlowGraph->record(0, -1, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, szFile, 0, 0, NULL, MprRecorder::WAV_PCM_16) ;
}

OsStatus CpPhoneMediaInterface::stopRecordChannelAudio(int connectionId) 
{
    // TODO:: This API is designed to record the audio from a single channel.  
    // If the connectionId is -1, record all.


    return stopRecording() ;
}


OsStatus CpPhoneMediaInterface::startTone(int toneId,
                                          UtlBoolean local,
                                          UtlBoolean remote)
{
   OsStatus returnCode = OS_SUCCESS;
   int toneDestination = 0 ;

   if(mpFlowGraph)
   {
      if (local)
      {
         toneDestination |= MpCallFlowGraph::TONE_TO_SPKR;
      }                  
      
      if(remote)
      {
         toneDestination |= MpCallFlowGraph::TONE_TO_NET;
      }
     
      mpFlowGraph->startTone(toneId, toneDestination);
   } 

   return(returnCode);
}

OsStatus CpPhoneMediaInterface::stopTone()
{
   OsStatus returnCode = OS_SUCCESS;
   if(mpFlowGraph)
   {
      mpFlowGraph->stopTone();
   }

   return(returnCode);
}

OsStatus CpPhoneMediaInterface::startChannelTone(int connectionId, int toneId, UtlBoolean local, UtlBoolean remote) 
{
    return startTone(toneId, local, remote) ;
}

OsStatus CpPhoneMediaInterface::stopChannelTone(int connectionId)
{
    return stopTone() ;
}

OsStatus CpPhoneMediaInterface::muteInput(int connectionId, UtlBoolean bMute)
{
   OsStatus returnCode = OS_FAILED;
   if(mpFlowGraph)
   {
	   if(bMute)
	   {
	      return mpFlowGraph->muteInput(connectionId);
	   }
	   else
	   {
		  return mpFlowGraph->unmuteInput(connectionId);
	   }
   }

   return(returnCode);
}

OsStatus CpPhoneMediaInterface::giveFocus()
{
    if(mpFlowGraph)
    {
        // There should probably be a lock here
        // Set the flow graph to have the focus
        MpMediaTask* mediaTask = MpMediaTask::getMediaTask();
        mediaTask->setFocus(mpFlowGraph);
        // osPrintf("Setting focus for flow graph\n");
   }

   return OS_SUCCESS ;
}

OsStatus CpPhoneMediaInterface::defocus()
{
    if(mpFlowGraph)
    {
        MpMediaTask* mediaTask = MpMediaTask::getMediaTask();

        // There should probably be a lock here
        // take focus away from the flow graph if it is focus
        if(mpFlowGraph == (MpCallFlowGraph*) mediaTask->getFocus())
        {
            mediaTask->setFocus(NULL);
            // osPrintf("Setting NULL focus for flow graph\n");
        }
    }
    return OS_SUCCESS ;
}

OsStatus CpPhoneMediaInterface::recordAudio(const char* szFile)
{
   if (mpFlowGraph)
   {
      /* use new call recorder
      from now on, call recorder records both mic, speaker and local dtmf      
      we don't want raw pcm, but wav pcm, raw pcm should be passed to a callback
      meant for recording, for example for conversion to mp3 or other format */
      return mpFlowGraph->record(0, -1, NULL, NULL, NULL, NULL, NULL, NULL,
                                 NULL, NULL, szFile, 0, 0, NULL, MprRecorder::WAV_PCM_16);
   }

   return OS_FAILED;
}

OsStatus CpPhoneMediaInterface::stopRecording()
{
   OsStatus ret = OS_UNSPECIFIED;
   if (mpFlowGraph)
   {
#ifdef TEST_PRINT
     osPrintf("CpPhoneMediaInterface::stopRecording() : calling flowgraph::stoprecorders\n");
     OsSysLog::add(FAC_CP, PRI_DEBUG, "CpPhoneMediaInterface::stopRecording() : calling flowgraph::stoprecorders");
#endif
     mpFlowGraph->closeRecorders();
     ret = OS_SUCCESS;
   }
   
   return ret;
}

OsStatus CpPhoneMediaInterface::recordMic(UtlString* pAudioBuffer)
{
   OsStatus stat = OS_UNSPECIFIED;
   if(mpFlowGraph != NULL)
   {
      stat = mpFlowGraph->recordMic(pAudioBuffer);
   }
   return stat;
}

OsStatus CpPhoneMediaInterface::recordMic(int ms,
                                          int silenceLength,
                                          const char* fileName)
{
    OsStatus ret = OS_UNSPECIFIED;
    if (mpFlowGraph && fileName)
    {
        ret = mpFlowGraph->recordMic(ms, silenceLength, fileName);
    }
    return ret ;
}


OsStatus CpPhoneMediaInterface::ezRecord(int ms, 
                                         int silenceLength, 
                                         const char* fileName, 
                                         double& duration,
                                         int& dtmfterm,
                                         OsProtectedEvent* ev)
{
   OsStatus ret = OS_UNSPECIFIED;
   if (mpFlowGraph && fileName)
   {
     if (!ev) // default behavior
        ret = mpFlowGraph->ezRecord(ms, 
                                 silenceLength, 
                                 fileName, 
                                 duration, 
                                 dtmfterm, 
                                 MprRecorder::WAV_PCM_16);
     else
        ret = mpFlowGraph->mediaRecord(ms, 
                                 silenceLength, 
                                 fileName, 
                                 duration, 
                                 dtmfterm, 
                                 MprRecorder::WAV_PCM_16,
                                 ev);
   }
   
   return ret;
}

void CpPhoneMediaInterface::setContactType(int connectionId, SIPX_CONTACT_TYPE eType, SIPX_CONTACT_ID contactId) 
{
    CpPhoneMediaConnection* pMediaConn = getMediaConnection(connectionId);

    if (pMediaConn)
    {
        if (pMediaConn->mIsMulticast && eType == CONTACT_AUTO)
        {
            pMediaConn->mContactType = CONTACT_LOCAL;
        }
        else
        {
            // Only CONTACT_LOCAL is allowed for multicast addresses.
            assert(!pMediaConn->mIsMulticast || eType == CONTACT_LOCAL);
            pMediaConn->mContactType = eType;
        }
    }
}

OsStatus CpPhoneMediaInterface::setAudioCodecBandwidth(int connectionId, int bandWidth) 
{
    return OS_NOT_SUPPORTED ;
}

OsStatus CpPhoneMediaInterface::setCodecList(const SdpCodecList& sdpCodecList)
{
   mSdpCodecList = sdpCodecList;
   // Assign any unset payload types
   mSdpCodecList.bindPayloadIds();

   return OS_SUCCESS;
}

OsStatus CpPhoneMediaInterface::getCodecList(SdpCodecList& sdpCodecList)
{
   sdpCodecList = mSdpCodecList;
   return OS_SUCCESS;
}

OsStatus CpPhoneMediaInterface::setConnectionBitrate(int connectionId, int bitrate) 
{
    return OS_NOT_SUPPORTED ;
}


OsStatus CpPhoneMediaInterface::setConnectionFramerate(int connectionId, int framerate) 
{
    return OS_NOT_SUPPORTED ;
}


OsStatus CpPhoneMediaInterface::setSecurityAttributes(const void* security) 
{
    return OS_NOT_SUPPORTED ;
}

OsStatus CpPhoneMediaInterface::generateVoiceQualityReport(int         connectiond,
                                                           const char* callId,
                                                           UtlString&  report) 
{
	return OS_NOT_SUPPORTED ;
}


/* ============================ ACCESSORS ================================= */


OsStatus CpPhoneMediaInterface::setVideoQuality(int quality)
{
   return OS_SUCCESS;
}

OsStatus CpPhoneMediaInterface::setVideoParameters(int bitRate, int frameRate)
{
   return OS_SUCCESS;
}

// Calculate the current cost for our sending/receiving codecs
int CpPhoneMediaInterface::getCodecCPUCost()
{   
   int iCost = SdpCodec::SDP_CODEC_CPU_LOW ;   

   if (mMediaConnections.entries() > 0)
   {      
      CpPhoneMediaConnection* mediaConnection = NULL;

      // Iterate the connections and determine the most expensive supported 
      // codec.
      UtlDListIterator connectionIterator(mMediaConnections);
      while ((mediaConnection = (CpPhoneMediaConnection*) connectionIterator()))
      {
         // If the codec is null, assume LOW.
         if (mediaConnection->mpAudioCodec != NULL)
         {
            int iCodecCost = mediaConnection->mpAudioCodec->getCPUCost();
            if (iCodecCost > iCost)
               iCost = iCodecCost;
         }

         // Optimization: If we have already hit the highest, kick out.
         if (iCost == SdpCodec::SDP_CODEC_CPU_HIGH)
            break ;
      }
   }
   
   return iCost ;
}


// Calculate the worst case cost for our sending/receiving codecs
int CpPhoneMediaInterface::getCodecCPULimit()
{   
   int iCost = SdpCodec::SDP_CODEC_CPU_LOW ;   
   int         iCodecs = 0 ;
   SdpCodec**  codecs ;


   //
   // If have connections; report what we have offered
   //
   if (mMediaConnections.entries() > 0)
   {      
      CpPhoneMediaConnection* mediaConnection = NULL;

      // Iterate the connections and determine the most expensive supported 
      // codec.
      UtlDListIterator connectionIterator(mMediaConnections);
      while ((mediaConnection = (CpPhoneMediaConnection*) connectionIterator()))
      {
         mediaConnection->mpCodecFactory->getCodecs(iCodecs, codecs) ;      
         for(int i = 0; i < iCodecs; i++)
         {
            // If the cost is greater than what we have, then make that the cost.
            int iCodecCost = codecs[i]->getCPUCost();
            if (iCodecCost > iCost)
               iCost = iCodecCost;

             delete codecs[i];
         }
         delete[] codecs;

         // Optimization: If we have already hit the highest, kick out.
         if (iCost == SdpCodec::SDP_CODEC_CPU_HIGH)
            break ;
      }
   }
   //
   // If no connections; report what we plan on using
   //
   else
   {
      mSdpCodecList.getCodecs(iCodecs, codecs) ;  
      for(int i = 0; i < iCodecs; i++)
      {
         // If the cost is greater than what we have, then make that the cost.
         int iCodecCost = codecs[i]->getCPUCost();
         if (iCodecCost > iCost)
            iCost = iCodecCost;

          delete codecs[i];
      }
      delete[] codecs;
   }

   return iCost ;
}

// Returns the flowgraph's message queue
OsMsgQ* CpPhoneMediaInterface::getMsgQ()
{
   return mpFlowGraph->getMsgQ() ;
}

OsMsgDispatcher* CpPhoneMediaInterface::getMediaNotificationDispatcher()
{
   return mpFlowGraph ? mpFlowGraph->getNotificationDispatcher() : NULL;
}

OsStatus CpPhoneMediaInterface::getPrimaryCodec(int connectionId, 
                                                UtlString& audioCodec,
                                                UtlString& videoCodec,
                                                int* audioPayloadType,
                                                int* videoPayloadType,
                                                bool& isEncrypted)
{
    UtlString codecType;
    CpPhoneMediaConnection* pConnection = getMediaConnection(connectionId);
    if (pConnection == NULL)
       return OS_NOT_FOUND;

    if (pConnection->mpAudioCodec != NULL)
    {
        pConnection->mpAudioCodec->getEncodingName(audioCodec);
        *audioPayloadType = pConnection->mpAudioCodec->getCodecPayloadId();
    }

    videoCodec="";
    *videoPayloadType=0;

   return OS_SUCCESS;
}

OsStatus CpPhoneMediaInterface::getVideoQuality(int& quality)
{
   quality = 0;
   return OS_SUCCESS;
}

OsStatus CpPhoneMediaInterface::getVideoBitRate(int& bitRate)
{
   bitRate = 0;
   return OS_SUCCESS;
}


OsStatus CpPhoneMediaInterface::getVideoFrameRate(int& frameRate)
{
   frameRate = 0;
   return OS_SUCCESS;
}

/* ============================ INQUIRY =================================== */
UtlBoolean CpPhoneMediaInterface::isSendingRtpAudio(int connectionId)
{
   UtlBoolean sending = FALSE;
   CpPhoneMediaConnection* mediaConnection = getMediaConnection(connectionId);

   if(mediaConnection)
   {
       sending = mediaConnection->mRtpAudioSending;
   }
   else
   {
       osPrintf("CpPhoneMediaInterface::isSendingRtpAudio invalid connectionId: %d\n",
          connectionId);
   }

   return(sending);
}

UtlBoolean CpPhoneMediaInterface::isReceivingRtpAudio(int connectionId)
{
   UtlBoolean receiving = FALSE;
   CpPhoneMediaConnection* mediaConnection = getMediaConnection(connectionId);

   if(mediaConnection)
   {
      receiving = mediaConnection->mRtpAudioReceiving;
   }
   else
   {
       osPrintf("CpPhoneMediaInterface::isReceivingRtpAudio invalid connectionId: %d\n",
          connectionId);
   }
   return(receiving);
}

UtlBoolean CpPhoneMediaInterface::isSendingRtpVideo(int connectionId)
{
   UtlBoolean sending = FALSE;

   return(sending);
}

UtlBoolean CpPhoneMediaInterface::isReceivingRtpVideo(int connectionId)
{
   UtlBoolean receiving = FALSE;

   return(receiving);
}


UtlBoolean CpPhoneMediaInterface::isDestinationSet(int connectionId)
{
    UtlBoolean isSet = FALSE;
    CpPhoneMediaConnection* mediaConnection = getMediaConnection(connectionId);

    if(mediaConnection)
    {
        isSet = mediaConnection->mDestinationSet;
    }
    else
    {
       osPrintf("CpPhoneMediaInterface::isDestinationSet invalid connectionId: %d\n",
          connectionId);
    }
    return(isSet);
}

UtlBoolean CpPhoneMediaInterface::canAddParty() 
{
    return (mMediaConnections.entries() < MAX_CONFERENCE_PARTICIPANTS);
}


UtlBoolean CpPhoneMediaInterface::isVideoInitialized(int connectionId)
{
   return false ;
}

UtlBoolean CpPhoneMediaInterface::isAudioInitialized(int connectionId) 
{
    return true ;
}

UtlBoolean CpPhoneMediaInterface::isAudioAvailable() 
{
    return true ;
}

OsStatus CpPhoneMediaInterface::setVideoWindowDisplay(const void* hWnd)
{
   return OS_NOT_YET_IMPLEMENTED;
}

const void* CpPhoneMediaInterface::getVideoWindowDisplay()
{
   return NULL;
}


/* //////////////////////////// PROTECTED ///////////////////////////////// */


UtlBoolean CpPhoneMediaInterface::getLocalAddresses(int connectionId,
                                              UtlString& hostIp,
                                              int& rtpAudioPort,
                                              int& rtcpAudioPort,
                                              int& rtpVideoPort,
                                              int& rtcpVideoPort)
{
    UtlBoolean bRC = FALSE ;
    CpPhoneMediaConnection* pMediaConn = getMediaConnection(connectionId);    

    hostIp.remove(0) ;
    rtpAudioPort = PORT_NONE ;
    rtcpAudioPort = PORT_NONE ;
    rtpVideoPort = PORT_NONE ;
    rtcpVideoPort = PORT_NONE ;

    if (pMediaConn)
    {
        // Audio rtp port (must exist)
        if (pMediaConn->mpRtpAudioSocket)
        {
            hostIp = pMediaConn->mpRtpAudioSocket->getLocalIp();
            rtpAudioPort = pMediaConn->mpRtpAudioSocket->getLocalHostPort();
            if (rtpAudioPort > 0)
            {
                bRC = TRUE ;
            }

            // Audio rtcp port (optional) 
            if (pMediaConn->mpRtcpAudioSocket && bRC)
            {
                rtcpAudioPort = pMediaConn->mpRtcpAudioSocket->getLocalHostPort();
            }        
        }

#ifdef VIDEO
        // Video rtp port (optional)
        if (pMediaConn->mpRtpVideoSocket && bRC)
        {
            rtpVideoPort = pMediaConn->mpRtpVideoSocket->getLocalHostPort();

            // Video rtcp port (optional)
            if (pMediaConn->mpRtcpVideoSocket)
            {
                rtcpVideoPort = pMediaConn->mpRtcpVideoSocket->getLocalHostPort();
            }
        }
#endif
    }

    return bRC ;
}

UtlBoolean CpPhoneMediaInterface::getNatedAddresses(int connectionId,
                                                    UtlString& hostIp,
                                                    int& rtpAudioPort,
                                                    int& rtcpAudioPort,
                                                    int& rtpVideoPort,
                                                    int& rtcpVideoPort)
{
    UtlBoolean bRC = FALSE ;
    UtlString host ;
    int port ;
    CpPhoneMediaConnection* pMediaConn = getMediaConnection(connectionId);

    hostIp.remove(0) ;
    rtpAudioPort = PORT_NONE ;
    rtcpAudioPort = PORT_NONE ;
    rtpVideoPort = PORT_NONE ;
    rtcpVideoPort = PORT_NONE ;

    if (pMediaConn)
    {
        // Audio rtp port (must exist)
        if (pMediaConn->mpRtpAudioSocket)
        {
            if (pMediaConn->mpRtpAudioSocket->getMappedIp(&host, &port))
            {
                if (port > 0)
                {
                    hostIp = host ;
                    rtpAudioPort = port ;

                    bRC = TRUE ;
                }
            
                // Audio rtcp port (optional) 
                if (pMediaConn->mpRtcpAudioSocket && bRC)
                {
                    if (pMediaConn->mpRtcpAudioSocket->getMappedIp(&host, &port))
                    {
                        rtcpAudioPort = port ;
                        if (host.compareTo(hostIp) != 0)
                        {
                            OsSysLog::add(FAC_MP, PRI_ERR, 
                                    "Stun host IP mismatches for rtcp/audio (%s != %s)", 
                                    hostIp.data(), host.data()) ;                          
                        }
                    }
                }
            }
        }

#ifdef VIDEO
        // Video rtp port (optional)
        if (pMediaConn->mpRtpVideoSocket && bRC)
        {
            if (pMediaConn->mpRtpVideoSocket->getMappedIp(&host, &port))
            {
                rtpVideoPort = port ;
                if (host.compareTo(hostIp) != 0)
                {
                    OsSysLog::add(FAC_MP, PRI_ERR, 
                            "Stun host IP mismatches for rtp/video (%s != %s)", 
                            hostIp.data(), host.data()) ;                          
                }

                // Video rtcp port (optional)
                if (pMediaConn->mpRtcpVideoSocket)
                {
                    if (pMediaConn->mpRtcpVideoSocket->getMappedIp(&host, &port))
                    {
                        rtcpVideoPort = port ;
                        if (host.compareTo(hostIp) != 0)
                        {
                            OsSysLog::add(FAC_MP, PRI_ERR, 
                                    "Stun host IP mismatches for rtcp/video (%s != %s)", 
                                    hostIp.data(), host.data()) ;                          
                        }
                    }
                }
            }            
        }
#endif
    }

    return bRC ;
}

UtlBoolean CpPhoneMediaInterface::getRelayAddresses(int connectionId,
                                                    UtlString& hostIp,
                                                    int& rtpAudioPort,
                                                    int& rtcpAudioPort,
                                                    int& rtpVideoPort,
                                                    int& rtcpVideoPort)
{
    UtlBoolean bRC = FALSE ;
    UtlString host ;
    int port ;
    CpPhoneMediaConnection* pMediaConn = getMediaConnection(connectionId);

    hostIp.remove(0) ;
    rtpAudioPort = PORT_NONE ;
    rtcpAudioPort = PORT_NONE ;
    rtpVideoPort = PORT_NONE ;
    rtcpVideoPort = PORT_NONE ;

    if (pMediaConn)
    {
        assert(!pMediaConn->mIsMulticast);
        if (pMediaConn->mIsMulticast)
        {
           return FALSE;
        }

        // Audio rtp port (must exist)
        if (pMediaConn->mpRtpAudioSocket)
        {
            if (((OsNatDatagramSocket*)pMediaConn->mpRtpAudioSocket)->getRelayIp(&host, &port))
            {
                if (port > 0)
                {
                    hostIp = host ;
                    rtpAudioPort = port ;

                    bRC = TRUE ;
                }
            
                // Audio rtcp port (optional) 
                if (pMediaConn->mpRtcpAudioSocket && bRC)
                {
                    if (((OsNatDatagramSocket*)pMediaConn->mpRtcpAudioSocket)->getRelayIp(&host, &port))
                    {
                        rtcpAudioPort = port ;
                        if (host.compareTo(hostIp) != 0)
                        {
                            OsSysLog::add(FAC_MP, PRI_ERR, 
                                    "Turn host IP mismatches for rtcp/audio (%s != %s)", 
                                    hostIp.data(), host.data()) ;                          
                        }
                    }
                }
            }
        }

#ifdef VIDEO
        // Video rtp port (optional)
        if (pMediaConn->mpRtpVideoSocket && bRC)
        {
            if ((OsNatDatagramSocket*)pMediaConn->mpRtpVideoSocket)->getRelayIp(&host, &port))
            {
                rtpVideoPort = port ;
                if (host.compareTo(hostIp) != 0)
                {
                    OsSysLog::add(FAC_MP, PRI_ERR, 
                            "Turn host IP mismatches for rtp/video (%s != %s)", 
                            hostIp.data(), host.data()) ;                          
                }

                // Video rtcp port (optional)
                if (pMediaConn->mpRtcpVideoSocket)
                {
                    if ((OsNatDatagramSocket*)pMediaConn->mpRtcpVideoSocket)->getRelayIp(&host, &port))
                    {
                        rtcpVideoPort = port ;
                        if (host.compareTo(hostIp) != 0)
                        {
                            OsSysLog::add(FAC_MP, PRI_ERR, 
                                    "Turn host IP mismatches for rtcp/video (%s != %s)", 
                                    hostIp.data(), host.data()) ;                          
                        }
                    }
                }
            }            
        }
#endif
    }

    return bRC ;
}

OsStatus CpPhoneMediaInterface::addLocalContacts(int connectionId, 
                                                 int nMaxAddresses,
                                                 UtlString rtpHostAddresses[], 
                                                 int rtpAudioPorts[],
                                                 int rtcpAudioPorts[],
                                                 int rtpVideoPorts[],
                                                 int rtcpVideoPorts[],
                                                 int& nActualAddresses)
{
    UtlString hostIp ;
    int rtpAudioPort = PORT_NONE ;
    int rtcpAudioPort = PORT_NONE ;
    int rtpVideoPort = PORT_NONE ;
    int rtcpVideoPort = PORT_NONE ;
    OsStatus rc = OS_FAILED ;

    // Local Addresses
    if (    (nActualAddresses < nMaxAddresses) && 
            getLocalAddresses(connectionId, hostIp, rtpAudioPort, 
            rtcpAudioPort, rtpVideoPort, rtcpVideoPort) )
    {
        UtlBoolean bDuplicate = FALSE ;
        
        // Check for duplicates
        for (int i=0; i<nActualAddresses; i++)
        {
            if (    (rtpHostAddresses[i].compareTo(hostIp) == 0) &&
                    (rtpAudioPorts[i] == rtpAudioPort) &&
                    (rtcpAudioPorts[i] == rtcpAudioPort) &&
                    (rtpVideoPorts[i] == rtpVideoPort) &&
                    (rtcpVideoPorts[i] == rtcpVideoPort))
            {
                bDuplicate = TRUE ;
                break ;
            }
        }

        if (!bDuplicate)
        {
            rtpHostAddresses[nActualAddresses] = hostIp ;
            rtpAudioPorts[nActualAddresses] = rtpAudioPort ;
            rtcpAudioPorts[nActualAddresses] = rtcpAudioPort ;
            rtpVideoPorts[nActualAddresses] = rtpVideoPort ;
            rtcpVideoPorts[nActualAddresses] = rtcpVideoPort ;
            nActualAddresses++ ;

            rc = OS_SUCCESS ;
        }
    }

    return rc ;
}


OsStatus CpPhoneMediaInterface::addNatedContacts(int connectionId, 
                                                 int nMaxAddresses,
                                                 UtlString rtpHostAddresses[], 
                                                 int rtpAudioPorts[],
                                                 int rtcpAudioPorts[],
                                                 int rtpVideoPorts[],
                                                 int rtcpVideoPorts[],
                                                 int& nActualAddresses)
{
    UtlString hostIp ;
    int rtpAudioPort = PORT_NONE ;
    int rtcpAudioPort = PORT_NONE ;
    int rtpVideoPort = PORT_NONE ;
    int rtcpVideoPort = PORT_NONE ;
    OsStatus rc = OS_FAILED ;

    // NAT Addresses
    if (    (nActualAddresses < nMaxAddresses) && 
            getNatedAddresses(connectionId, hostIp, rtpAudioPort, 
            rtcpAudioPort, rtpVideoPort, rtcpVideoPort) )
    {
        bool bDuplicate = false ;
        
        // Check for duplicates
        for (int i=0; i<nActualAddresses; i++)
        {
            if (    (rtpHostAddresses[i].compareTo(hostIp) == 0) &&
                    (rtpAudioPorts[i] == rtpAudioPort) &&
                    (rtcpAudioPorts[i] == rtcpAudioPort) &&
                    (rtpVideoPorts[i] == rtpVideoPort) &&
                    (rtcpVideoPorts[i] == rtcpVideoPort))
            {
                bDuplicate = true ;
                break ;
            }
        }

        if (!bDuplicate)
        {
            rtpHostAddresses[nActualAddresses] = hostIp ;
            rtpAudioPorts[nActualAddresses] = rtpAudioPort ;
            rtcpAudioPorts[nActualAddresses] = rtcpAudioPort ;
            rtpVideoPorts[nActualAddresses] = rtpVideoPort ;
            rtcpVideoPorts[nActualAddresses] = rtcpVideoPort ;
            nActualAddresses++ ;

            rc = OS_SUCCESS ;
        }
    }
    return rc ;
}


OsStatus CpPhoneMediaInterface::addRelayContacts(int connectionId, 
                                                 int nMaxAddresses,
                                                 UtlString rtpHostAddresses[], 
                                                 int rtpAudioPorts[],
                                                 int rtcpAudioPorts[],
                                                 int rtpVideoPorts[],
                                                 int rtcpVideoPorts[],
                                                 int& nActualAddresses)
{
    UtlString hostIp ;
    int rtpAudioPort = PORT_NONE ;
    int rtcpAudioPort = PORT_NONE ;
    int rtpVideoPort = PORT_NONE ;
    int rtcpVideoPort = PORT_NONE ;
    OsStatus rc = OS_FAILED ;

    // Relay Addresses
    if (    (nActualAddresses < nMaxAddresses) && 
            getRelayAddresses(connectionId, hostIp, rtpAudioPort, 
            rtcpAudioPort, rtpVideoPort, rtcpVideoPort) )
    {
        bool bDuplicate = false ;
        
        // Check for duplicates
        for (int i=0; i<nActualAddresses; i++)
        {
            if (    (rtpHostAddresses[i].compareTo(hostIp) == 0) &&
                    (rtpAudioPorts[i] == rtpAudioPort) &&
                    (rtcpAudioPorts[i] == rtcpAudioPort) &&
                    (rtpVideoPorts[i] == rtpVideoPort) &&
                    (rtcpVideoPorts[i] == rtcpVideoPort))
            {
                bDuplicate = true ;
                break ;
            }
        }

        if (!bDuplicate)
        {
            rtpHostAddresses[nActualAddresses] = hostIp ;
            rtpAudioPorts[nActualAddresses] = rtpAudioPort ;
            rtcpAudioPorts[nActualAddresses] = rtcpAudioPort ;
            rtpVideoPorts[nActualAddresses] = rtpVideoPort ;
            rtcpVideoPorts[nActualAddresses] = rtcpVideoPort ;
            nActualAddresses++ ;

            rc = OS_SUCCESS ;
        }
    }

    return rc ;
}


void CpPhoneMediaInterface::applyAlternateDestinations(int connectionId) 
{
    UtlString destAddress ;
    int       destPort ;

    CpPhoneMediaConnection* pMediaConnection = getMediaConnection(connectionId);

    if (pMediaConnection)
    {
        assert(!pMediaConnection->mIsMulticast);
        if (pMediaConnection->mIsMulticast)
        {
           return;
        }

        assert(!pMediaConnection->mDestinationSet) ;
        pMediaConnection->mDestinationSet = true ;

        pMediaConnection->mRtpSendHostAddress.remove(0) ;
        pMediaConnection->mRtpAudioSendHostPort = 0 ;
        pMediaConnection->mRtcpAudioSendHostPort = 0 ;
#ifdef VIDEO
        pMediaConnection->mRtpVideoSendHostPort = 0 ;
        pMediaConnection->mRtcpVideoSendHostPort = 0 ;
#endif

        // TODO: We should REALLY store a different host for each connection -- they could
        //       differ when using TURN (could get forwarded to another turn server)
        //       For now, we store the rtp host


        // Connect RTP Audio Socket
        if (pMediaConnection->mpRtpAudioSocket)
        {
           OsNatDatagramSocket *pSocket = (OsNatDatagramSocket*)pMediaConnection->mpRtpAudioSocket;
            if (pSocket->getBestDestinationAddress(destAddress, destPort))
            {
                pSocket->applyDestinationAddress(destAddress, destPort, !mStunServer.isNull(), mStunRefreshPeriodSecs, !mTurnServer.isNull());
                pMediaConnection->mRtpSendHostAddress = destAddress;
                pMediaConnection->mRtpAudioSendHostPort = destPort;
            }
        }

        // Connect RTCP Audio Socket
        if (pMediaConnection->mpRtcpAudioSocket)
        {
            OsNatDatagramSocket *pSocket = (OsNatDatagramSocket*)pMediaConnection->mpRtcpAudioSocket;
            if (pSocket->getBestDestinationAddress(destAddress, destPort))
            {
                pSocket->applyDestinationAddress(destAddress, destPort, !mStunServer.isNull(), mStunRefreshPeriodSecs, !mTurnServer.isNull()) ;                
                pMediaConnection->mRtcpAudioSendHostPort = destPort;                
            }            
        }

        // TODO:: Enable/Disable RTCP

#ifdef VIDEO
        // Connect RTP Video Socket
        if (pMediaConnection->mpRtpVideoSocket)
        {
            OsNatDatagramSocket *pSocket = (OsNatDatagramSocket*)pMediaConnection->mpRtpVideoSocket;
            if (pSocket->getBestDestinationAddress(destAddress, destPort))
            {
                pSocket->applyDestinationAddress(destAddress, destPort, !mStunServer.isNull(), mStunRefreshPeriodSecs, !mTurnServer.isNull()) ;                
                pMediaConnection->mRtpVideoSendHostPort = destPort;
            }            
        }

        // Connect RTCP Video Socket
        if (pMediaConnection->mpRtcpVideoSocket)
        {
            OsNatDatagramSocket *pSocket = (OsNatDatagramSocket*)pMediaConnection->mpRtcpVideoSocket;
            if (pSocket->getBestDestinationAddress(destAddress, destPort))
            {
                pSocket->applyDestinationAddress(destAddress, destPort, !mStunServer.isNull(), mStunRefreshPeriodSecs, !mTurnServer.isNull()) ;                
                pMediaConnection->mRtcpVideoSendHostPort = destPort;
            }            
        }

        // TODO:: Enable/Disable RTCP
#endif
    }
}

OsStatus CpPhoneMediaInterface::setMediaProperty(const UtlString& propertyName,
                                                 const UtlString& propertyValue)
{
    OsSysLog::add(FAC_CP, PRI_ERR, 
        "CpPhoneMediaInterface::setMediaProperty %p propertyName=\"%s\" propertyValue=\"%s\"",
        this, propertyName.data(), propertyValue.data());

    UtlString* oldProperty = (UtlString*)mInterfaceProperties.findValue(&propertyValue);
    if(oldProperty)
    {
        // Update the old value
        (*oldProperty) = propertyValue;
    }
    else
    {
        // No prior value for this property create copies for the map
        mInterfaceProperties.insertKeyAndValue(new UtlString(propertyName),
                                               new UtlString(propertyValue));
    }
    return OS_NOT_YET_IMPLEMENTED;
}

OsStatus CpPhoneMediaInterface::getMediaProperty(const UtlString& propertyName,
                                                 UtlString& propertyValue)
{
    OsStatus returnCode = OS_NOT_FOUND;
    OsSysLog::add(FAC_CP, PRI_ERR, 
        "CpPhoneMediaInterface::getMediaProperty %p propertyName=\"%s\"",
        this, propertyName.data());

    UtlString* foundValue = (UtlString*)mInterfaceProperties.findValue(&propertyName);
    if(foundValue)
    {
        propertyValue = *foundValue;
        returnCode = OS_SUCCESS;
    }
    else
    {
        propertyValue = "";
        returnCode = OS_NOT_FOUND;
    }

    returnCode = OS_NOT_YET_IMPLEMENTED;
    return(returnCode);
}

OsStatus CpPhoneMediaInterface::setMediaProperty(int connectionId,
                                                 const UtlString& propertyName,
                                                 const UtlString& propertyValue)
{
    OsStatus returnCode = OS_NOT_YET_IMPLEMENTED;
    OsSysLog::add(FAC_CP, PRI_ERR, 
        "CpPhoneMediaInterface::setMediaProperty %p connectionId=%d propertyName=\"%s\" propertyValue=\"%s\"",
        this, connectionId, propertyName.data(), propertyValue.data());

    CpPhoneMediaConnection* mediaConnection = getMediaConnection(connectionId);
    if(mediaConnection)
    {
        UtlString* oldProperty = (UtlString*)(mediaConnection->mConnectionProperties.findValue(&propertyValue));
        if(oldProperty)
        {
            // Update the old value
            (*oldProperty) = propertyValue;
        }
        else
        {
            // No prior value for this property create copies for the map
            mediaConnection->mConnectionProperties.insertKeyAndValue(new UtlString(propertyName),
                                                                     new UtlString(propertyValue));
        }
    }
    else
    {
        returnCode = OS_NOT_FOUND;
    }


    return(returnCode);
}

OsStatus CpPhoneMediaInterface::getMediaProperty(int connectionId,
                                                 const UtlString& propertyName,
                                                 UtlString& propertyValue)
{
    OsStatus returnCode = OS_NOT_FOUND;
    propertyValue = "";

    OsSysLog::add(FAC_CP, PRI_ERR, 
        "CpPhoneMediaInterface::getMediaProperty %p connectionId=%d propertyName=\"%s\"",
        this, connectionId, propertyName.data());

    CpPhoneMediaConnection* mediaConnection = getMediaConnection(connectionId);
    if(mediaConnection)
    {
        UtlString* oldProperty = (UtlString*)(mediaConnection->mConnectionProperties.findValue(&propertyName));
        if(oldProperty)
        {
            propertyValue = *oldProperty;
            returnCode = OS_SUCCESS;
        }
    }

    return OS_NOT_YET_IMPLEMENTED;//(returnCode);
}
/* //////////////////////////// PROTECTED ///////////////////////////////// */

OsStatus CpPhoneMediaInterface::createRtpSocketPair(UtlString localAddress,
                                                    int localPort,
                                                    SIPX_CONTACT_TYPE contactType,
                                                    OsDatagramSocket* &rtpSocket,
                                                    OsDatagramSocket* &rtcpSocket)
{
   int firstRtpPort;
   bool localPortGiven = (localPort != 0); // Does user specified the local port?
   UtlBoolean isMulticast = OsSocket::isMcastAddr(localAddress);

   if (!localPortGiven)
   {
      mpFactoryImpl->getNextRtpPort(localPort);
      firstRtpPort = localPort;
   }

   if (isMulticast)
   {
        rtpSocket = new OsMulticastSocket(localPort, localAddress,
                                          localPort, localAddress);
        rtcpSocket = new OsMulticastSocket(
                              localPort == 0 ? 0 : localPort + 1, localAddress,
                              localPort == 0 ? 0 : localPort + 1, localAddress);
   }
   else
   {
       rtpSocket = new OsNatDatagramSocket(0, NULL, localPort, localAddress, NULL);
       ((OsNatDatagramSocket*)rtpSocket)->enableTransparentReads(false);

       rtcpSocket = new OsNatDatagramSocket(0, NULL,localPort == 0 ? 0 : localPort+1,
                                            localAddress, NULL);
       ((OsNatDatagramSocket*)rtcpSocket)->enableTransparentReads(false);
   }

   // Validate local port is not auto-selecting.
   if (localPort != 0 && !localPortGiven)
   {
      // If either of the sockets are bad (e.g. already in use) or
      // if either have stuff on them to read (e.g. someone is
      // sending junk to the ports, look for another port pair
      while(!rtpSocket->isOk() ||
            !rtcpSocket->isOk() ||
             rtcpSocket->isReadyToRead() ||
             rtpSocket->isReadyToRead(60))
      {
            localPort +=2;
            // This should use mLastRtpPort instead of some
            // hardcoded MAX, but I do not think mLastRtpPort
            // is set correctly in all of the products.
            if(localPort > firstRtpPort + MAX_RTP_PORTS) 
            {
               OsSysLog::add(FAC_CP, PRI_ERR, 
                  "No available ports for RTP and RTCP in range %d - %d",
                  firstRtpPort, firstRtpPort + MAX_RTP_PORTS);
               break;  // time to give up
            }

            delete rtpSocket;
            delete rtcpSocket;
            if (isMulticast)
            {
               OsMulticastSocket* rtpSocket = new OsMulticastSocket(
                        localPort, localAddress,
                        localPort, localAddress);
               OsMulticastSocket* rtcpSocket = new OsMulticastSocket(
                        localPort == 0 ? 0 : localPort + 1, localAddress,
                        localPort == 0 ? 0 : localPort + 1, localAddress);
            }
            else
            {
               rtpSocket = new OsNatDatagramSocket(0, NULL, localPort, localAddress, NULL);
               ((OsNatDatagramSocket*)rtpSocket)->enableTransparentReads(false);

               rtcpSocket = new OsNatDatagramSocket(0, NULL,localPort == 0 ? 0 : localPort+1,
                                                   localAddress, NULL);
               ((OsNatDatagramSocket*)rtcpSocket)->enableTransparentReads(false);
            }
      }
   }

   // Did our sockets get created OK?
   if (!rtpSocket->isOk() || !rtcpSocket->isOk())
   {
       delete rtpSocket;
       delete rtcpSocket;
       return OS_NETWORK_UNAVAILABLE;
   }

   if (isMulticast)
   {
       // Set multicast options
       const unsigned char MC_HOP_COUNT = 8;
       ((OsMulticastSocket*)rtpSocket)->setHopCount(MC_HOP_COUNT);
       ((OsMulticastSocket*)rtcpSocket)->setHopCount(MC_HOP_COUNT);
       ((OsMulticastSocket*)rtpSocket)->setLoopback(false);
       ((OsMulticastSocket*)rtcpSocket)->setLoopback(false);
   }

   // Set a maximum on the buffers for the sockets so
   // that the network stack does not get swamped by early media
   // from the other side;
   {
      int sRtp, sRtcp, oRtp, oRtcp, optlen;

      sRtp = rtpSocket->getSocketDescriptor();
      sRtcp = rtcpSocket->getSocketDescriptor();

      optlen = sizeof(int);
      oRtp = 20000;
      setsockopt(sRtp, SOL_SOCKET, SO_RCVBUF, (char *) (&oRtp), optlen);
      oRtcp = 500;
      setsockopt(sRtcp, SOL_SOCKET, SO_RCVBUF, (char *) (&oRtcp), optlen);

      // Set the type of service (DiffServ code point) to low delay
      int tos = mExpeditedIpTos;
      
#ifndef WIN32 // [
            // Under Windows this options are supported under Win2000 only and
            // are not recommended to use.
      setsockopt (sRtp, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(int));
      setsockopt (sRtcp, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(int));
#else  // WIN32 ][
      // TODO:: Implement QoS  request under Windows.
#endif // WIN32 ]
   }

   if (!isMulticast)
   {
      NAT_BINDING rtpBindingMode = NO_BINDING;
      NAT_BINDING rtcpBindingMode = NO_BINDING;

      // Enable Stun if we have a stun server and either non-local contact type or 
      // ICE is enabled.
      if ((mStunServer.length() != 0) && ((contactType != CONTACT_LOCAL) || mEnableIce))
      {
         ((OsNatDatagramSocket*)rtpSocket)->enableStun(mStunServer, mStunPort, mStunRefreshPeriodSecs, 0, false) ;
         rtpBindingMode = STUN_BINDING;
      }

      // Enable Turn if we have a stun server and either non-local contact type or 
      // ICE is enabled.
      if ((mTurnServer.length() != 0) && ((contactType != CONTACT_LOCAL) || mEnableIce))
      {
         ((OsNatDatagramSocket*)rtpSocket)->enableTurn(mTurnServer, mTurnPort, 
                  mTurnRefreshPeriodSecs, mTurnUsername, mTurnPassword, false) ;

         if (rtpBindingMode == STUN_BINDING)
         {
            rtpBindingMode = STUN_TURN_BINDING;
         }
         else
         {
            rtpBindingMode = TURN_BINDING;
         }
      }

      // Enable Stun if we have a stun server and either non-local contact type or 
      // ICE is enabled.
      if ((mStunServer.length() != 0) && ((contactType != CONTACT_LOCAL) || mEnableIce))
      {
         ((OsNatDatagramSocket*)rtcpSocket)->enableStun(mStunServer, mStunPort, mStunRefreshPeriodSecs, 0, false) ;
         rtcpBindingMode = STUN_BINDING;
      }

      // Enable Turn if we have a stun server and either non-local contact type or 
      // ICE is enabled.
      if ((mTurnServer.length() != 0) && ((contactType != CONTACT_LOCAL) || mEnableIce))
      {
         ((OsNatDatagramSocket*)rtcpSocket)->enableTurn(mTurnServer, mTurnPort, 
                  mTurnRefreshPeriodSecs, mTurnUsername, mTurnPassword, false) ;

         if (rtcpBindingMode == STUN_BINDING)
         {
            rtcpBindingMode = STUN_TURN_BINDING;
         }
         else
         {
            rtcpBindingMode = TURN_BINDING;
         }
      }

      // wait until all sockets have results
      if (rtpBindingMode != NO_BINDING || rtcpBindingMode!= NO_BINDING)
      {
         bool bRepeat = true;
         while(bRepeat)
         {
            bRepeat = false;
            bRepeat |= ((OsNatDatagramSocket*)rtpSocket)->waitForBinding(rtpBindingMode, false);
            bRepeat |= ((OsNatDatagramSocket*)rtcpSocket)->waitForBinding(rtcpBindingMode, false);
            // repeat as long as one of sockets is waiting for result
         }
      }
   }

   return OS_SUCCESS;
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */


