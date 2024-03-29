//  
// Copyright (C) 2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
//////////////////////////////////////////////////////////////////////////////
// Author: Dan Petrie (dpetrie AT SIPez DOT com)

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>
#include <sipxunit/TestUtilities.h>
#include <utl/UtlHashMap.h>
#include <utl/UtlString.h>
#include <os/OsDefs.h>
#include <os/OsDateTime.h>
#include <net/SipDialog.h>
#include <net/SipMessage.h>
#include <net/SipUserAgent.h>
#include <net/SipDialogMgr.h>
#include <net/SipSubscriptionMgr.h>
#include <net/SipSubscribeClient.h>
#include <net/SipSubscribeServer.h>
#include <net/SipPublishContentMgr.h>

#define UNIT_TEST_SIP_PORT 44446

/**
 * Unittest for SipSubscribeClientMgr
 */

class SipSubscribeClientMgr : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(SipSubscribeClientMgr);
    CPPUNIT_TEST(subscribeMwiClientTest);
    CPPUNIT_TEST_SUITE_END();

    public:

    // Notify effected states:
    static int smNumClientNotifiesReceived;
    static SipMessage* smLastClientNotifyReceived;
    static UtlString smClientNotifyEarlyDialog;
    static UtlString smClientNotifyEstablishedDialog;
    static void notifyCallback(const char* earlyDialogHandle,
                         const char* dialogHandle,
                         void* applicationData,
                         const SipMessage* notifyRequest)
    {
        //printf("notifyCallback: %d\n", smNumClientNotifiesReceived);
        smNumClientNotifiesReceived++;
        if(smLastClientNotifyReceived)
        {
            // Don't delete as we take the pointer rather than copy
            // in the unit tests.
            //delete smLastClientNotifyReceived;
        }
        smLastClientNotifyReceived = new SipMessage(*notifyRequest);
        smClientNotifyEarlyDialog = earlyDialogHandle;
        smClientNotifyEstablishedDialog = dialogHandle;
//        fprintf(stderr, "notifyCallback: dialogHandle = '%s'\n", dialogHandle);
    };

    // Subscribe effected states
    static int smNumClientSubResponsesReceived;
    static long smClientExpiration;
    static UtlString smClientSubEarlyDialog;
    static UtlString smClientSubEstablishedDialog;
    static SipMessage* smLastClientSubResponseReceived;
    static void subStateCallback(SipSubscribeClient::SubscriptionState newState,
                          const char* earlyDialogHandle,
                          const char* dialogHandle,
                          void* applicationData,
                          int responseCode,
                          const char* responseText,
                          long expiration,
                          const SipMessage* subscribeResponse)
    {
        //printf("subStateCallback: %d\n", smNumClientSubResponsesReceived);
        smNumClientSubResponsesReceived++;
        smClientExpiration = expiration;
        if(smLastClientSubResponseReceived)
        {
            // Don't delete as we take the pointer rather than copy in the
            // unit tests
            //delete smLastClientSubResponseReceived;
        }
        if(subscribeResponse)
        {
            smLastClientSubResponseReceived = new SipMessage(*subscribeResponse);
        }
        smClientSubEarlyDialog = earlyDialogHandle;
        smClientSubEstablishedDialog = dialogHandle;
//        fprintf(stderr, "subStateCallback: dialogHandle = '%s'\n", dialogHandle);
    };

    UtlBoolean removeMessage(OsMsgQ& messageQueue,
                             int waitMilliSeconds,
                             const SipMessage*& message)
    {
        UtlBoolean gotMessage = FALSE;
        message = NULL;
        OsTime messageTimeout(0, waitMilliSeconds * 1000);
        OsMsg* osMessage = NULL;
        messageQueue.receive(osMessage, messageTimeout);
        if(osMessage)
        {
            int msgType = osMessage->getMsgType();
            int msgSubType = osMessage->getMsgSubType();
            int messageType = ((SipMessageEvent*)osMessage)->getMessageStatus();
            if(msgType == OsMsg::PHONE_APP &&
               msgSubType == SipMessage::NET_SIP_MESSAGE &&
               messageType == SipMessageEvent::APPLICATION)
            {
                message = ((SipMessageEvent*)osMessage)->getMessage();
                gotMessage = TRUE;

#ifdef TEST_PRINT
                if(message)
                {
                    UtlString messageBytes;
                    int len;
                    message->getBytes(&messageBytes, &len);
                    printf("%s", messageBytes.data());
                }
                else
                {
                    printf("removeMessage: messageBytes: <null>\n");
                }
#endif
            }
        }
        return(gotMessage);
    }

    void subscribeMwiClientTest()
    {
        smClientExpiration = -1;
        smNumClientNotifiesReceived = 0;
        smLastClientNotifyReceived = NULL;
        smNumClientSubResponsesReceived = 0;
        smLastClientSubResponseReceived = NULL;

        UtlString hostPort;
        OsSocket::getHostIp(&hostPort);
        hostPort.append(':');
        char portText[20];
        sprintf(portText, "%d", UNIT_TEST_SIP_PORT);
        hostPort.append(portText);

        UtlString resourceId("111@");
        UtlString eventTypeKey("message-summary");
        UtlString eventType(eventTypeKey);
        UtlString from("Frida<sip:111@");
        UtlString to("Tia<sip:222@");
        UtlString contact("sip:111@");

        resourceId.append(hostPort);
        from.append(hostPort);
        from.append('>');
        to.append(hostPort);
        to.append('>');
        contact.append(hostPort);
        SipUserAgent userAgent(UNIT_TEST_SIP_PORT, UNIT_TEST_SIP_PORT);
        userAgent.start();

        // Set up the subscribe client
        SipDialogMgr clientDialogMgr;
        SipRefreshManager refreshMgr(userAgent, clientDialogMgr);
        refreshMgr.start();
        SipSubscribeClient subClient(userAgent, clientDialogMgr, refreshMgr);
        subClient.start();

        // Set up the subscribe server
        SipSubscribeServer* subServer = 
           SipSubscribeServer::buildBasicServer(userAgent, 
                                                eventType);
        SipSubscriptionMgr* subMgr = subServer->getSubscriptionMgr(eventType);
        SipDialogMgr* serverDialogMgr = subMgr->getDialogMgr();
        SipPublishContentMgr* contentMgr = subServer->getPublishMgr(eventType);
        HttpBody* preexistingBodyPtr = NULL;
        UtlBoolean isDefaultContent;

        subServer->start();
        // Enable the handler for the MWI server
        subServer->enableEventType(eventType, &userAgent);

        // Create a crude Subscription server/observer
        OsMsgQ incomingServerMsgQueue;
        // Register an interest in SUBSCRIBE requests 
        // for this event type
        userAgent.addMessageObserver(incomingServerMsgQueue,
                                    SIP_SUBSCRIBE_METHOD,
                                    TRUE, // requests
                                    FALSE, // no reponses
                                    TRUE, // incoming
                                    FALSE, // no outgoing
                                    eventType,
                                    NULL,
                                    NULL);

        OsMsgQ incomingClientMsgQueue;
        userAgent.addMessageObserver(incomingClientMsgQueue,
                                    SIP_SUBSCRIBE_METHOD,
                                    FALSE, // no requests
                                    TRUE, // reponses
                                    TRUE, // incoming
                                    FALSE, // no outgoing
                                    eventType,
                                    NULL,
                                    NULL);

        // Should not be any pre-existing content
        CPPUNIT_ASSERT(!contentMgr->getContent(resourceId, 
                                               eventTypeKey, 
                                               eventType, 
                                               NULL, 
                                               preexistingBodyPtr, 
                                               isDefaultContent));
        int numDefaultContent = -1;
        int numDefaultConstructor = -1;
        int numResourceSpecificContent = -1;
        int numCallbacksRegistered = -1;
        contentMgr->getStats(numDefaultContent,
                             numDefaultConstructor,
                             numResourceSpecificContent,
                             numCallbacksRegistered);
        CPPUNIT_ASSERT(numDefaultContent == 0);
        CPPUNIT_ASSERT(numDefaultConstructor == 0);
        CPPUNIT_ASSERT(numResourceSpecificContent == 0);
        CPPUNIT_ASSERT(numCallbacksRegistered == 1);

        // Create a subscribe request, send it and keep it refreshed
        UtlString earlyDialogHandle;
        CPPUNIT_ASSERT(subClient.addSubscription(resourceId,
                                                  eventType,
                                                  NULL,
                                                  from,
                                                  to,
                                                  contact,
                                                  60, // seconds expiration
                                                  this,
                                                  subStateCallback,
                                                  notifyCallback,
                                                  earlyDialogHandle));


        contentMgr->getStats(numDefaultContent,
                             numDefaultConstructor,
                             numResourceSpecificContent,
                             numCallbacksRegistered);
        CPPUNIT_ASSERT(numDefaultContent == 0);
        CPPUNIT_ASSERT(numDefaultConstructor == 0);
        CPPUNIT_ASSERT(numResourceSpecificContent == 0);
        CPPUNIT_ASSERT(numCallbacksRegistered == 1);

        // See if a subscribe was sent and received
       const SipMessage* serverSideSubRequest = NULL;
       CPPUNIT_ASSERT(removeMessage(incomingServerMsgQueue,
                     5000, // milliseconds
                     serverSideSubRequest));
       CPPUNIT_ASSERT(serverSideSubRequest); // Sub request got to server

       const SipMessage* clientSideSubResponse = NULL;
       CPPUNIT_ASSERT(removeMessage(incomingClientMsgQueue,
                      5000, // milliseconds
                      clientSideSubResponse));
       CPPUNIT_ASSERT(clientSideSubResponse);

        int waitIterations = 0;
        while(smLastClientNotifyReceived == NULL ||
            smLastClientSubResponseReceived == NULL)
        {
            OsTask::delay(100);
            waitIterations++;
            if(waitIterations >= 100)
            {
                break;
            }
        }

        CPPUNIT_ASSERT(smLastClientSubResponseReceived);
        CPPUNIT_ASSERT(smLastClientNotifyReceived);
        SipMessage* firstSubResponse = smLastClientSubResponseReceived;
        smLastClientSubResponseReceived = NULL;
        int firstSubCseq;
        firstSubResponse->getCSeqField(&firstSubCseq, NULL);
        SipMessage* firstNotifyRequest = smLastClientNotifyReceived;
        smLastClientNotifyReceived = NULL;
        int firstNotifyCseq;
        firstNotifyRequest->getCSeqField(&firstNotifyCseq, NULL);
        CPPUNIT_ASSERT(firstSubCseq == 1);
        CPPUNIT_ASSERT(firstNotifyCseq == 1);

        // The refresh manager should re-SUBSCRIBE
        // Wait for the next notify request and subscribe response
        int secondMessageWait = 60;
        int resendTimeout = 0.55 * secondMessageWait;
        if(resendTimeout < 40)
        {
            resendTimeout = 40;
        }
        for(int i = 0; i < secondMessageWait - 1; i++)
        {
            if(i == resendTimeout - 1)
            {
                printf("v");
            }
            else
            {
                printf("=");
            }
        }
        printf("v\n");
        fflush(stdout);
        SipMessage* secondSubResponse = NULL;
        SipMessage* secondNotifyRequest = NULL;

        while(secondNotifyRequest == NULL ||
            secondSubResponse == NULL)
        {
            OsTask::delay(1000);
            if(smLastClientSubResponseReceived)
            {
                secondSubResponse = smLastClientSubResponseReceived;
                smLastClientSubResponseReceived = NULL;
            }
            if(smLastClientNotifyReceived)
            {
                secondNotifyRequest = smLastClientNotifyReceived;
                smLastClientNotifyReceived = NULL;
            }
            printf(".");
            fflush(stdout);
            waitIterations++;
            if(waitIterations >= secondMessageWait)
            {
                printf("\n");
                break;
            }
        }

       CPPUNIT_ASSERT(removeMessage(incomingServerMsgQueue,
                     5000, // milliseconds
                     serverSideSubRequest));
       CPPUNIT_ASSERT(serverSideSubRequest); // Sub request got to server
       CPPUNIT_ASSERT(removeMessage(incomingClientMsgQueue,
                      5000, // milliseconds
                      clientSideSubResponse));
       CPPUNIT_ASSERT(clientSideSubResponse); // Sub respon got to client
        CPPUNIT_ASSERT(secondNotifyRequest);
        // Unregister the queues so we stop receiving messages on them
        userAgent.removeMessageObserver(incomingServerMsgQueue);
        userAgent.removeMessageObserver(incomingClientMsgQueue);

        subClient.requestShutdown();
        refreshMgr.requestShutdown();

        userAgent.shutdown(TRUE);
        OsTask::delay(1000);   // 1 second to let other threads clean up
        delete subServer;
    }
};

int SipSubscribeClientMgr::smNumClientNotifiesReceived;
SipMessage* SipSubscribeClientMgr::smLastClientNotifyReceived;
UtlString SipSubscribeClientMgr::smClientNotifyEarlyDialog;
UtlString SipSubscribeClientMgr::smClientNotifyEstablishedDialog;
int SipSubscribeClientMgr::smNumClientSubResponsesReceived;
long SipSubscribeClientMgr::smClientExpiration;
UtlString SipSubscribeClientMgr::smClientSubEarlyDialog;
UtlString SipSubscribeClientMgr::smClientSubEstablishedDialog;
SipMessage* SipSubscribeClientMgr::smLastClientSubResponseReceived;

CPPUNIT_TEST_SUITE_REGISTRATION(SipSubscribeClientMgr);
