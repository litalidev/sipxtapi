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

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>
#include <sipxunit/TestUtilities.h>
#include <utl/UtlHashMap.h>

#include <os/OsDefs.h>
#include <net/SipMessage.h>
#include <net/SipUserAgent.h>

/**
* Unittest for SipMessage
*/
class SipMessageTest : public CppUnit::TestCase
{
   CPPUNIT_TEST_SUITE(SipMessageTest);
   CPPUNIT_TEST(testGetRAck);
   CPPUNIT_TEST(testGetRSeq);
   CPPUNIT_TEST(testGetReasonField);
   CPPUNIT_TEST(testGetJoinField);
   CPPUNIT_TEST(testGetSessionExpires);
   CPPUNIT_TEST(testGetMinSe);
   CPPUNIT_TEST(testGetVia);
   CPPUNIT_TEST(testGetViaShort);
   CPPUNIT_TEST(testGetAddrVia);
   CPPUNIT_TEST(testGetNoBranchVia);
   CPPUNIT_TEST(testGetViaPort);
   CPPUNIT_TEST(testGetViaFieldSubField);
   CPPUNIT_TEST(testGetEventField);
   CPPUNIT_TEST(testGetToAddress);
   CPPUNIT_TEST(testGetFromAddress);
   CPPUNIT_TEST(testGetResponseSendAddress);
   CPPUNIT_TEST(testParseAddressFromUriPort);
   CPPUNIT_TEST(testProbPort);
   //CPPUNIT_TEST(testMultipartBody);
   CPPUNIT_TEST(testSdpParse);
   CPPUNIT_TEST(testSdpShortHeaderNames);
   CPPUNIT_TEST(testNonSdpSipMessage);
   CPPUNIT_TEST(testSetInviteDataHeaders);
   CPPUNIT_TEST(testSetInviteDataHeadersUnique);
   CPPUNIT_TEST(testSetInviteDataHeadersForbidden);
   CPPUNIT_TEST(testCompactNames);
   CPPUNIT_TEST(testHeaderFieldAccessors);
   CPPUNIT_TEST(testApplyTargetUriHeaderParams);
   CPPUNIT_TEST_SUITE_END();

public:

   void testGetRAck()
   {
      const char* simpleMessage =
         "PRACK sip:65681@testserver.com SIP/2.0\r\n"
         "To: <sip:65681@testserver.com>;tag=985624-1623\r\n"
         "From: <sip:10.21.128.204>;tag=15039611-4B5\r\n"
         "Call-Id: 55147C1E-F6ED11D9-80E3EC05-47D61469@10.21.128.204\r\n"
         "Via: SIP/2.0/UDP  10.21.128.204:5060;branch=z9hG4bK9E8\r\n"
         "Date: Mon, 18 Jul 2005 18:05:17 GMT\r\n"
         "Supported: 100rel,timer\r\n"
         "Allow: INVITE, OPTIONS, BYE, CANCEL, ACK, PRACK, COMET, REFER, SUBSCRIBE, NOTIFY, INFO, UPDATE, REGISTER\r\n"
         "Cseq: 101 PRACK\r\n"
         "RAck: 776656 100 INVITE\r\n"
         "Max-Forwards: 9\r\n"
         "Contact: <sip:10.21.128.204:5060>\r\n"
         "Expires: 180\r\n"
         "Mime-Version: 1.0\r\n"
         "Content-Length: 0\r\n"
         "\r\n";
      SipMessage testMsg(simpleMessage);
      int rsequence;
      int csequence;
      UtlString method;
      // test getting
      testMsg.getRAckField(rsequence, csequence, method);
      CPPUNIT_ASSERT_EQUAL(776656, rsequence);
      CPPUNIT_ASSERT_EQUAL(100, csequence);
      ASSERT_STR_EQUAL("INVITE", method.data());
      // now try changing
      testMsg.setRAckField(1, 2, "BADMETHOD");
      // now verify
      testMsg.getRAckField(rsequence, csequence, method);
      CPPUNIT_ASSERT_EQUAL(1, rsequence);
      CPPUNIT_ASSERT_EQUAL(2, csequence);
      ASSERT_STR_EQUAL("BADMETHOD", method.data());
   }

   void testGetRSeq()
   {
      const char* simpleMessage =
         "SIP/2.0 180 Ringing\r\n"
         "To: <sip:10.21.128.204>;tag=15039611-4B5\r\n"
         "From: <sip:65681@testserver.com>;tag=985624-1623\r\n"
         "Call-Id: 55147C1E-F6ED11D9-80E3EC05-47D61469@10.21.128.204\r\n"
         "Require: 100rel\r\n"
         "Allow: INVITE, OPTIONS, BYE, CANCEL, ACK, PRACK, COMET, REFER, SUBSCRIBE, NOTIFY, INFO, UPDATE, REGISTER\r\n"
         "RSeq: 1\r\n"
         "Cseq: 100 INVITE\r\n"
         "Contact: <sip:10.21.128.204:5060>\r\n"
         "Content-Length: 0\r\n"
         "\r\n";
      SipMessage testMsg(simpleMessage);
      int rsequence;
      testMsg.getRSeqField(rsequence);
      CPPUNIT_ASSERT_EQUAL(1, rsequence);
      // now try changing
      testMsg.setRSeqField(25242);
      testMsg.getRSeqField(rsequence);
      CPPUNIT_ASSERT_EQUAL(25242, rsequence);
   }

   void testGetReasonField()
   {
      const char* simpleMessage =
         "CANCEL sip:65681@testserver.com SIP/2.0\r\n"
         "To: <sip:65681@testserver.com>;tag=985624-1623\r\n"
         "From: <sip:10.21.128.204>;tag=15039611-4B5\r\n"
         "Call-Id: 55147C1E-F6ED11D9-80E3EC05-47D61469@10.21.128.204\r\n"
         "Via: SIP/2.0/UDP  10.21.128.204:5060;branch=z9hG4bK9E8\r\n"
         "Date: Mon, 18 Jul 2005 18:05:17 GMT\r\n"
         "Supported: 100rel,timer\r\n"
         "Allow: INVITE, OPTIONS, BYE, CANCEL, ACK, PRACK, COMET, REFER, SUBSCRIBE, NOTIFY, INFO, UPDATE, REGISTER\r\n"
         "Cseq: 101 CANCEL\r\n"
         "Max-Forwards: 9\r\n"
         "Reason: SIP ;cause=200 ;text=\"Call completed elsewhere\", Q.850 ;cause=16 ;text=\"Terminated\"\r\n"
         "Contact: <sip:10.21.128.204:5060>\r\n"
         "Expires: 180\r\n"
         "Mime-Version: 1.0\r\n"
         "Content-Length: 0\r\n"
         "\r\n";
      SipMessage testMsg(simpleMessage);
      int cause;
      UtlString protocol;
      UtlString text;
      // test getting 1st value
      testMsg.getReasonField(0, protocol, cause, text);
      ASSERT_STR_EQUAL("SIP", protocol.data());
      CPPUNIT_ASSERT_EQUAL(200, cause);
      ASSERT_STR_EQUAL("Call completed elsewhere", text.data());
      // test getting 2nd value
      cause = 0;
      protocol.remove(0);
      text.remove(0);
      testMsg.getReasonField(1, protocol, cause, text);
      ASSERT_STR_EQUAL("Q.850", protocol.data());
      CPPUNIT_ASSERT_EQUAL(16, cause);
      ASSERT_STR_EQUAL("Terminated", text.data());
   }

   void testGetJoinField()
   {
      const char* simpleMessage =
         "INVITE sip:65681@testserver.com SIP/2.0\r\n"
         "To: <sip:65681@testserver.com>;tag=985624-1623\r\n"
         "From: <sip:10.21.128.204>;tag=15039611-4B5\r\n"
         "Call-Id: 55147C1E-F6ED11D9-80E3EC05-47D61469@10.21.128.204\r\n"
         "Via: SIP/2.0/UDP  10.21.128.204:5060;branch=z9hG4bK9E8\r\n"
         "Date: Mon, 18 Jul 2005 18:05:17 GMT\r\n"
         "Supported: 100rel,timer\r\n"
         "Allow: INVITE, OPTIONS, BYE, CANCEL, ACK, PRACK, COMET, REFER, SUBSCRIBE, NOTIFY, INFO, UPDATE, REGISTER\r\n"
         "Cseq: 101 INVITE\r\n"
         "Max-Forwards: 9\r\n"
         "Contact: <sip:10.21.128.204:5060>\r\n"
         "Session-Expires: 4000;refresher=uac\r\n"
         "Join: 12adf2f34456gs5;to-tag=12345;from-tag=54321\r\n"
         "Expires: 180\r\n"
         "Mime-Version: 1.0\r\n"
         "Content-Length: 0\r\n"
         "\r\n";
      SipMessage testMsg(simpleMessage);
      UtlString callId;
      UtlString fromTag;
      UtlString toTag;
      // test getting
      testMsg.getJoinField(callId, fromTag, toTag);
      ASSERT_STR_EQUAL("12adf2f34456gs5", callId.data());
      ASSERT_STR_EQUAL("12345", toTag.data());
      ASSERT_STR_EQUAL("54321", fromTag.data());
      // test setting and getting
      callId = "iughx438xg3m762gxfius";
      fromTag = "7652663";
      toTag = "8572451";
      testMsg.setJoinField(callId, fromTag, toTag);
      callId.remove(0);
      fromTag.remove(0);
      toTag.remove(0);
      // now get again and test
      testMsg.getJoinField(callId, fromTag, toTag);
      ASSERT_STR_EQUAL("iughx438xg3m762gxfius", callId.data());
      ASSERT_STR_EQUAL("8572451", toTag.data());
      ASSERT_STR_EQUAL("7652663", fromTag.data());
   }
   void testGetSessionExpires()
   {
      const char* simpleMessage =
         "INVITE sip:65681@testserver.com SIP/2.0\r\n"
         "To: <sip:65681@testserver.com>;tag=985624-1623\r\n"
         "From: <sip:10.21.128.204>;tag=15039611-4B5\r\n"
         "Call-Id: 55147C1E-F6ED11D9-80E3EC05-47D61469@10.21.128.204\r\n"
         "Via: SIP/2.0/UDP  10.21.128.204:5060;branch=z9hG4bK9E8\r\n"
         "Date: Mon, 18 Jul 2005 18:05:17 GMT\r\n"
         "Supported: 100rel,timer\r\n"
         "Allow: INVITE, OPTIONS, BYE, CANCEL, ACK, PRACK, COMET, REFER, SUBSCRIBE, NOTIFY, INFO, UPDATE, REGISTER\r\n"
         "Cseq: 101 INVITE\r\n"
         "Max-Forwards: 9\r\n"
         "Contact: <sip:10.21.128.204:5060>\r\n"
         "Session-Expires: 4000;refresher=uac\r\n"
         "Expires: 180\r\n"
         "Mime-Version: 1.0\r\n"
         "Content-Length: 0\r\n"
         "\r\n";
      SipMessage testMsg(simpleMessage);

      int sessionExpiresSeconds;
      UtlString refresher;
      // test getting
      testMsg.getSessionExpires(&sessionExpiresSeconds, &refresher);
      CPPUNIT_ASSERT_EQUAL(4000, sessionExpiresSeconds);
      ASSERT_STR_EQUAL("uac", refresher.data());
      // try changing
      testMsg.setSessionExpires(120, "uas");
      sessionExpiresSeconds = -1;
      refresher.remove(0);
      // test again
      testMsg.getSessionExpires(&sessionExpiresSeconds, &refresher);
      CPPUNIT_ASSERT_EQUAL(120, sessionExpiresSeconds);
      ASSERT_STR_EQUAL("uas", refresher.data());
   }

   void testGetMinSe()
   {
      const char* simpleMessage =
         "INVITE sip:65681@testserver.com SIP/2.0\r\n"
         "To: <sip:65681@testserver.com>;tag=985624-1623\r\n"
         "From: <sip:10.21.128.204>;tag=15039611-4B5\r\n"
         "Call-Id: 55147C1E-F6ED11D9-80E3EC05-47D61469@10.21.128.204\r\n"
         "Via: SIP/2.0/UDP  10.21.128.204:5060;branch=z9hG4bK9E8\r\n"
         "Date: Mon, 18 Jul 2005 18:05:17 GMT\r\n"
         "Supported: 100rel,timer\r\n"
         "Allow: INVITE, OPTIONS, BYE, CANCEL, ACK, PRACK, COMET, REFER, SUBSCRIBE, NOTIFY, INFO, UPDATE, REGISTER\r\n"
         "Cseq: 101 INVITE\r\n"
         "Max-Forwards: 9\r\n"
         "Contact: <sip:10.21.128.204:5060>\r\n"
         "Session-Expires: 4000;refresher=uac\r\n"
         "Min-SE: 200\r\n"
         "Expires: 180\r\n"
         "Mime-Version: 1.0\r\n"
         "Content-Length: 0\r\n"
         "\r\n";
      SipMessage testMsg(simpleMessage);

      int minSe;
      // test getting
      testMsg.getMinSe(minSe);
      CPPUNIT_ASSERT_EQUAL(200, minSe);
      // try changing
      testMsg.setMinSe(120);
      minSe = -1;
      // test again
      testMsg.getMinSe(minSe);
      CPPUNIT_ASSERT_EQUAL(120, minSe);
   }

   void testGetVia()
   {
      const char* SimpleMessage =
         "REGISTER sip:sipx.local SIP/2.0\r\n"
         "Via: SIP/2.0/TCP sipx.local:33855;branch=z9hG4bK-10cb6f9378a12d4218e10ef4dc78ea3d\r\n"
         "To: sip:sipx.local\r\n"
         "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
         "Cseq: 1 REGISTER\r\n"
         "Max-Forwards: 20\r\n"
         "User-Agent: sipsend/0.01\r\n"
         "Contact: me@127.0.0.1\r\n"
         "Expires: 300\r\n"
         "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
         "Content-Length: 0\r\n"
         "\r\n";
      SipMessage testMsg( SimpleMessage, strlen( SimpleMessage ) );

      UtlString viaAddress;
      int viaPort;
      UtlString protocol;
      int recievedPort;
      UtlBoolean receivedSet;
      UtlBoolean maddrSet;
      UtlBoolean receivePortSet;

      testMsg.getLastVia(&viaAddress,
         &viaPort,
         &protocol,
         &recievedPort,
         &receivedSet,
         &maddrSet,
         &receivePortSet);

      ASSERT_STR_EQUAL("sipx.local",viaAddress.data());
      CPPUNIT_ASSERT_EQUAL(33855, viaPort);
      ASSERT_STR_EQUAL("TCP",protocol.data());

   };

   void testGetViaShort()
   {
      const char* SimpleMessage =
         "REGISTER sip:sipx.local SIP/2.0\r\n"
         "v: SIP/2.0/TCP sipx.local:33855;branch=z9hG4bK-9378a12d4218e10ef4dc78ea3d\r\n"
         "v: SIP/2.0/UDP sipx.remote:9999;branch=z9hG4bK-10cb6f\r\n"
         "To: sip:sipx.local\r\n"
         "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
         "Cseq: 1 REGISTER\r\n"
         "Max-Forwards: 20\r\n"
         "User-Agent: sipsend/0.01\r\n"
         "Contact: me@127.0.0.1\r\n"
         "Expires: 300\r\n"
         "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
         "Content-Length: 0\r\n"
         "\r\n";
      SipMessage testMsg( SimpleMessage, strlen( SimpleMessage ) );

      UtlString viaAddress;
      int viaPort;
      UtlString protocol;
      int recievedPort;
      UtlBoolean receivedSet;
      UtlBoolean maddrSet;
      UtlBoolean receivePortSet;

      testMsg.removeLastVia();

      testMsg.getLastVia(&viaAddress,
         &viaPort,
         &protocol,
         &recievedPort,
         &receivedSet,
         &maddrSet,
         &receivePortSet);

      ASSERT_STR_EQUAL("sipx.remote",viaAddress.data());
      CPPUNIT_ASSERT_EQUAL(9999, viaPort);
      ASSERT_STR_EQUAL("UDP",protocol.data());

   };


   void testGetAddrVia()
   {
      const char* SimpleMessage =
         "REGISTER sip:sipx.local SIP/2.0\r\n"
         "Via: SIP/2.0/TCP 10.1.1.3:33855;branch=z9hG4bK-10cb6f9378a12d4218e10ef4dc78ea3d\r\n"
         "To: sip:sipx.local\r\n"
         "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
         "Cseq: 1 REGISTER\r\n"
         "Max-Forwards: 20\r\n"
         "User-Agent: sipsend/0.01\r\n"
         "Contact: me@127.0.0.1\r\n"
         "Expires: 300\r\n"
         "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
         "Content-Length: 0\r\n"
         "\r\n";
      SipMessage testMsg( SimpleMessage, strlen( SimpleMessage ) );

      UtlString viaAddress;
      int viaPort;
      UtlString protocol;
      int recievedPort;
      UtlBoolean receivedSet;
      UtlBoolean maddrSet;
      UtlBoolean receivePortSet;

      testMsg.getLastVia(&viaAddress,
         &viaPort,
         &protocol,
         &recievedPort,
         &receivedSet,
         &maddrSet,
         &receivePortSet);

      ASSERT_STR_EQUAL("10.1.1.3",viaAddress.data());
      CPPUNIT_ASSERT_EQUAL(33855, viaPort);
      ASSERT_STR_EQUAL("TCP",protocol.data());

   };

   void testGetNoBranchVia()
   {
      const char* SimpleMessage =
         "REGISTER sip:sipx.local SIP/2.0\r\n"
         "Via: SIP/2.0/TCP 10.1.1.3:33855\r\n"
         "To: sip:sipx.local\r\n"
         "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
         "Cseq: 1 REGISTER\r\n"
         "Max-Forwards: 20\r\n"
         "User-Agent: sipsend/0.01\r\n"
         "Contact: me@127.0.0.1\r\n"
         "Expires: 300\r\n"
         "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
         "Content-Length: 0\r\n"
         "\r\n";
      SipMessage testMsg( SimpleMessage, strlen( SimpleMessage ) );

      UtlString viaAddress;
      int viaPort;
      UtlString protocol;
      int recievedPort;
      UtlBoolean receivedSet;
      UtlBoolean maddrSet;
      UtlBoolean receivePortSet;

      testMsg.getLastVia(&viaAddress,
         &viaPort,
         &protocol,
         &recievedPort,
         &receivedSet,
         &maddrSet,
         &receivePortSet);

      ASSERT_STR_EQUAL("10.1.1.3",viaAddress.data());
      CPPUNIT_ASSERT_EQUAL(33855, viaPort);
      ASSERT_STR_EQUAL("TCP",protocol.data());

   };

   void testGetViaPort()
   {
      struct test {
         const char* string;         // Input string.
         int port;                   // Expected returned viaPort.
         UtlBoolean rportSet;        // Expected returned receivedPortSet.
         int rport;                  // Expected returned receivedPort.
      };

      struct test tests[] = {
         { "sip:foo@bar", PORT_NONE, 0, PORT_NONE },
         { "sip:foo@bar:5060", 5060, 0, PORT_NONE },
         { "sip:foo@bar:1", 1, 0, PORT_NONE },
         { "sip:foo@bar:100", 100, 0, PORT_NONE },
         { "sip:foo@bar:65535", 65535, 0, PORT_NONE },
         { "sip:foo@bar;rport=1", PORT_NONE, 1, 1 },
         { "sip:foo@bar:5060;rport=1", 5060, 1, 1 },
         { "sip:foo@bar:1;rport=1", 1, 1, 1 },
         { "sip:foo@bar:100;rport=1", 100, 1, 1 },
         { "sip:foo@bar:65535;rport=1", 65535, 1, 1 },
         { "sip:foo@bar;rport=100", PORT_NONE, 1, 100 },
         { "sip:foo@bar:5060;rport=100", 5060, 1, 100 },
         { "sip:foo@bar:1;rport=100", 1, 1, 100 },
         { "sip:foo@bar:100;rport=100", 100, 1, 100 },
         { "sip:foo@bar:65535;rport=100", 65535, 1, 100 },
         { "sip:foo@bar;rport=5060", PORT_NONE, 1, 5060 },
         { "sip:foo@bar:5060;rport=5060", 5060, 1, 5060 },
         { "sip:foo@bar:1;rport=5060", 1, 1, 5060 },
         { "sip:foo@bar:100;rport=5060", 100, 1, 5060 },
         { "sip:foo@bar:65535;rport=5060", 65535, 1, 5060 },
         { "sip:foo@bar;rport=65535", PORT_NONE, 1, 65535 },
         { "sip:foo@bar:5060;rport=65535", 5060, 1, 65535 },
         { "sip:foo@bar:1;rport=65535", 1, 1, 65535 },
         { "sip:foo@bar:100;rport=65535", 100, 1, 65535 },
         { "sip:foo@bar:65535;rport=65535", 65535, 1, 65535 }
      };

      // Buffer to compose message.
      char message[1000];

      // Message templates into which to insert addresses.
      // Template has at least 2 Via's, to make sure the function is looking
      // at the right Via.
      const char* message_template =
         "REGISTER sip:sipx.local SIP/2.0\r\n"
         "Via: SIP/2.0/TCP %s;branch=z9hG4bK-foobarbazquux\r\n"
         "Via: SIP/2.0/TCP sipx.local:33855;branch=z9hG4bK-10cb6f9378a12d4218e10ef4dc78ea3d\r\n"
         "To: sip:sipx.local\r\n"
         "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
         "Cseq: 1 REGISTER\r\n"
         "Max-Forwards: 20\r\n"
         "User-Agent: sipsend/0.01\r\n"
         "Contact: me@127.0.0.1\r\n"
         "Expires: 300\r\n"
         "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
         "Content-Length: 0\r\n"
         "\r\n";

      UtlString viaAddress;
      int viaPort;
      UtlString protocol;
      int receivedPort;
      UtlBoolean receivedSet;
      UtlBoolean maddrSet;
      UtlBoolean receivedPortSet;

      for (unsigned int i = 0; i < sizeof (tests) / sizeof (tests[0]);
         i++)
      {
         // Compose the message.
         sprintf(message, message_template, tests[i].string);
         SipMessage sipMessage(message, strlen(message));

         sipMessage.getLastVia(&viaAddress,
            &viaPort,
            &protocol,
            &receivedPort,
            &receivedSet,
            &maddrSet,
            &receivedPortSet);
         CPPUNIT_ASSERT_EQUAL_MESSAGE(tests[i].string, tests[i].port,
            viaPort);
         CPPUNIT_ASSERT_EQUAL_MESSAGE(tests[i].string, tests[i].rportSet,
            receivedPortSet);
         CPPUNIT_ASSERT_EQUAL_MESSAGE(tests[i].string, tests[i].rport,
            receivedPort);
      }
   }

   void testMultipartBody()
   {
      const char* MultipartBodyMessage =
         "INVITE sip:65681@testserver.com SIP/2.0\r\n"
         "Record-Route: <sip:172.20.26.36:5080;lr;a;t=15039611-4B5;s=5fc190d408cc09d245d115792f6d61e1>\r\n"
         "Via: SIP/2.0/UDP 172.20.26.36:5080;branch=z9hG4bK-826b994f3fa1136ea6da35868d05fcbb\r\n"
         "Via: SIP/2.0/TCP 172.20.26.36;branch=z9hG4bK-a45e8e1a92501d6d29854307651741b3\r\n"
         "Via: SIP/2.0/UDP  10.21.128.204:5060;branch=z9hG4bK9E8\r\n"
         "From: <sip:10.21.128.204>;tag=15039611-4B5\r\n"
         "To: <sip:65681@testserver.com>\r\n"
         "Date: Mon, 18 Jul 2005 18:05:17 GMT\r\n"
         "Call-Id: 55147C1E-F6ED11D9-80E3EC05-47D61469@10.21.128.204\r\n"
         "Supported: 100rel,timer\r\n"
         "Min-Se: 1800\r\n"
         "Cisco-Guid: 1427005942-4142731737-2150891535-615471488\r\n"
         "User-Agent: Cisco-SIPGateway/IOS-12.x\r\n"
         "Allow: INVITE, OPTIONS, BYE, CANCEL, ACK, PRACK, COMET, REFER, SUBSCRIBE, NOTIFY, INFO, UPDATE, REGISTER\r\n"
         "Cseq: 101 INVITE\r\n"
         "Max-Forwards: 9\r\n"
         "Timestamp: 1121709917\r\n"
         "Contact: <sip:10.21.128.204:5060>\r\n"
         "Expires: 180\r\n"
         "Allow-Events: telephone-event\r\n"
         "Mime-Version: 1.0\r\n"
         "Content-Type: multipart/mixed;boundary=uniqueBoundary\r\n"
         "Content-Length: 561\r\n"
         "\r\n"
         "--uniqueBoundary\r\n"
         "Content-Type: application/sdp\r\n"
         "\r\n"
         "v=0\r\n"
         "o=CiscoSystemsSIP-GW-UserAgent 9773 1231 IN IP4 10.21.128.204\r\n"
         "s=SIP Call\r\n"
         "c=IN IP4 10.21.128.204\r\n"
         "t=0 0\r\n"
         "m=audio 16634 RTP/AVP 0 98\r\n"
         "c=IN IP4 10.21.128.204\r\n"
         "a=rtpmap:0 PCMU/8000\r\n"
         "a=rtpmap:98 telephone-event/8000\r\n"
         "a=fmtp:98 0-16\r\n"
         "a=ptime:20\r\n"
         "--uniqueBoundary\r\n"
         "Content-Type: application/gtd\r\n"
         "Content-Disposition: signal;handling=optional\r\n"
         "\r\n"
         "IAM,\r\n"
         "PRN,isdn*,,NT100,\r\n"
         "USI,rate,c,s,c,1\r\n"
         "USI,lay1,ulaw\r\n"
         "TMR,00\r\n"
         "CPN,04,,1,65681\r\n"
         "CPC,09\r\n"
         "FCI,,,,,,,y,\r\n"
         "GCI,550e61f6f6ed11d98034000f24af5980\r\n"
         "\r\n"
         "--uniqueBoundary--\r\n"
         ;

      const char* correctBody =
         "v=0\r\n"
         "o=CiscoSystemsSIP-GW-UserAgent 9773 1231 IN IP4 10.21.128.204\r\n"
         "s=SIP Call\r\n"
         "c=IN IP4 10.21.128.204\r\n"
         "t=0 0\r\n"
         "m=audio 16634 RTP/AVP 0 98\r\n"
         "c=IN IP4 10.21.128.204\r\n"
         "a=rtpmap:0 PCMU/8000\r\n"
         "a=rtpmap:98 telephone-event/8000\r\n"
         "a=fmtp:98 0-16\r\n"
         "a=ptime:20"
         ;

      SipMessage testMsg( MultipartBodyMessage, strlen( MultipartBodyMessage ) );

      const SdpBody* sdpBody = testMsg.getSdpBody();
      CPPUNIT_ASSERT(sdpBody);

      UtlString theBody;
      int theLength;

      sdpBody->getBytes(&theBody, &theLength);

      ASSERT_STR_EQUAL(correctBody,theBody.data());
   };


   void testGetViaFieldSubField()
   {
      // Test that the getViaFieldSubField method returns the right results,
      // especially when Via values are combined in one header.

      const char* message1 = 
         "SIP/2.0 481 Call Leg/Transaction Does Not Exist\r\n"
         "Via: SIP/2.0/UDP 10.0.11.35:5080;branch=z9hG4bK-80e0607bee4944e9ecb678caae8638d5;received=10.0.11.37,"
         "SIP/2.0/UDP 10.0.11.35;branch=z9hG4bK-379fceb40dc3c5716a3f167d93ceadf4;received=10.0.11.37,"
         "SIP/2.0/UDP 10.0.11.35:5080;branch=z9hG4bK-d05de917f970cd88ea048891ea57f140;received=10.0.11.37,"
         "SIP/2.0/UDP 10.0.11.35;branch=z9hG4bK-09d4d158ad31b82192efa4795b49df90;received=10.0.11.37,"
         "SIP/2.0/UDP 10.0.8.90:5060;branch=z9hG4bK5fa09267\r\n"
         "From: \"joanne brunet\" <sip:245@jaguar.local>;tag=0002fd3bb5770ab64fcc4d65-34791f85\r\n"
         "To: <sip:*4706@jaguar.local>\r\n"
         "Call-ID: 0002fd3b-b5770020-6a00fe74-11feac1a@10.0.8.90\r\n"
         "Date: Wed, 19 Apr 2006 13:19:40 GMT\r\n"
         "CSeq: 101 INVITE\r\n"
         "Content-Length: 0\r\n"
         "\r\n";

      // Same as message1, but with each Via value in a separate header.
      const char* message2 = 
         "SIP/2.0 481 Call Leg/Transaction Does Not Exist\r\n"
         "Via: SIP/2.0/UDP 10.0.11.35:5080;branch=z9hG4bK-80e0607bee4944e9ecb678caae8638d5;received=10.0.11.37\r\n"
         "Via: SIP/2.0/UDP 10.0.11.35;branch=z9hG4bK-379fceb40dc3c5716a3f167d93ceadf4;received=10.0.11.37\r\n"
         "Via: SIP/2.0/UDP 10.0.11.35:5080;branch=z9hG4bK-d05de917f970cd88ea048891ea57f140;received=10.0.11.37\r\n"
         "Via: SIP/2.0/UDP 10.0.11.35;branch=z9hG4bK-09d4d158ad31b82192efa4795b49df90;received=10.0.11.37\r\n"
         "Via: SIP/2.0/UDP 10.0.8.90:5060;branch=z9hG4bK5fa09267\r\n"
         "From: \"joanne brunet\" <sip:245@jaguar.local>;tag=0002fd3bb5770ab64fcc4d65-34791f85\r\n"
         "To: <sip:*4706@jaguar.local>\r\n"
         "Call-ID: 0002fd3b-b5770020-6a00fe74-11feac1a@10.0.8.90\r\n"
         "Date: Wed, 19 Apr 2006 13:19:40 GMT\r\n"
         "CSeq: 101 INVITE\r\n"
         "Content-Length: 0\r\n"
         "\r\n";

      // The Via values.
      const char* (vias[]) = {
         "SIP/2.0/UDP 10.0.11.35:5080;branch=z9hG4bK-80e0607bee4944e9ecb678caae8638d5;received=10.0.11.37",
         "SIP/2.0/UDP 10.0.11.35;branch=z9hG4bK-379fceb40dc3c5716a3f167d93ceadf4;received=10.0.11.37",
         "SIP/2.0/UDP 10.0.11.35:5080;branch=z9hG4bK-d05de917f970cd88ea048891ea57f140;received=10.0.11.37",
         "SIP/2.0/UDP 10.0.11.35;branch=z9hG4bK-09d4d158ad31b82192efa4795b49df90;received=10.0.11.37",
         "SIP/2.0/UDP 10.0.8.90:5060;branch=z9hG4bK5fa09267",
      };

      UtlString value;
      unsigned int i;

      SipMessage testMessage1(message1, strlen(message1));

      for (i = 0;
         i < sizeof (vias) / sizeof (vias[0]) &&
         testMessage1.getViaFieldSubField(&value, i);
      i++)
      {
         char buffer[100];
         sprintf(buffer,
            "testMessage1.getViaFieldSubField(..., %d) == vias[%d]",
            i, i);
         ASSERT_STR_EQUAL_MESSAGE(buffer, vias[i], value.data());
      }
      CPPUNIT_ASSERT_EQUAL(i, (unsigned int) (sizeof (vias) / sizeof (vias[0])));

      SipMessage testMessage2(message2, strlen(message2));

      for (i = 0;
         i < sizeof (vias) / sizeof (vias[0]) &&
         testMessage2.getViaFieldSubField(&value, i);
      i++)
      {
         char buffer[100];
         sprintf(buffer,
            "testMessage2.getViaFieldSubField(..., %d) == vias[%d]",
            i, i);
         ASSERT_STR_EQUAL_MESSAGE(buffer, vias[i], value.data());
      }
      CPPUNIT_ASSERT_EQUAL(i, (unsigned int) (sizeof (vias) / sizeof (vias[0])));
   };


   void testGetEventField()
   {
      UtlString fullEventField;
      UtlString package;
      UtlString id;
      UtlHashMap params;
      UtlString* paramValue;

      const char* SubscribeMessage =
         "SUBSCRIBE sip:sipx.local SIP/2.0\r\n"
         "Via: SIP/2.0/TCP 10.1.1.3:33855\r\n"
         "To: sip:sipx.local\r\n"
         "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
         "Cseq: 1 SUBSCRIBE\r\n"
         "Max-Forwards: 20\r\n"
         "User-Agent: sipsend/0.01\r\n"
         "Contact: me@127.0.0.1\r\n"
         "Expires: 300\r\n"
         "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
         "Event: the-package\r\n"
         "Content-Length: 0\r\n"
         "\r\n";
      SipMessage testMsg( SubscribeMessage, strlen( SubscribeMessage ) );

      // use the raw interface to get the full field value
      CPPUNIT_ASSERT(testMsg.getEventField(fullEventField));
      ASSERT_STR_EQUAL("the-package",fullEventField.data());

      // use the parsing interface, but don't ask for the parameters
      CPPUNIT_ASSERT(testMsg.getEventField(&package));
      ASSERT_STR_EQUAL("the-package",package.data());

      // use the parsing interface and get the parameters (which should be empty)
      CPPUNIT_ASSERT(testMsg.getEventField(&package, &id, &params));
      ASSERT_STR_EQUAL("the-package",package.data());
      CPPUNIT_ASSERT(id.isNull());
      CPPUNIT_ASSERT(params.isEmpty());

      fullEventField.remove(0);
      package.remove(0);

      const char* SubscribeMessageWithId =
         "SUBSCRIBE sip:sipx.local SIP/2.0\r\n"
         "Via: SIP/2.0/TCP 10.1.1.3:33855\r\n"
         "To: sip:sipx.local\r\n"
         "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
         "Cseq: 1 SUBSCRIBE\r\n"
         "Max-Forwards: 20\r\n"
         "User-Agent: sipsend/0.01\r\n"
         "Contact: me@127.0.0.1\r\n"
         "Expires: 300\r\n"
         "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
         "Event: the-package;id=45wwrt2\r\n"
         "Content-Length: 0\r\n"
         "\r\n";
      SipMessage testMsgWithId( SubscribeMessageWithId, strlen( SubscribeMessageWithId ) );

      // use the raw interface to get the full field value
      CPPUNIT_ASSERT(testMsgWithId.getEventField(fullEventField));
      ASSERT_STR_EQUAL("the-package;id=45wwrt2",fullEventField.data());

      // use the parsing interface, but don't ask for the parameters
      CPPUNIT_ASSERT(testMsgWithId.getEventField(&package));
      ASSERT_STR_EQUAL("the-package",package.data());

      // use the parsing interface and get the parameters (which should be empty)
      CPPUNIT_ASSERT(testMsgWithId.getEventField(&package, &id, &params));
      ASSERT_STR_EQUAL("the-package",package.data());
      ASSERT_STR_EQUAL("45wwrt2",id.data());
      CPPUNIT_ASSERT(params.isEmpty());

      fullEventField.remove(0);
      package.remove(0);
      id.remove(0);

      const char* SubscribeMessageWithParams =
         "SUBSCRIBE sip:sipx.local SIP/2.0\r\n"
         "Via: SIP/2.0/TCP 10.1.1.3:33855\r\n"
         "To: sip:sipx.local\r\n"
         "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
         "Cseq: 1 SUBSCRIBE\r\n"
         "Max-Forwards: 20\r\n"
         "User-Agent: sipsend/0.01\r\n"
         "Contact: me@127.0.0.1\r\n"
         "Expires: 300\r\n"
         "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
         "Event: the-package;p1=one;id=45wwrt2;p2=two\r\n"
         "Content-Length: 0\r\n"
         "\r\n";
      SipMessage testMsgWithParams( SubscribeMessageWithParams, strlen( SubscribeMessageWithParams ) );

      // use the raw interface to get the full field value
      CPPUNIT_ASSERT(testMsgWithParams.getEventField(fullEventField));
      ASSERT_STR_EQUAL("the-package;p1=one;id=45wwrt2;p2=two",fullEventField.data());

      // use the parsing interface, but don't ask for the parameters
      CPPUNIT_ASSERT(testMsgWithParams.getEventField(&package));
      ASSERT_STR_EQUAL("the-package",package.data());

      // use the parsing interface and get the parameters (which should have values)
      CPPUNIT_ASSERT(testMsgWithParams.getEventField(&package, &id, &params));
      ASSERT_STR_EQUAL("the-package",package.data());
      ASSERT_STR_EQUAL("45wwrt2",id.data());
      CPPUNIT_ASSERT(params.entries()==2);

      UtlString paramName1("p1");
      CPPUNIT_ASSERT(NULL != (paramValue = dynamic_cast<UtlString*>(params.findValue(&paramName1))));
      ASSERT_STR_EQUAL("one",paramValue->data());

      UtlString paramName2("p2");
      CPPUNIT_ASSERT(NULL != (paramValue = dynamic_cast<UtlString*>(params.findValue(&paramName2))));
      ASSERT_STR_EQUAL("two",paramValue->data());
   }

   void testGetToAddress()
   {
      struct test {
         const char* string;         // Input string.
         int port;                   // Expected returned to-address port.
      };

      struct test tests[] = {
         { "sip:foo@bar", PORT_NONE },
         { "sip:foo@bar:5060", 5060 },
         { "sip:foo@bar:1", 1 },
         { "sip:foo@bar:100", 100 },
         { "sip:foo@bar:65535", 65535 }
      };

      UtlString address;
      int port;
      UtlString protocol;
      UtlString user;
      UtlString userLabel;
      UtlString tag;
      // Buffer to compose message.
      char message[1000];
      // Message template into which to insert the To address.
      const char* message_template =
         "REGISTER sip:sipx.local SIP/2.0\r\n"
         "Via: SIP/2.0/TCP sipx.local:33855;branch=z9hG4bK-10cb6f9378a12d4218e10ef4dc78ea3d\r\n"
         "To: %s\r\n"
         "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
         "Cseq: 1 REGISTER\r\n"
         "Max-Forwards: 20\r\n"
         "User-Agent: sipsend/0.01\r\n"
         "Contact: me@127.0.0.1\r\n"
         "Expires: 300\r\n"
         "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
         "Content-Length: 0\r\n"
         "\r\n";

      for (unsigned int i = 0; i < sizeof (tests) / sizeof (tests[0]);
         i++)
      {
         // Compose the message.
         sprintf(message, message_template, tests[i].string);
         SipMessage sipMessage(message, strlen(message));

         sipMessage.getToAddress(&address, &port, &protocol, &user,
            &userLabel, &tag);
         CPPUNIT_ASSERT_EQUAL_MESSAGE(tests[i].string, tests[i].port, port);
      }
   }

   void testGetFromAddress()
   {
      struct test {
         const char* string;         // Input string.
         int port;                  // Expected returned from-address port.
      };

      struct test tests[] = {
         { "sip:foo@bar", PORT_NONE },
         { "sip:foo@bar:5060", 5060 },
         { "sip:foo@bar:1", 1 },
         { "sip:foo@bar:100", 100 },
         { "sip:foo@bar:65535", 65535 }
      };

      UtlString address;
      int port;
      UtlString protocol;
      UtlString user;
      UtlString userLabel;
      UtlString tag;
      // Buffer to compose message.
      char message[1000];
      // Message template into which to insert the From address.
      const char* message_template =
         "REGISTER sip:sipx.local SIP/2.0\r\n"
         "Via: SIP/2.0/TCP sipx.local:33855;branch=z9hG4bK-10cb6f9378a12d4218e10ef4dc78ea3d\r\n"
         "To: Sip Send <sip:sipsend@pingtel.org>\r\n"
         "From: %s; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
         "Cseq: 1 REGISTER\r\n"
         "Max-Forwards: 20\r\n"
         "User-Agent: sipsend/0.01\r\n"
         "Contact: me@127.0.0.1\r\n"
         "Expires: 300\r\n"
         "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
         "Content-Length: 0\r\n"
         "\r\n";

      for (unsigned int i = 0; i < sizeof (tests) / sizeof (tests[0]);
         i++)
      {
         // Compose the message.
         sprintf(message, message_template, tests[i].string);
         SipMessage sipMessage(message, strlen(message));

         sipMessage.getFromAddress(&address, &port, &protocol, &user,
            &userLabel, &tag);
         CPPUNIT_ASSERT_EQUAL_MESSAGE(tests[i].string, tests[i].port, port);
      }
   }

   void testGetResponseSendAddress()
   {
      // Message templates into which to insert the address.

      // Template has 2 Via's, to make sure the function is looking
      // at the right Via.
      const char* message_template2 =
         "REGISTER sip:sipx.local SIP/2.0\r\n"
         "Via: SIP/2.0/TCP %s;branch=z9hG4bK-foobarbazquux\r\n"
         "Via: SIP/2.0/TCP sipx.local:33855;branch=z9hG4bK-10cb6f9378a12d4218e10ef4dc78ea3d\r\n"
         "To: sip:sipx.local\r\n"
         "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "\r\n";

      // Template has 1 Via.
      const char* message_template1 =
         "REGISTER sip:sipx.local SIP/2.0\r\n"
         "Via: SIP/2.0/TCP %s;branch=z9hG4bK-foobarbazquux\r\n"
         "To: sip:sipx.local\r\n"
         "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "\r\n";

      // Template has 0 Via's.
      // The From address is used.
      const char* message_template0 =
         "REGISTER sip:sipsend@pingtel.org SIP/2.0\r\n"
         "To: sip:sipx.local\r\n"
         "From: %s; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "\r\n";

      struct test {
         const char* message_template;
         const char* address;
         int port;
      };

      struct test tests[] = {
         { message_template0, "sip:foo@bar", PORT_NONE },
         { message_template0, "sip:foo@bar:0", 0 },
         { message_template0, "sip:foo@bar:100", 100 },
         { message_template0, "sip:foo@bar:5060", 5060 },
         { message_template0, "sip:foo@bar:65535", 65535 },
         { message_template1, "sip:foo@bar", PORT_NONE },
         { message_template1, "sip:foo@bar:0", 0 },
         { message_template1, "sip:foo@bar:100", 100 },
         { message_template1, "sip:foo@bar:5060", 5060 },
         { message_template1, "sip:foo@bar:65535", 65535 },
         { message_template2, "sip:foo@bar", PORT_NONE },
         { message_template2, "sip:foo@bar:0", 0 },
         { message_template2, "sip:foo@bar:100", 100 },
         { message_template2, "sip:foo@bar:5060", 5060 },
         { message_template2, "sip:foo@bar:65535", 65535 }
      };

      // Buffer to compose message.
      char message[1000];

      UtlString address;
      int port;
      UtlString protocol;

      for (unsigned int i = 0; i < sizeof (tests) / sizeof (tests[0]);
         i++)
      {
         // Compose the message.
         sprintf(message, tests[i].message_template, tests[i].address);
         SipMessage sipMessage(message, strlen(message));

         sipMessage.getResponseSendAddress(address,
            port,
            protocol);
         char number[10];
         sprintf(number, "Test %d", i);
         CPPUNIT_ASSERT_EQUAL_MESSAGE(number, tests[i].port, port);
      }
   }

   void testParseAddressFromUriPort()
   {
      struct test {
         const char* string;         // Input string.
         int port;                  // Expected returned port.
      };

      struct test tests[] = {
         { "sip:foo@bar", PORT_NONE },
         { "sip:foo@bar:5060", 5060 },
         { "sip:foo@bar:1", 1 },
         { "sip:foo@bar:100", 100 },
         { "sip:foo@bar:65535", 65535 }
      };

      UtlString address;
      int port;
      UtlString protocol;
      UtlString user;
      UtlString userLabel;
      UtlString tag;

      for (unsigned int i = 0; i < sizeof (tests) / sizeof (tests[0]);
         i++)
      {
         SipMessage::parseAddressFromUri(tests[i].string,
            &address, &port, &protocol, &user,
            &userLabel, &tag);
         CPPUNIT_ASSERT_EQUAL_MESSAGE(tests[i].string, tests[i].port, port);
      }
   }

   void testProbPort()
   {
      const char *szUrl = "\"Display@Name\"<sip:username@sipserver;transport=tcp>;tag=1234-2345";

      UtlString address;
      int       port;
      UtlString protocol;
      UtlString user;
      UtlString userLabel;
      UtlString tag;

      SipMessage::parseAddressFromUri(szUrl,
         &address,
         &port,
         &protocol,
         &user,
         &userLabel,
         &tag);

      CPPUNIT_ASSERT_EQUAL(PORT_NONE, port);

      ASSERT_STR_EQUAL("username", user);
      ASSERT_STR_EQUAL("\"Display@Name\"", userLabel);
      ASSERT_STR_EQUAL("1234-2345", tag);
      ASSERT_STR_EQUAL("sipserver", address);

      OsSocket::IpProtocolSocketType protoNumber;
      SipMessage::convertProtocolStringToEnum(protocol.data(), protoNumber);

      CPPUNIT_ASSERT_EQUAL(OsSocket::TCP, protoNumber);
   }

   void testSdpParse()
   {
      const char* sip = "INVITE 14 SIP/2.0\nContent-Type:application/sdp\n\n"
         "v=0\nm=audio 49170 RTP/AVP 0\nc=IN IP4 224.2.17.12/127";

      SipMessage *msg = new SipMessage(sip);
      const SdpBody *sdp = msg->getSdpBody();

      CPPUNIT_ASSERT_MESSAGE("Null sdp buffer", sdp != NULL);

      int mediaCount = sdp->getMediaSetCount();
      CPPUNIT_ASSERT_EQUAL_MESSAGE("incorrect media count", 1, mediaCount);

      const char* referenceSdp = 
         "v=0\r\nm=audio 49170 RTP/AVP 0\r\nc=IN IP4 224.2.17.12/127\r\n";
      const char* sdpBytes = NULL;
      int sdpByteLength = 0;
      sdp->getBytes(&sdpBytes, &sdpByteLength);
      for(int iii = 0; iii < sdpByteLength; iii++)
      {
         if(referenceSdp[iii] != sdpBytes[iii])
         {
            printf("index[%d]: expected: %d got: %d\n",
               iii, referenceSdp[iii], sdpBytes[iii]);
         }
      }
      CPPUNIT_ASSERT_MESSAGE("Null sdp serialized content", sdpBytes != NULL);
      CPPUNIT_ASSERT_MESSAGE("SDP does not match expected content",
         strcmp(referenceSdp, sdpBytes) == 0);

      SipMessage* msgCopy = new SipMessage(*msg);
      CPPUNIT_ASSERT_MESSAGE("NULL message copy", msgCopy != NULL);
      const SdpBody *sdpCopy = msgCopy->getSdpBody();
      CPPUNIT_ASSERT_MESSAGE("NULL SDP copy", sdpCopy != NULL);
      const char* sdpCopyBytes = NULL;
      int sdpCopyLen = 0;
      sdpCopy->getBytes(&sdpCopyBytes, &sdpCopyLen);
      //printf("SDP copy length: %d\n%s\n", sdpCopyLen, sdpCopyBytes);
      CPPUNIT_ASSERT_MESSAGE("Null sdp copy serialized content", sdpCopyBytes != NULL);
      CPPUNIT_ASSERT_MESSAGE("SDP does not match expected content",
         strcmp(referenceSdp, sdpCopyBytes) == 0);
   }


   void testSdpShortHeaderNames()
   {
      const char* sip = "INVITE 14 SIP/2.0\nc:application/sdp\n\n"
         "v=0\nm=audio 49170 RTP/AVP 0\nc=IN IP4 224.2.17.12/127";

      SipMessage *msg = new SipMessage(sip);
      const SdpBody *sdp = msg->getSdpBody();

      CPPUNIT_ASSERT_MESSAGE("Null sdp buffer", sdp != NULL);

      int mediaCount = sdp->getMediaSetCount();
      CPPUNIT_ASSERT_EQUAL_MESSAGE("incorrect media count", 1, mediaCount);

      const char* referenceSdp = 
         "v=0\r\nm=audio 49170 RTP/AVP 0\r\nc=IN IP4 224.2.17.12/127\r\n";
      const char* sdpBytes = NULL;
      int sdpByteLength = 0;
      sdp->getBytes(&sdpBytes, &sdpByteLength);
      for(int iii = 0; iii < sdpByteLength; iii++)
      {
         if(referenceSdp[iii] != sdpBytes[iii])
         {
            printf("index[%d]: expected: %d got: %d\n",
               iii, referenceSdp[iii], sdpBytes[iii]);
         }
      }
      CPPUNIT_ASSERT_MESSAGE("Null sdp serialized content", sdpBytes != NULL);
      CPPUNIT_ASSERT_MESSAGE("SDP does not match expected content",
         strcmp(referenceSdp, sdpBytes) == 0);

      SipMessage* msgCopy = new SipMessage(*msg);
      CPPUNIT_ASSERT_MESSAGE("NULL message copy", msgCopy != NULL);
      const SdpBody *sdpCopy = msgCopy->getSdpBody();
      CPPUNIT_ASSERT_MESSAGE("NULL SDP copy", sdpCopy != NULL);
      const char* sdpCopyBytes = NULL;
      int sdpCopyLen = 0;
      sdpCopy->getBytes(&sdpCopyBytes, &sdpCopyLen);
      //printf("SDP copy length: %d\n%s\n", sdpCopyLen, sdpCopyBytes);
      CPPUNIT_ASSERT_MESSAGE("Null sdp copy serialized content", sdpCopyBytes != NULL);
      CPPUNIT_ASSERT_MESSAGE("SDP does not match expected content",
         strcmp(referenceSdp, sdpCopyBytes) == 0);
   }

#define NON_SDP_REFERENCE_CONTENT "<FOOSTUFF>\n   <BAR/>\n\r</FOOSTUFF>\n"

   void testNonSdpSipMessage()
   {
      const char* referenceContent = NON_SDP_REFERENCE_CONTENT;
      const char* sip = "INVITE 14 SIP/2.0\nContent-Type:application/fooStuff\n\n"
         NON_SDP_REFERENCE_CONTENT;

      SipMessage *msg = new SipMessage(sip);
      const SdpBody *sdp = msg->getSdpBody();

      CPPUNIT_ASSERT_MESSAGE("sdp body not expected", sdp == NULL);

      const HttpBody* fooBody = msg->getBody();

      const char* fooBytes = NULL;
      int fooByteLength = 0;
      fooBody->getBytes(&fooBytes, &fooByteLength);
      for(int iii = 0; iii < fooByteLength; iii++)
      {
         if(referenceContent[iii] != fooBytes[iii])
         {
            printf("index[%d]: expected: %d got: %d\n",
               iii, referenceContent[iii], fooBytes[iii]);
         }
      }
      CPPUNIT_ASSERT_MESSAGE("Null foo serialized content", fooBytes != NULL);
      CPPUNIT_ASSERT_MESSAGE("serialized content does not match expected content",
         strcmp(referenceContent, fooBytes) == 0);

      SipMessage* msgCopy = new SipMessage(*msg);
      CPPUNIT_ASSERT_MESSAGE("NULL message copy", msgCopy != NULL);
      const HttpBody *fooCopy = msgCopy->getBody();
      CPPUNIT_ASSERT_MESSAGE("NULL foo body copy", fooCopy != NULL);
      const char* fooCopyBytes = NULL;
      int fooCopyLen = 0;
      fooCopy->getBytes(&fooCopyBytes, &fooCopyLen);
      //printf("foo copy length: %d\n%s\n", fooCopyLen, fooCopyBytes);
      CPPUNIT_ASSERT_MESSAGE("Null foo copy serialized content", fooCopyBytes != NULL);
      CPPUNIT_ASSERT_MESSAGE("foo body copy does not match expected content",
         strcmp(referenceContent, fooCopyBytes) == 0);
   }

   void testSetInviteDataHeaders()
   {
      // Test that SipMessage::setInviteData applies headers in the
      // To: URI correctly to the SIP message body.

      // List of headers that should be settable from the URI.
      const char* settable_headers[] =
      {
         SIP_SUBJECT_FIELD,
         SIP_ACCEPT_LANGUAGE_FIELD,
         "Alert-Info",
         "Call-Info",
         SIP_WARNING_FIELD,
         "Error-Info"
      };

      // For each field.
      for (unsigned int i = 0; i < sizeof (settable_headers) / sizeof (settable_headers[0]); i++)
      {
         // The name of the header.
         const char* header_name = settable_headers[i];

         // Create an empty SIP message.
         SipMessage *msg = new SipMessage();

         // Create a To URI containing the header.
         char to_URI[100];
         sprintf(to_URI, "To: <sip:to@example.com?%s=value1>", header_name);

         // Create the SIP message.
         // Since numRtpcodecs = 0, none of the RTP fields are used to produce SDP.
         msg->setInviteData("sip:from@example.com", // fromField 
            to_URI, // toField,
            "sip:remotecontact@example.com", // farEndContact
            "sip:contact@example.com", // contactUrl
            "callid@example.com", // callId
            0, // sequenceNumber
            17 // sessionReinviteTimer
            );

#if 0
         UtlString p;
         int l;
         msg->getBytes(&p, &l);
         fprintf(stderr,
            "testSetInviteDataHeaders for %s after first setInviteData:\n%s\n",
            header_name, p.data());
#endif

         const char* v = msg->getHeaderValue(0, header_name);
         CPPUNIT_ASSERT_MESSAGE(header_name, strcmp(v, "value1") == 0);

         // Create a second To URI containing the header.
         sprintf(to_URI, "To: <sip:to@example.com?%s=value2>", header_name);

         // Update the SIP message, creating a second value for the header.
         // Since numRtpcodecs = 0, none of the RTP fields are used to produce SDP.
         msg->setInviteData("sip:from@example.com", // fromField 
            to_URI, // toField,
            "sip:remotecontact@example.com", // farEndContact
            "sip:contact@example.com", // contactUrl
            "callid@example.com", // callId
            0, // sequenceNumber
            17 // sessionReinviteTimer
            );

#if 0
         msg->getBytes(&p, &l);
         fprintf(stderr,
            "testSetInviteDataHeaders for %s after second setInviteData:\n%s\n",
            header_name, p.data());
#endif

         v = msg->getHeaderValue(0, header_name);
         CPPUNIT_ASSERT_MESSAGE(header_name, strcmp(v, "value1") == 0);

         v = msg->getHeaderValue(1, header_name);
         CPPUNIT_ASSERT_MESSAGE(header_name, strcmp(v, "value2") == 0);

         delete msg;
      }
   }

   void testSetInviteDataHeadersUnique()
   {
      // Test that SipMessage::setInviteData applies headers in the
      // To: URI correctly to the SIP message body.

      // List of headers that should be settable from the URI, but are
      // allowed only one value, so the URI overrides what is already in the
      // message.
      const char* settable_unique_headers[] =
      {
         SIP_EXPIRES_FIELD
      };

      // For each field.
      for (unsigned int i = 0; i < sizeof (settable_unique_headers) / sizeof (settable_unique_headers[0]); i++)
      {
         // The name of the header.
         const char* header_name = settable_unique_headers[i];

         // Create an empty SIP message.
         SipMessage *msg = new SipMessage();

         // Create a To URI containing the header.
         char to_URI[100];
         sprintf(to_URI, "To: <sip:to@example.com?%s=value1>", header_name);

         // Create the SIP message.
         // Since numRtpcodecs = 0, none of the RTP fields are used to produce SDP.
         msg->setInviteData("sip:from@example.com", // fromField 
            to_URI, // toField,
            "sip:remotecontact@example.com", // farEndContact
            "sip:contact@example.com", // contactUrl
            "callid@example.com", // callId
            0, // sequenceNumber
            17 // sessionReinviteTimer
            );

#if 0
         UtlString p;
         int l;
         msg->getBytes(&p, &l);
         fprintf(stderr,
            "testSetInviteDataHeaders for %s after first setInviteData:\n%s\n",
            header_name, p.data());
#endif

         const char* v = msg->getHeaderValue(0, header_name);
         CPPUNIT_ASSERT_MESSAGE(header_name, strcmp(v, "value1") == 0);

         // Create a second To URI containing the header.
         sprintf(to_URI, "To: <sip:to@example.com?%s=value2>", header_name);

         // Update the SIP message, creating a second value for the header.
         // Since numRtpcodecs = 0, none of the RTP fields are used to produce SDP.
         msg->setInviteData("sip:from@example.com", // fromField 
            to_URI, // toField,
            "sip:remotecontact@example.com", // farEndContact
            "sip:contact@example.com", // contactUrl
            "callid@example.com", // callId
            0, // sequenceNumber
            17 // sessionReinviteTimer
            );

#if 0
         msg->getBytes(&p, &l);
         fprintf(stderr,
            "testSetInviteDataHeaders for %s after second setInviteData:\n%s\n",
            header_name, p.data());
#endif

         v = msg->getHeaderValue(0, header_name);
         CPPUNIT_ASSERT_MESSAGE(header_name, strcmp(v, "value2") == 0);

         // Second value must not be present.
         v = msg->getHeaderValue(1, header_name);
         CPPUNIT_ASSERT_MESSAGE(header_name,
            v == NULL ||strcmp(v, "") == 0);

         delete msg;
      }
   }

   void testSetInviteDataHeadersForbidden()
   {
      // Test that SipMessage::setInviteData applies headers in the
      // To: URI correctly to the SIP message body.

      // List of headers that should not be settable from the URI.
      const char* non_settable_headers[] =
      {
         SIP_CONTACT_FIELD,
         SIP_CALLID_FIELD,
         SIP_CSEQ_FIELD,
         SIP_VIA_FIELD,
         SIP_RECORD_ROUTE_FIELD
      };


      // For each field.
      for (unsigned int i = 0; i < sizeof (non_settable_headers) / sizeof (non_settable_headers[0]); i++)
      {
         // The name of the header.
         const char* header_name = non_settable_headers[i];

         // Create an empty SIP message.
         SipMessage *msg = new SipMessage();

         // Create a To URI containing the header.
         char to_URI[100];
         sprintf(to_URI, "To: <sip:to@example.com?%s=value1>", header_name);

         // Create the SIP message.
         // Since numRtpcodecs = 0, none of the RTP fields are used to produce SDP.
         msg->setInviteData("sip:from@example.com", // fromField 
            to_URI, // toField,
            "sip:remotecontact@example.com", // farEndContact
            "sip:contact@example.com", // contactUrl
            "callid@example.com", // callId
            100, // sequenceNumber
            17 // sessionReinviteTimer
            );

#if 0
         UtlString p;
         int l;
         msg->getBytes(&p, &l);
         fprintf(stderr,
            "testSetInviteDataHeaders for %s after first setInviteData:\n%s\n",
            header_name, p.data());
#endif

         const char* v = msg->getHeaderValue(0, header_name);
         // Value must be absent, or NOT the specified value.
         CPPUNIT_ASSERT_MESSAGE(header_name,
            v == NULL || strcmp(v, "value1") != 0);

         delete msg;
      }
   }

   void testApplyTargetUriHeaderParams()
   {
      UtlString dumpStr;
      int       dumpLength;

      dumpLength = 0;
      dumpLength = dumpLength; // suppress compiler warnings about unused variables

      {
         // add an arbitrary unknown header
         const char* rawmsg =
            "INVITE sip:sipx.local?arbitrary-header=foobar SIP/2.0\r\n"
            "To: sip:sipx.local\r\n"
            "From: Sip Send <sip:sender@example.org>; tag=ORIG-TAG\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: me@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";

         SipMessage sipmsg(rawmsg, strlen(rawmsg));
         sipmsg.applyTargetUriHeaderParams();

         // sipmsg.getBytes(&dumpStr, &dumpLength);
         // printf( "\nMSG BEFORE:\n%s\nAFTER:\n%s\n", rawmsg, dumpStr.data());

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("arbitrary-header"));
         ASSERT_STR_EQUAL("foobar", sipmsg.getHeaderValue(0, "arbitrary-header"));
         ASSERT_STR_EQUAL("INVITE sip:sipx.local SIP/2.0", sipmsg.getFirstHeaderLine());
      }

      {
         // add 2 of an arbitrary unknown header
         const char* rawmsg =
            "INVITE sip:sipx.local?arbitrary-header=foobar&arbitrary-header=again SIP/2.0\r\n"
            "To: sip:sipx.local\r\n"
            "From: Sip Send <sip:sender@example.org>; tag=ORIG-TAG\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: me@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";

         SipMessage sipmsg(rawmsg, strlen(rawmsg));
         sipmsg.applyTargetUriHeaderParams();

         // sipmsg.getBytes(&dumpStr, &dumpLength);
         // printf( "\nMSG BEFORE:\n%s\nAFTER:\n%s\n", rawmsg, dumpStr.data());

         CPPUNIT_ASSERT_EQUAL(2, sipmsg.getCountHeaderFields("arbitrary-header"));
         ASSERT_STR_EQUAL("foobar", sipmsg.getHeaderValue(0, "arbitrary-header"));
         ASSERT_STR_EQUAL("again", sipmsg.getHeaderValue(1, "arbitrary-header"));
         ASSERT_STR_EQUAL("INVITE sip:sipx.local SIP/2.0", sipmsg.getFirstHeaderLine());
      }

      {
         // add an expires header
         const char* rawmsg =
            "INVITE sip:sipx.local?expires=10 SIP/2.0\r\n"
            "To: sip:sipx.local\r\n"
            "From: Sip Send <sip:sender@example.org>; tag=ORIG-TAG\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: me@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";

         SipMessage sipmsg(rawmsg, strlen(rawmsg));
         sipmsg.applyTargetUriHeaderParams();

         // sipmsg.getBytes(&dumpStr, &dumpLength);
         // printf( "\nMSG BEFORE:\n%s\nAFTER:\n%s\n", rawmsg, dumpStr.data());

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("expires"));
         ASSERT_STR_EQUAL("10", sipmsg.getHeaderValue(0, "expires"));
         ASSERT_STR_EQUAL("INVITE sip:sipx.local SIP/2.0", sipmsg.getFirstHeaderLine());
      }

      {
         // try to add 2 expires headers
         const char* rawmsg =
            "INVITE sip:sipx.local?expires=10&expires=20 SIP/2.0\r\n"
            "To: sip:sipx.local\r\n"
            "From: Sip Send <sip:sender@example.org>; tag=ORIG-TAG\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: me@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";

         SipMessage sipmsg(rawmsg, strlen(rawmsg));
         sipmsg.applyTargetUriHeaderParams();

         // sipmsg.getBytes(&dumpStr, &dumpLength);
         // printf( "\nMSG BEFORE:\n%s\nAFTER:\n%s\n", rawmsg, dumpStr.data());

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("expires"));
         ASSERT_STR_EQUAL("20", sipmsg.getHeaderValue(0, "expires"));
         ASSERT_STR_EQUAL("INVITE sip:sipx.local SIP/2.0", sipmsg.getFirstHeaderLine());
      }

      {
         // add a From header
         const char* rawmsg =
            "INVITE sip:sipx.local?from=sip%3Afoo%40bar SIP/2.0\r\n"
            "To: sip:sipx.local\r\n"
            "From: Sip Send <sip:sender@example.org>; tag=ORIG-TAG\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: me@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";

         SipMessage sipmsg(rawmsg, strlen(rawmsg));
         sipmsg.applyTargetUriHeaderParams();

         // sipmsg.getBytes(&dumpStr, &dumpLength);
         // printf( "\nMSG BEFORE:\n%s\nAFTER:\n%s\n", rawmsg, dumpStr.data());

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("from"));
         ASSERT_STR_EQUAL("<sip:foo@bar>;tag=ORIG-TAG", sipmsg.getHeaderValue(0, "from"));

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("x-original-from"));
         ASSERT_STR_EQUAL("Sip Send <sip:sender@example.org>; tag=ORIG-TAG",
            sipmsg.getHeaderValue(0, "x-original-from"));

         ASSERT_STR_EQUAL("INVITE sip:sipx.local SIP/2.0", sipmsg.getFirstHeaderLine());
      }

      {
         // add a From header with a tag - the tag should be ignored
         const char* rawmsg =
            "INVITE sip:sipx.local?from=sip%3Afoo%40bar%3Btag%3DBAD-TAG SIP/2.0\r\n"
            "To: sip:sipx.local\r\n"
            "From: Sip Send <sip:sender@example.org>; tag=ORIG-TAG\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: me@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";

         SipMessage sipmsg(rawmsg, strlen(rawmsg));
         sipmsg.applyTargetUriHeaderParams();

         // sipmsg.getBytes(&dumpStr, &dumpLength);
         // printf( "\nMSG BEFORE:\n%s\nAFTER:\n%s\n", rawmsg, dumpStr.data());

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("from"));
         ASSERT_STR_EQUAL("<sip:foo@bar>;tag=ORIG-TAG", sipmsg.getHeaderValue(0, "from"));

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("x-original-from"));
         ASSERT_STR_EQUAL("Sip Send <sip:sender@example.org>; tag=ORIG-TAG",
            sipmsg.getHeaderValue(0, "x-original-from"));

         ASSERT_STR_EQUAL("INVITE sip:sipx.local SIP/2.0", sipmsg.getFirstHeaderLine());
      }


      {
         // try to add two From headers
         //   this should work, but the second one should overwrite the first
         //   and there will be 2 x-original-from headers (odd, but ok)
         const char* rawmsg =
            "INVITE sip:sipx.local?from=sip%3Afoo%40bar&from=sip%3Asecond%40try SIP/2.0\r\n"
            "To: sip:sipx.local\r\n"
            "From: Sip Send <sip:sender@example.org>; tag=ORIG-TAG\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: me@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";

         SipMessage sipmsg(rawmsg, strlen(rawmsg));
         sipmsg.applyTargetUriHeaderParams();

         // sipmsg.getBytes(&dumpStr, &dumpLength);
         // printf( "\nMSG BEFORE:\n%s\nAFTER:\n%s\n", rawmsg, dumpStr.data());

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("from"));
         ASSERT_STR_EQUAL("<sip:second@try>;tag=ORIG-TAG", sipmsg.getHeaderValue(0, "from"));

         CPPUNIT_ASSERT_EQUAL(2, sipmsg.getCountHeaderFields("x-original-from"));
         ASSERT_STR_EQUAL("Sip Send <sip:sender@example.org>; tag=ORIG-TAG",
            sipmsg.getHeaderValue(0, "x-original-from"));
         ASSERT_STR_EQUAL("<sip:foo@bar>;tag=ORIG-TAG",
            sipmsg.getHeaderValue(1, "x-original-from"));

         ASSERT_STR_EQUAL("INVITE sip:sipx.local SIP/2.0", sipmsg.getFirstHeaderLine());
      }

#        if 0
      // these next two are broken because of un-escaping problems...
      {
         // try to add a From header with a display name
         const char* rawmsg =
            "INVITE sip:sipx.local?from=%22Foo+Bar%22+sip%3Afoo%40bar SIP/2.0\r\n"
            "To: sip:sipx.local\r\n"
            "From: Sip Send <sip:sender@example.org>; tag=ORIG-TAG\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: me@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";

         SipMessage sipmsg(rawmsg, strlen(rawmsg));
         sipmsg.applyTargetUriHeaderParams();

         sipmsg.getBytes(&dumpStr, &dumpLength);
         printf( "\nMSG BEFORE:\n%s\nAFTER:\n%s\n", rawmsg, dumpStr.data());

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("from"));
         ASSERT_STR_EQUAL("Foo Bar<sip:foo@bar>;tag=ORIG-TAG", sipmsg.getHeaderValue(0, "from"));

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("x-original-from"));
         ASSERT_STR_EQUAL("From: Sip Send <sip:sender@example.org>; tag=ORIG-TAG",
            sipmsg.getHeaderValue(0, "x-original-from"));

         ASSERT_STR_EQUAL("INVITE sip:sipx.local SIP/2.0", sipmsg.getFirstHeaderLine());
      }

      {
         // try to add a From header with a display name
         const char* rawmsg =
            "INVITE sip:sipx.local?from=Foo+Bar+%3Csip%3Afoo%40bar%3A SIP/2.0\r\n"
            "To: sip:sipx.local\r\n"
            "From: Sip Send <sip:sender@example.org>; tag=ORIG-TAG\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: me@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";

         SipMessage sipmsg(rawmsg, strlen(rawmsg));
         sipmsg.applyTargetUriHeaderParams();

         sipmsg.getBytes(&dumpStr, &dumpLength);
         printf( "\nMSG BEFORE:\n%s\nAFTER:\n%s\n", rawmsg, dumpStr.data());

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("from"));
         ASSERT_STR_EQUAL("Foo Bar<sip:foo@bar>;tag=ORIG-TAG", sipmsg.getHeaderValue(0, "from"));

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("x-original-from"));
         ASSERT_STR_EQUAL("From: Sip Send <sip:sender@example.org>; tag=ORIG-TAG",
            sipmsg.getHeaderValue(0, "x-original-from"));

         ASSERT_STR_EQUAL("INVITE sip:sipx.local SIP/2.0", sipmsg.getFirstHeaderLine());
      }
#        endif

      {
         // try to add a Route header
         const char* rawmsg =
            "INVITE sip:sipx.local?route=%3Csip%3Afoo%40bar%3Blr%3E SIP/2.0\r\n"
            "To: sip:sipx.local\r\n"
            "From: Sip Send <sip:sender@example.org>; tag=ORIG-TAG\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: me@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";

         SipMessage sipmsg(rawmsg, strlen(rawmsg));
         sipmsg.applyTargetUriHeaderParams();

         // sipmsg.getBytes(&dumpStr, &dumpLength);
         // printf( "\nMSG BEFORE:\n%s\nAFTER:\n%s\n", rawmsg, dumpStr.data());

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("route"));
         ASSERT_STR_EQUAL("<sip:foo@bar;lr>", sipmsg.getHeaderValue(0, "route"));
         ASSERT_STR_EQUAL("INVITE sip:sipx.local SIP/2.0", sipmsg.getFirstHeaderLine());
      }

      {
         // try to add a Route header when there is already one present
         const char* rawmsg =
            "INVITE sip:sipx.local?route=%3Csip%3Afoo%40bar%3Blr%3E SIP/2.0\r\n"
            "Route: <sip:original@route;lr>\r\n"
            "To: sip:sipx.local\r\n"
            "From: Sip Send <sip:sender@example.org>; tag=ORIG-TAG\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: me@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";

         SipMessage sipmsg(rawmsg, strlen(rawmsg));
         sipmsg.applyTargetUriHeaderParams();

         // sipmsg.getBytes(&dumpStr, &dumpLength);
         // printf( "\nMSG BEFORE:\n%s\nAFTER:\n%s\n", rawmsg, dumpStr.data());

         CPPUNIT_ASSERT_EQUAL(1, sipmsg.getCountHeaderFields("route"));
         ASSERT_STR_EQUAL("<sip:foo@bar;lr>,<sip:original@route;lr>", sipmsg.getHeaderValue(0, "route"));
         ASSERT_STR_EQUAL("INVITE sip:sipx.local SIP/2.0", sipmsg.getFirstHeaderLine());
      }
   }

   void testCompactNames()
   {
      const char* CompactMessage =
         "METHOD sip:sipx.local SIP/2.0\r\n"
         "v: SIP/2.0/TCP sipx.local:33855;branch=z9hG4bK-10cb6f9378a12d4218e10ef4dc78ea3d\r\n"
         "v: SIP/2.0/TCP sipx.remote:999999;branch=z9hG4bK-remote-tid\r\n"
         "t: sip:sipx.local\r\n"
         "f: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "i: f88dfabce84b6a2787ef024a7dbe8749\r\n"
         "Cseq: 1 REGISTER\r\n"
         "Max-Forwards: 20\r\n"
         "o: event-package\r\n"
         "r: sip:refer@address.example.com\r\n"
         "b: sip:refered-by@address.example.com\r\n"
         "User-Agent: sipsend/0.01\r\n"
         "s: Some silly subject\r\n"
         "k: a-supported-token\r\n"
         "m: me@127.0.0.1\r\n"
         "Expires: 300\r\n"
         "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
         "l: 0\r\n"
         "c: application/sdp\r\n"
         "e: gzip\r\n"
         "\r\n";
      SipMessage testMsg( CompactMessage, strlen( CompactMessage ) );

      const char* LongForm =
         "METHOD sip:sipx.local SIP/2.0\r\n"
         "Via: SIP/2.0/TCP sipx.local:33855;branch=z9hG4bK-10cb6f9378a12d4218e10ef4dc78ea3d\r\n"
         "Via: SIP/2.0/TCP sipx.remote:999999;branch=z9hG4bK-remote-tid\r\n"
         "To: sip:sipx.local\r\n"
         "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
         "Call-Id: f88dfabce84b6a2787ef024a7dbe8749\r\n"
         "Cseq: 1 REGISTER\r\n"
         "Max-Forwards: 20\r\n"
         "Event: event-package\r\n"
         "Refer-To: sip:refer@address.example.com\r\n"
         "Referred-By: sip:refered-by@address.example.com\r\n"
         "User-Agent: sipsend/0.01\r\n"
         "Subject: Some silly subject\r\n"
         "Supported: a-supported-token\r\n"
         "Contact: me@127.0.0.1\r\n"
         "Expires: 300\r\n"
         "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
         "Content-Length: 0\r\n"
         "Content-Type: application/sdp\r\n"
         "Content-Encoding: gzip\r\n"
         "\r\n";

      UtlString translated;
      int length;

      testMsg.getBytes(&translated, &length);

      ASSERT_STR_EQUAL(LongForm, translated.data());

   };

   void testHeaderFieldAccessors()
   {
      const char* messageBlob =
         "INVITE sip:fred@example.com SIP/2.0\r\n"
         "From: sip:betty@example.com\r\n"
         "To: Fred<sip:fred@example.com\r\n"
         "CSeq: 3 INVITE\r\n"
         "Call-Id: 1234\r\n"
         "Via: SIP/2.0/UDP 127.0.0.1:4444;branch=z9hG4bK-10\r\n"
         "P-Asserted-Identity: Fredrick<freddy@east.example.com>    ,     tel:1234567890  \r\n"
         "P-ASSERTED-IDENTITY: foo<sip:bar@my.example.com\r\n"
         "Content-Length: 0\r\n"
         "\r\n";

      SipMessage message(messageBlob);

      UtlString identity;
      UtlString testErrorMessage;
      CPPUNIT_ASSERT(message.getPAssertedIdentityField(identity, 0));
      const char* expectedIdentity0 = "Fredrick<freddy@east.example.com>";
      testErrorMessage = "expected: ";
      testErrorMessage.append(expectedIdentity0);
      CPPUNIT_ASSERT_MESSAGE(testErrorMessage.data(),
         identity.compareTo(expectedIdentity0) == 0);

      const char* expectedIdentity1 = "tel:1234567890";
      testErrorMessage = "expected: ";
      testErrorMessage.append(expectedIdentity1);
      CPPUNIT_ASSERT(message.getPAssertedIdentityField(identity, 1));
      CPPUNIT_ASSERT_MESSAGE(testErrorMessage.data(),
         identity.compareTo(expectedIdentity1) == 0);

      message.removePAssertedIdentityFields();
      CPPUNIT_ASSERT(!message.getPAssertedIdentityField(identity, 0));

      const char* messageHeaderLine = "FOO sip:fred@example.com SIP/2.0\r\n";
      SipMessage writeMessage(messageHeaderLine);
      writeMessage.addPAssertedIdentityField(expectedIdentity1);
      writeMessage.addPAssertedIdentityField(expectedIdentity0);

      UtlString expectedMessage = messageHeaderLine;
      expectedMessage.append("P-Asserted-Identity: ");
      expectedMessage.append(expectedIdentity1);
      expectedMessage.append(", ");
      expectedMessage.append(expectedIdentity0);
      expectedMessage.append("\r\nContent-Length: 0\r\n\r\n");
      UtlString messageBytes;
      int len;
      writeMessage.getBytes(&messageBytes, &len);

      testErrorMessage = "expected: ";
      testErrorMessage.append(expectedMessage);
      if(messageBytes.compareTo(expectedMessage))
      {
         printf("Actual: %s\n", messageBytes.data());
         printf("Expected: %s\n", expectedMessage.data());
      }
      CPPUNIT_ASSERT( messageBytes.compareTo(expectedMessage) == 0);
   }
};

CPPUNIT_TEST_SUITE_REGISTRATION(SipMessageTest);
