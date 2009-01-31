// 
// Copyright (C) 2005-2007 SIPez LLC
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2005-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
// 
// $$
//////////////////////////////////////////////////////////////////////////////

// Author: Dan Petrie (dpetrie AT SIPez DOT com)

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>
#include <mi/CpMediaInterfaceFactory.h>
#include <mi/CpMediaInterfaceFactoryFactory.h>
#include <mi/CpMediaInterface.h>
#include <os/OsTask.h>
#include <utl/UtlSList.h>
#include <utl/UtlInt.h>
#include <os/OsMsgDispatcher.h>

#ifdef RTL_ENABLED
#  include <rtl_macro.h>
   RTL_DECLARE
#else
#  define RTL_START(x)
#  define RTL_BLOCK(x)
#  define RTL_EVENT(x,y)
#  define RTL_WRITE(x)
#  define RTL_STOP
#endif

// Unittest for CpPhoneMediaInterface

class CpPhoneMediaInterfaceTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(CpPhoneMediaInterfaceTest);
    CPPUNIT_TEST(printMediaInterfaceType); // Just prints the media interface type.
    CPPUNIT_TEST(testTones);
    CPPUNIT_TEST(testTwoTones);
    CPPUNIT_TEST_SUITE_END();

    public:

    CpMediaInterfaceFactory* mpMediaFactory;

    CpPhoneMediaInterfaceTest()
    {
    };

    virtual void setUp()
    {
        enableConsoleOutput(0);

        mpMediaFactory = sipXmediaFactoryFactory(NULL);
    } 

    virtual void tearDown()
    {
        sipxDestroyMediaFactoryFactory();
        mpMediaFactory = NULL;
    }

    void printMediaInterfaceType()
    {
        CPPUNIT_ASSERT(mpMediaFactory);
        CpMediaInterface* mediaInterface = 
            mpMediaFactory->createMediaInterface(NULL, NULL, "", 
                                                 "", "", 0, "", 0, 0, "",
                                                 0, "", "", 0, false);
        UtlString miType = mediaInterface->getType();
        if(miType == "SipXMediaInterfaceImpl")
        {
            printf("Phone media interface enabled\n");
        }
        else
        {
            CPPUNIT_FAIL("ERROR: Unknown type of media interface!");
        }
        mediaInterface->release();
    }

    void testTones()
    {
        RTL_START(1600000);

        CPPUNIT_ASSERT(mpMediaFactory);

        SdpCodecList* sdpCodecList = new SdpCodecList();
        CPPUNIT_ASSERT(sdpCodecList);
        UtlSList utlCodecList;
        sdpCodecList->getCodecs(utlCodecList);

        UtlString localRtpInterfaceAddress("127.0.0.1");
        UtlString locale;
        int tosOptions = 0;
        UtlString stunServerAddress;
        int stunOptions = 0;
        int stunKeepAlivePeriodSecs = 25;
        UtlString turnServerAddress;
        int turnPort = 0 ;
        UtlString turnUser;
        UtlString turnPassword;
        int turnKeepAlivePeriodSecs = 25;
        bool enableIce = false ;


        CpMediaInterface* mediaInterface = 
            mpMediaFactory->createMediaInterface(NULL,
                                                 sdpCodecList,
                                                 NULL, // public mapped RTP IP address
                                                 localRtpInterfaceAddress, 
                                                 locale,
                                                 tosOptions,
                                                 stunServerAddress, 
                                                 stunOptions, 
                                                 stunKeepAlivePeriodSecs,
                                                 turnServerAddress,
                                                 turnPort,
                                                 turnUser,
                                                 turnPassword,
                                                 turnKeepAlivePeriodSecs,
                                                 enableIce);


        // Record the entire "call" - all connections.
        mediaInterface->recordAudio("testTones_call_recording.wav");

        mediaInterface->giveFocus() ;

        RTL_EVENT("Tone set", 0);
        printf("first tone set\n");
        RTL_EVENT("Tone set", 1);
        mediaInterface->startTone(6, true, false) ;OsTask::delay(500) ;
        RTL_EVENT("Tone set", 2);
        mediaInterface->startTone(8, true, false) ;OsTask::delay(500) ;
        RTL_EVENT("Tone set", 3);
        mediaInterface->startTone(4, true, false) ;OsTask::delay(500) ;
        RTL_EVENT("Tone set", 0);
        printf("second tone set\n");        
        OsTask::delay(500) ;
        RTL_EVENT("Tone set", 1);
        mediaInterface->startTone(6, true, false) ;OsTask::delay(500) ;
        RTL_EVENT("Tone set", 2);
        mediaInterface->startTone(8, true, false) ;OsTask::delay(500) ;
        RTL_EVENT("Tone set", 3);
        mediaInterface->startTone(4, true, false) ;OsTask::delay(500) ;
        RTL_EVENT("Tone set", 0);
        printf("third tone set\n");        
        OsTask::delay(500) ;
        RTL_EVENT("Tone set", 1);
        mediaInterface->startTone(9, true, false) ;OsTask::delay(500) ;
        mediaInterface->startTone(5, true, false) ;OsTask::delay(500) ;
        mediaInterface->startTone(5, true, false) ;OsTask::delay(500) ;
        mediaInterface->startTone(4, true, false) ;OsTask::delay(500) ;
        RTL_EVENT("Tone set", 0);
        printf("fourth tone set\n");        
        OsTask::delay(500) ;
        RTL_EVENT("Tone set", 1);
        mediaInterface->startTone(9, true, false) ;OsTask::delay(500) ;
        mediaInterface->startTone(5, true, false) ;OsTask::delay(500) ;
        mediaInterface->startTone(5, true, false) ;OsTask::delay(500) ;
        mediaInterface->startTone(4, true, false) ;OsTask::delay(500) ;
        RTL_EVENT("Tone set", 0);
        printf("tone set done\n");        
        OsTask::delay(1000) ;

        // Stop recording the "call" -- all connections.
        mediaInterface->stopRecording();

        RTL_WRITE("testTones.rtl");
        RTL_STOP;

        // delete interface
        mediaInterface->release(); 

        OsTask::delay(500) ;
        delete sdpCodecList ;
    };

    void testTwoTones()
    {
        RTL_START(2400000);

        // This test creates three flowgraphs.  It streams RTP with tones
        // from the 2nd and 3rd to be received and mixed in the first flowgraph
        // So we test RTP and we test that we can generate 2 different tones in
        // to different flowgraphs to ensure that the ToneGen has no global
        // interactions or dependencies.
        CPPUNIT_ASSERT(mpMediaFactory);

        SdpCodecList* pSdpCodecList = new SdpCodecList();
        CPPUNIT_ASSERT(pSdpCodecList);
        UtlSList utlCodecList;
        pSdpCodecList->getCodecs(utlCodecList);

        UtlString localRtpInterfaceAddress("127.0.0.1");
        OsSocket::getHostIp(&localRtpInterfaceAddress);
        UtlString locale;
        int tosOptions = 0;
        UtlString stunServerAddress;
        int stunOptions = 0;
        int stunKeepAlivePeriodSecs = 25;
        UtlString turnServerAddress;
        int turnPort = 0 ;
        UtlString turnUser;
        UtlString turnPassword;
        int turnKeepAlivePeriodSecs = 25;
        bool enableIce = false ;

        // Create a flowgraph (sink) to receive and mix 2 sources
        CpMediaInterface* mixedInterface = 
            mpMediaFactory->createMediaInterface(NULL,
                                                 pSdpCodecList,
                                                 NULL, // public mapped RTP IP address
                                                 localRtpInterfaceAddress, 
                                                 locale,
                                                 tosOptions,
                                                 stunServerAddress, 
                                                 stunOptions, 
                                                 stunKeepAlivePeriodSecs,
                                                 turnServerAddress,
                                                 turnPort,
                                                 turnUser,
                                                 turnPassword,
                                                 turnKeepAlivePeriodSecs,
                                                 enableIce);

        // Create connections for mixed(sink) flowgraph
        int mixedConnection1Id = -1;
        CPPUNIT_ASSERT(mixedInterface->createConnection(mixedConnection1Id, NULL) == OS_SUCCESS);
        CPPUNIT_ASSERT(mixedConnection1Id > 0);
        int mixedConnection2Id = -1;
        CPPUNIT_ASSERT(mixedInterface->createConnection(mixedConnection2Id, NULL) == OS_SUCCESS);
        CPPUNIT_ASSERT(mixedConnection2Id > 0);
        
        // Get the address of the connections so we can send RTP to them
        // capabilities of first connection on mixed(sink) flowgraph
        const int maxAddresses = 1;
        UtlString rtpHostAddresses1[maxAddresses];
        int rtpAudioPorts1[maxAddresses];
        int rtcpAudioPorts1[maxAddresses];
        int rtpVideoPorts1[maxAddresses];
        int rtcpVideoPorts1[maxAddresses];
        RTP_TRANSPORT transportTypes1[maxAddresses];
        int numActualAddresses1;
        SdpCodecList supportedCodecs1;
        SdpSrtpParameters srtpParameters1;
        int videoBandwidth1;
        int videoFramerate1;
        CPPUNIT_ASSERT_EQUAL(
            mixedInterface->getCapabilitiesEx(mixedConnection1Id, 
                                             maxAddresses,
                                             rtpHostAddresses1, 
                                             rtpAudioPorts1,
                                             rtcpAudioPorts1,
                                             rtpVideoPorts1,
                                             rtcpVideoPorts1,
                                             transportTypes1,
                                             numActualAddresses1,
                                             supportedCodecs1,
                                             srtpParameters1,
                                             videoBandwidth1,
                                             videoFramerate1), 

             OS_SUCCESS);

        // capabilities of second connection on mixed(sink) flowgraph
        UtlString rtpHostAddresses2[maxAddresses];
        int rtpAudioPorts2[maxAddresses];
        int rtcpAudioPorts2[maxAddresses];
        int rtpVideoPorts2[maxAddresses];
        int rtcpVideoPorts2[maxAddresses];
        RTP_TRANSPORT transportTypes2[maxAddresses];
        int numActualAddresses2;
        SdpCodecList supportedCodecs2;
        SdpSrtpParameters srtpParameters2;
        int videoBandwidth2;
        int videoFramerate2;
        CPPUNIT_ASSERT_EQUAL(
            mixedInterface->getCapabilitiesEx(mixedConnection2Id, 
                                             maxAddresses,
                                             rtpHostAddresses2, 
                                             rtpAudioPorts2,
                                             rtcpAudioPorts2,
                                             rtpVideoPorts2,
                                             rtcpVideoPorts2,
                                             transportTypes2,
                                             numActualAddresses2,
                                             supportedCodecs2,
                                             srtpParameters2,
                                             videoBandwidth2,
                                             videoFramerate2), 

             OS_SUCCESS);

        // Prep the sink connections to receive RTP
        UtlSList codec1List;
        supportedCodecs1.getCodecs(codec1List);
        CPPUNIT_ASSERT_EQUAL(
            mixedInterface->startRtpReceive(mixedConnection1Id,
                                            codec1List),
            OS_SUCCESS);

        // Want to hear what is on the mixed flowgraph
        mixedInterface->giveFocus();

        UtlSList codec2List;
        supportedCodecs2.getCodecs(codec2List);
        CPPUNIT_ASSERT_EQUAL(
            mixedInterface->startRtpReceive(mixedConnection2Id,
                                            codec2List),
            OS_SUCCESS);

        // Second flowgraph to be one of two sources
        CpMediaInterface* source1Interface = 
            mpMediaFactory->createMediaInterface(NULL,
                                                 pSdpCodecList,
                                                 NULL, // public mapped RTP IP address
                                                 localRtpInterfaceAddress, 
                                                 locale,
                                                 tosOptions,
                                                 stunServerAddress, 
                                                 stunOptions, 
                                                 stunKeepAlivePeriodSecs,
                                                 turnServerAddress,
                                                 turnPort,
                                                 turnUser,
                                                 turnPassword,
                                                 turnKeepAlivePeriodSecs,
                                                 enableIce);

        // Create connection for source 1 flowgraph
        int source1ConnectionId = -1;
        CPPUNIT_ASSERT(source1Interface->createConnection(source1ConnectionId, NULL) == OS_SUCCESS);
        CPPUNIT_ASSERT(source1ConnectionId > 0);

        // Set the destination for sending RTP from source 1 to connection 1 on
        // the mix flowgraph
        printf("rtpHostAddresses1: \"%s\"\nrtpAudioPorts1: %d\nrtcpAudioPorts1: %d\nrtpVideoPorts1: %d\nrtcpVideoPorts1: %d\n",
            rtpHostAddresses1->data(), 
            *rtpAudioPorts1,
            *rtcpAudioPorts1,
            *rtpVideoPorts1,
            *rtcpVideoPorts1);

        CPPUNIT_ASSERT_EQUAL(
            source1Interface->setConnectionDestination(source1ConnectionId,
                                                       rtpHostAddresses1->data(), 
                                                       *rtpAudioPorts1,
                                                       *rtcpAudioPorts1,
                                                       *rtpVideoPorts1,
                                                       *rtcpVideoPorts1),
            OS_SUCCESS);

        // Start sending RTP from source 1 to the mix flowgraph
        CPPUNIT_ASSERT_EQUAL(
            source1Interface->startRtpSend(source1ConnectionId, 
                                           codec1List),
            OS_SUCCESS);


        // Second flowgraph to be one of two sources
        CpMediaInterface* source2Interface = 
            mpMediaFactory->createMediaInterface(NULL,
                                                 pSdpCodecList,
                                                 NULL, // public mapped RTP IP address
                                                 localRtpInterfaceAddress, 
                                                 locale,
                                                 tosOptions,
                                                 stunServerAddress, 
                                                 stunOptions, 
                                                 stunKeepAlivePeriodSecs,
                                                 turnServerAddress,
                                                 turnPort,
                                                 turnUser,
                                                 turnPassword,
                                                 turnKeepAlivePeriodSecs,
                                                 enableIce);

        // Create connection for source 2 flowgraph
        int source2ConnectionId = -1;
        CPPUNIT_ASSERT(source2Interface->createConnection(source2ConnectionId, NULL) == OS_SUCCESS);
        CPPUNIT_ASSERT(source2ConnectionId > 0);

        // Set the destination for sending RTP from source 2 to connection 2 on
        // the mix flowgraph
        CPPUNIT_ASSERT_EQUAL(
            source2Interface->setConnectionDestination(source2ConnectionId,
                                                       *rtpHostAddresses2, 
                                                       *rtpAudioPorts2,
                                                       *rtcpAudioPorts2,
                                                       *rtpVideoPorts2,
                                                       *rtcpVideoPorts2),
            OS_SUCCESS);

        RTL_EVENT("Tone count", 0);

        // Record the entire "call" - all connections.
        mixedInterface->recordAudio("testTwoTones_call_recording.wav");

        // Start sending RTP from source 2 to the mix flowgraph
        CPPUNIT_ASSERT_EQUAL(
            source2Interface->startRtpSend(source2ConnectionId, 
                                           codec2List),
            OS_SUCCESS);

        RTL_EVENT("Tone count", 1);
        printf("generate tones in source 1\n");
        source1Interface->startTone(1, true, true);

        OsTask::delay(1000);

        RTL_EVENT("Tone count", 2);
        printf("generate tones in source 2 as well\n");
        source2Interface->startTone(2, true, true);

        OsTask::delay(1000);

        RTL_EVENT("Tone count", 1);
        printf("stop tones in source 1\n");

        OsTask::delay(1000);
        RTL_EVENT("Tone count", 0);

        OsTask::delay(1000);
        printf("two tones done\n");        

        // Stop recording the "call" -- all connections.
        mixedInterface->stopRecording();

        // Delete connections
        mixedInterface->deleteConnection(mixedConnection1Id);
        mixedInterface->deleteConnection(mixedConnection2Id);
        source1Interface->deleteConnection(source1ConnectionId);
        source2Interface->deleteConnection(source2ConnectionId);

        // delete interfaces
        mixedInterface->release();
        source1Interface->release();
        source2Interface->release();

        OsTask::delay(500) ;

        RTL_WRITE("testTwoTones.rtl");
        RTL_STOP;

        delete pSdpCodecList ;
    };
};

CPPUNIT_TEST_SUITE_REGISTRATION(CpPhoneMediaInterfaceTest);
