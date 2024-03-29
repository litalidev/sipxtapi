//
// Copyright (C) 2006 SIPez LLC.
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


#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include "net/Url.h"
#include "sipXtapiTest.h"
#include "EventValidator.h"
#include "callbacks.h"

extern SIPX_INST g_hInst1;
extern SIPX_INST g_hInst2;
extern SIPX_INST g_hInst3;
extern SIPX_INST g_hInst5;

extern bool g_bCallbackCalled;


// Setup conference and drop conference as a whole
void sipXtapiTestSuite::testConfBasic1()
{
   bool bRC;
   EventValidator validatorCalling("testConfBasic1.calling");
   EventValidator validatorCalled1("testConfBasic1.called1");
   EventValidator validatorCalled2("testConfBasic1.called2");

   for (int iStressFactor = 0; iStressFactor<STRESS_FACTOR; iStressFactor++)
   {
      printf("\ntestConfBasic1 (%2d of %2d)", iStressFactor+1, STRESS_FACTOR);

      SIPX_CALL hCall1;
      SIPX_CALL hCall2;
      SIPX_CONF hConf;
      SIPX_LINE hLine;
      SIPX_LINE hLine_Alt;
      SIPX_LINE hReceivingLine1;
      SIPX_LINE hReceivingLine2;

      SIPX_CALL hCheckCalls[10];
      size_t nCheckCalls;

      validatorCalling.reset();
      validatorCalled1.reset();
      validatorCalled2.reset();
      validatorCalling.setMaxLookhead(12);
      validatorCalled1.setMaxLookhead(12);
      validatorCalled2.setMaxLookhead(12);

      // Setup Auto-answer call back
      resetAutoAnswerCallback();
      resetAutoAnswerCallback2();
      sipxEventListenerAdd(g_hInst1, UniversalEventValidatorCallback, &validatorCalling);
      sipxEventListenerAdd(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1);
      sipxEventListenerAdd(g_hInst2, AutoAnswerCallback, NULL);
      sipxEventListenerAdd(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2);
      sipxEventListenerAdd(g_hInst3, AutoAnswerCallback2, NULL);

      // Create Lines
      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst1, "sip:foo@127.0.0.1:8000", &hLine), SIPX_RESULT_SUCCESS);
      bRC = validatorCalling.waitForLineEvent(hLine, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst2, "sip:foo@127.0.0.1:9100", &hReceivingLine1, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled1.waitForLineEvent(hReceivingLine1, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst3, "sip:foo@127.0.0.1:10000", &hReceivingLine2, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled2.waitForLineEvent(hReceivingLine2, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Setup first leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceCreate(g_hInst1, &hConf), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine, "sip:blah@127.0.0.1:9100", &hCall1), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalling.validateCallMediaStartEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);


      // Validate Called Side
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalled1.validateCallMediaStartEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 1);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall1);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst1, "\"Flibble Flabble\" <sip:bar@127.0.0.1:8000>", &hLine_Alt), SIPX_RESULT_SUCCESS);
      bRC = validatorCalling.waitForLineEvent(hLine_Alt, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Setup 2nd Leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine_Alt, "sip:blah2@127.0.0.1:10000", &hCall2), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine_Alt, hCall2, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalling.validateCallMediaStartEvents();
      bRC = validatorCalling.waitForCallEvent(hLine_Alt, hCall2, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine_Alt, hCall2, CALLSTATE_REMOTE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine_Alt, hCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);


      // Validate Called Side
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalled2.validateCallMediaStartEvents();
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);


      char szCallerId[256];
      sipxCallGetRemoteField(g_hAutoAnswerCallbackCall2, szCallerId, 256);
      Url callerUrl(szCallerId);
      UtlString displayName;
      callerUrl.getDisplayName(displayName);

      const char* flibble = "\"Flibble Flabble\"";
      CPPUNIT_ASSERT(0 == strcmp(displayName, flibble));

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 2);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall1);
      CPPUNIT_ASSERT(hCheckCalls[1] == hCall2);
      CPPUNIT_ASSERT(hCheckCalls[0] != hCheckCalls[1]);

      // Tear down conference
      CPPUNIT_ASSERT_EQUAL(sipxConferenceDestroy(hConf), SIPX_RESULT_SUCCESS);
      // Validate Calling Side
      OsTask::delay(CALL_DELAY*4);

      validatorCalling.validateCallMediaStopEvents();
      validatorCalling.validateCallMediaStopEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, false);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine_Alt, hCall2, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, false);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, false);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine_Alt, hCall2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, false);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Sides
      validatorCalled1.validateCallMediaStopEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      validatorCalled2.validateCallMediaStopEvents();
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst1, UniversalEventValidatorCallback, &validatorCalling), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, AutoAnswerCallback, NULL), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, AutoAnswerCallback2, NULL), SIPX_RESULT_SUCCESS);

      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hLine), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hLine_Alt), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine2), SIPX_RESULT_SUCCESS);
   }

   OsTask::delay(TEST_DELAY);
   checkForLeaks();
}

// Setup conference, drop party 1, drop party 2
void sipXtapiTestSuite::testConfBasic2()
{
   bool bRC;
   EventValidator validatorCalling("testConfBasic2.calling");
   EventValidator validatorCalled1("testConfBasic2.called1");
   EventValidator validatorCalled2("testConfBasic2.called2");

   for (int iStressFactor = 0; iStressFactor<STRESS_FACTOR; iStressFactor++)
   {
      printf("\ntestConfBasic2 (%2d of %2d)", iStressFactor+1, STRESS_FACTOR);

      SIPX_CALL hCall1;
      SIPX_CALL hCall2;
      SIPX_CONF hConf;
      SIPX_LINE hLine;
      SIPX_LINE hReceivingLine1;
      SIPX_LINE hReceivingLine2;

      SIPX_CALL hCheckCalls[10];
      size_t nCheckCalls;

      validatorCalling.reset();
      validatorCalled1.reset();
      validatorCalled2.reset();

      // Setup Auto-answer call back
      resetAutoAnswerCallback();
      resetAutoAnswerCallback2();
      sipxEventListenerAdd(g_hInst1, UniversalEventValidatorCallback, &validatorCalling);
      sipxEventListenerAdd(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1);
      sipxEventListenerAdd(g_hInst2, AutoAnswerCallback, NULL);
      sipxEventListenerAdd(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2);
      sipxEventListenerAdd(g_hInst3, AutoAnswerCallback2, NULL);

      // Create Lines
      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst1, "sip:foo@127.0.0.1:8000", &hLine), SIPX_RESULT_SUCCESS);
      bRC = validatorCalling.waitForLineEvent(hLine, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst2, "sip:foo@127.0.0.1:9100", &hReceivingLine1, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled1.waitForLineEvent(hReceivingLine1, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst3, "sip:foo@127.0.0.1:10000", &hReceivingLine2, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled2.waitForLineEvent(hReceivingLine2, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Setup first leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceCreate(g_hInst1, &hConf), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine, "sip:blah@127.0.0.1:9100", &hCall1), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalling.validateCallMediaStartEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalled1.validateCallMediaStartEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);


      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 1);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall1);

      // Setup 2nd Leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine, "sip:blah2@127.0.0.1:10000", &hCall2), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalling.validateCallMediaStartEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_REMOTE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalled2.validateCallMediaStartEvents();
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 2);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall1);
      CPPUNIT_ASSERT(hCheckCalls[1] == hCall2);
      CPPUNIT_ASSERT(hCheckCalls[0] != hCheckCalls[1]);

      SIPX_CALL hTmpCall1 = hCall1;
      // Tear Down first leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceRemove(hConf, &hTmpCall1), SIPX_RESULT_SUCCESS);

      validatorCalling.validateCallMediaStopEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Sides
      validatorCalled1.validateCallMediaStopEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 1);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall2);

      SIPX_CALL hTmpCall2 = hCall2;
      // Tear Down 2nd leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceRemove(hConf, &hTmpCall2), SIPX_RESULT_SUCCESS);

      validatorCalling.validateCallMediaStopEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Sides
      validatorCalled2.validateCallMediaStopEvents();
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Tear down conference
      CPPUNIT_ASSERT_EQUAL(sipxConferenceDestroy(hConf), SIPX_RESULT_SUCCESS);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst1, UniversalEventValidatorCallback, &validatorCalling), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, AutoAnswerCallback, NULL), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, AutoAnswerCallback2, NULL), SIPX_RESULT_SUCCESS);

      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hLine), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine2), SIPX_RESULT_SUCCESS);
   }

   OsTask::delay(TEST_DELAY);
   checkForLeaks();
}


// Setup conference, drop party 2, drop party 1
void sipXtapiTestSuite::testConfBasic3()
{
   bool bRC;
   EventValidator validatorCalling("testConfBasic3.calling");
   EventValidator validatorCalled1("testConfBasic3.called1");
   EventValidator validatorCalled2("testConfBasic3.called2");

   for (int iStressFactor = 0; iStressFactor<STRESS_FACTOR; iStressFactor++)
   {
      printf("\ntestConfBasic3 (%2d of %2d)", iStressFactor+1, STRESS_FACTOR);

      SIPX_CALL hCall1;
      SIPX_CALL hCall2;
      SIPX_CONF hConf;
      SIPX_LINE hLine;
      SIPX_LINE hReceivingLine1;
      SIPX_LINE hReceivingLine2;

      SIPX_CALL hCheckCalls[10];
      size_t nCheckCalls;

      validatorCalling.reset();
      validatorCalled1.reset();
      validatorCalled2.reset();

      // Setup Auto-answer call back
      resetAutoAnswerCallback();
      resetAutoAnswerCallback2();
      sipxEventListenerAdd(g_hInst1,  UniversalEventValidatorCallback, &validatorCalling);
      sipxEventListenerAdd(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1);
      sipxEventListenerAdd(g_hInst2, AutoAnswerCallback, NULL);
      sipxEventListenerAdd(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2);
      sipxEventListenerAdd(g_hInst3, AutoAnswerCallback2, NULL);

      // Create Lines
      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst1, "sip:foo@127.0.0.1:8000", &hLine), SIPX_RESULT_SUCCESS);
      bRC = validatorCalling.waitForLineEvent(hLine, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst2, "sip:foo@127.0.0.1:9100", &hReceivingLine1, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled1.waitForLineEvent(hReceivingLine1, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst3, "sip:foo@127.0.0.1:10000", &hReceivingLine2, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled2.waitForLineEvent(hReceivingLine2, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Setup first leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceCreate(g_hInst1, &hConf), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine, "sip:blah@127.0.0.1:9100", &hCall1), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalling.validateCallMediaStartEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalled1.validateCallMediaStartEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 1);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall1);

      // Setup 2nd Leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine, "sip:blah2@127.0.0.1:10000", &hCall2), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalling.validateCallMediaStartEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_REMOTE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalled2.validateCallMediaStartEvents();
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 2);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall1);
      CPPUNIT_ASSERT(hCheckCalls[1] == hCall2);
      CPPUNIT_ASSERT(hCheckCalls[0] != hCheckCalls[1]);

      SIPX_CALL hRemovedCall2 = hCall2;
      // Tear Down 2nd leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceRemove(hConf, &hRemovedCall2), SIPX_RESULT_SUCCESS);

      validatorCalling.validateCallMediaStopEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Sides
      validatorCalled2.validateCallMediaStopEvents();
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 1);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall1);

      SIPX_CALL hRemovedCall1 = hCall1;
      // Tear Down first leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceRemove(hConf, &hRemovedCall1), SIPX_RESULT_SUCCESS);

      validatorCalling.validateCallMediaStopEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Sides
      validatorCalled1.validateCallMediaStopEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 0);

      // Tear down conference
      CPPUNIT_ASSERT_EQUAL(sipxConferenceDestroy(hConf), SIPX_RESULT_SUCCESS);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst1, UniversalEventValidatorCallback, &validatorCalling), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, AutoAnswerCallback, NULL), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, AutoAnswerCallback2, NULL), SIPX_RESULT_SUCCESS);

      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hLine), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine2), SIPX_RESULT_SUCCESS);
   }

   OsTask::delay(TEST_DELAY);
   checkForLeaks();
}


// Setup conference, drop party 1, drop party 2 using callDestroy
void sipXtapiTestSuite::testConfBasic4()
{
   bool bRC;
   EventValidator validatorCalling("testConfBasic4.calling");
   EventValidator validatorCalled1("testConfBasic4.called1");
   EventValidator validatorCalled2("testConfBasic4.called2");

   for (int iStressFactor = 0; iStressFactor<STRESS_FACTOR; iStressFactor++)
   {
      printf("\ntestConfBasic4 (%2d of %2d)", iStressFactor+1, STRESS_FACTOR);

      SIPX_CALL hCall1;
      SIPX_CALL hCall2;
      SIPX_CALL hCallDestroy;
      SIPX_CONF hConf;
      SIPX_LINE hLine;
      SIPX_LINE hReceivingLine1;
      SIPX_LINE hReceivingLine2;

      SIPX_CALL hCheckCalls[10];
      size_t nCheckCalls;

      validatorCalling.reset();
      validatorCalled1.reset();
      validatorCalled2.reset();

      // Setup Auto-answer call back
      resetAutoAnswerCallback();
      resetAutoAnswerCallback2();
      sipxEventListenerAdd(g_hInst1, UniversalEventValidatorCallback, &validatorCalling);
      sipxEventListenerAdd(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1);
      sipxEventListenerAdd(g_hInst2, AutoAnswerCallback, NULL);
      sipxEventListenerAdd(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2);
      sipxEventListenerAdd(g_hInst3, AutoAnswerCallback2, NULL);

      // Create Lines
      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst1, "sip:foo@127.0.0.1:8000", &hLine), SIPX_RESULT_SUCCESS);
      bRC = validatorCalling.waitForLineEvent(hLine, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst2, "sip:foo@127.0.0.1:9100", &hReceivingLine1, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled1.waitForLineEvent(hReceivingLine1, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst3, "sip:foo@127.0.0.1:10000", &hReceivingLine2, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled2.waitForLineEvent(hReceivingLine2, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Setup first leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceCreate(g_hInst1, &hConf), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine, "sip:blah@127.0.0.1:9100", &hCall1), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalling.validateCallMediaStartEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalled1.validateCallMediaStartEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 1);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall1);

      // Setup 2nd Leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine, "sip:blah2@127.0.0.1:10000", &hCall2), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalling.validateCallMediaStartEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_REMOTE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalled2.validateCallMediaStartEvents();
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 2);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall1);
      CPPUNIT_ASSERT(hCheckCalls[1] == hCall2);
      CPPUNIT_ASSERT(hCheckCalls[0] != hCheckCalls[1]);


      // Tear Down first leg
      hCallDestroy = hCall1;
      CPPUNIT_ASSERT_EQUAL(sipxCallDestroy(&hCallDestroy), SIPX_RESULT_SUCCESS);

      validatorCalling.validateCallMediaStopEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Sides
      validatorCalled1.validateCallMediaStopEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 1);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall2);

      // Tear Down 2nd leg
      hCallDestroy = hCall2;
      CPPUNIT_ASSERT_EQUAL(sipxCallDestroy(&hCallDestroy), SIPX_RESULT_SUCCESS);

      validatorCalling.validateCallMediaStopEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Sides
      validatorCalled2.validateCallMediaStopEvents();
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Tear down conference
      CPPUNIT_ASSERT_EQUAL(sipxConferenceDestroy(hConf), SIPX_RESULT_SUCCESS);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst1, UniversalEventValidatorCallback, &validatorCalling), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, AutoAnswerCallback, NULL), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, AutoAnswerCallback2, NULL), SIPX_RESULT_SUCCESS);

      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hLine), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine2), SIPX_RESULT_SUCCESS);
   }

   OsTask::delay(TEST_DELAY);
   checkForLeaks();
}

// Setup conference, drop party 2, redial party 2, drop conference
void sipXtapiTestSuite::testConfBasic5()
{
   bool bRC;
   EventValidator validatorCalling("testConfBasic5.calling");
   EventValidator validatorCalled1("testConfBasic5.called1");
   EventValidator validatorCalled2("testConfBasic5.called2");

   for (int iStressFactor = 0; iStressFactor<STRESS_FACTOR; iStressFactor++)
   {
      printf("\ntestConfBasic5 (%2d of %2d)", iStressFactor+1, STRESS_FACTOR);

      SIPX_CALL hCall1;
      SIPX_CALL hCall2;
      SIPX_CONF hConf;
      SIPX_LINE hLine;
      SIPX_LINE hReceivingLine1;
      SIPX_LINE hReceivingLine2;

      SIPX_CALL hCheckCalls[10];
      size_t nCheckCalls;

      validatorCalling.reset();
      validatorCalled1.reset();
      validatorCalled2.reset();

      // Setup Auto-answer call back
      resetAutoAnswerCallback();
      resetAutoAnswerCallback2();
      sipxEventListenerAdd(g_hInst1, UniversalEventValidatorCallback, &validatorCalling);
      sipxEventListenerAdd(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1);
      sipxEventListenerAdd(g_hInst2, AutoAnswerCallback, NULL);
      sipxEventListenerAdd(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2);
      sipxEventListenerAdd(g_hInst3, AutoAnswerCallback2, NULL);

      // Create Lines
      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst1, "sip:foo@127.0.0.1:8000", &hLine), SIPX_RESULT_SUCCESS);
      bRC = validatorCalling.waitForLineEvent(hLine, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst2, "sip:foo@127.0.0.1:9100", &hReceivingLine1, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled1.waitForLineEvent(hReceivingLine1, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst3, "sip:foo@127.0.0.1:10000", &hReceivingLine2, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled2.waitForLineEvent(hReceivingLine2, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Setup first leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceCreate(g_hInst1, &hConf), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine, "sip:blah@127.0.0.1:9100", &hCall1), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalling.validateCallMediaStartEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalled1.validateCallMediaStartEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 1);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall1);

      // Setup 2nd Leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine, "sip:blah2@127.0.0.1:10000", &hCall2), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalling.validateCallMediaStartEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_REMOTE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalled2.validateCallMediaStartEvents();
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 2);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall1);
      CPPUNIT_ASSERT(hCheckCalls[1] == hCall2);
      CPPUNIT_ASSERT(hCheckCalls[0] != hCheckCalls[1]);

      SIPX_CALL hRemovedCall1 = hCall1;
      // Tear Down first leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceRemove(hConf, &hRemovedCall1), SIPX_RESULT_SUCCESS);

      validatorCalling.validateCallMediaStopEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Sides
      validatorCalled1.validateCallMediaStopEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 1);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall2);

      // Redial first leg
      resetAutoAnswerCallback();
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine, "sip:blah@127.0.0.1:9100", &hCall1), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalling.validateCallMediaStartEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalled1.validateCallMediaStartEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 2);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall2);
      CPPUNIT_ASSERT(hCheckCalls[1] == hCall1);
      CPPUNIT_ASSERT(hCheckCalls[0] != hCheckCalls[1]);

      // Tear down conference
      sipxConferenceDestroy(hConf);

      validatorCalling.validateCallMediaStopEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      validatorCalling.validateCallMediaStopEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Sides
      validatorCalled1.validateCallMediaStopEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      validatorCalled2.validateCallMediaStopEvents();
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst1,  UniversalEventValidatorCallback, &validatorCalling), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, AutoAnswerCallback, NULL), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, AutoAnswerCallback2, NULL), SIPX_RESULT_SUCCESS);

      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hLine), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine2), SIPX_RESULT_SUCCESS);
   }

   OsTask::delay(TEST_DELAY);
   checkForLeaks();
}


// Setup conference, remote drop party 1, remote drop party 2
void sipXtapiTestSuite::testConfBasic6()
{
   bool bRC;
   EventValidator validatorCalling("testConfBasic6.calling");
   EventValidator validatorCalled1("testConfBasic6.called1");
   EventValidator validatorCalled2("testConfBasic6.called2");

   for (int iStressFactor = 0; iStressFactor<STRESS_FACTOR; iStressFactor++)
   {
      printf("\ntestConfBasic6 (%2d of %2d)", iStressFactor+1, STRESS_FACTOR);

      SIPX_CALL hCall1;
      SIPX_CALL hCall2;
      SIPX_CONF hConf;
      SIPX_LINE hLine;
      SIPX_LINE hReceivingLine1;
      SIPX_LINE hReceivingLine2;
      SIPX_CALL hCallDestroy;

      SIPX_CALL hCheckCalls[10];
      size_t nCheckCalls;

      validatorCalling.reset();
      validatorCalled1.reset();
      validatorCalled2.reset();

      // Setup Auto-answer call back
      resetAutoAnswerCallback();
      resetAutoAnswerCallback2();
      sipxEventListenerAdd(g_hInst1, UniversalEventValidatorCallback, &validatorCalling);
      sipxEventListenerAdd(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1);
      sipxEventListenerAdd(g_hInst2, AutoAnswerCallback, NULL);
      sipxEventListenerAdd(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2);
      sipxEventListenerAdd(g_hInst3, AutoAnswerCallback2, NULL);

      // Create Lines
      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst1, "sip:foo@127.0.0.1:8000", &hLine), SIPX_RESULT_SUCCESS);
      bRC = validatorCalling.waitForLineEvent(hLine, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst2, "sip:foo@127.0.0.1:9100", &hReceivingLine1, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled1.waitForLineEvent(hReceivingLine1, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst3, "sip:foo@127.0.0.1:10000", &hReceivingLine2, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled2.waitForLineEvent(hReceivingLine2, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Setup first leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceCreate(g_hInst1, &hConf), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine, "sip:blah@127.0.0.1:9100", &hCall1), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalling.validateCallMediaStartEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_REMOTE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalled1.validateCallMediaStartEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 1);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall1);

      // Setup 2nd Leg
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine, "sip:blah2@127.0.0.1:10000", &hCall2), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalling.validateCallMediaStartEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_REMOTE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      validatorCalled2.validateCallMediaStartEvents();
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_ALERTING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 2);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall1);
      CPPUNIT_ASSERT(hCheckCalls[1] == hCall2);
      CPPUNIT_ASSERT(hCheckCalls[0] != hCheckCalls[1]);

      // Tear Down first leg (remotely)
      hCallDestroy = g_hAutoAnswerCallbackCall;
      CPPUNIT_ASSERT_EQUAL(sipxCallDestroy(&hCallDestroy), SIPX_RESULT_SUCCESS);

      validatorCalling.validateCallMediaStopEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall1, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Sides
      validatorCalled1.validateCallMediaStopEvents();
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 1);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall2);

      // Tear Down 2nd leg (remotely)
      hCallDestroy = g_hAutoAnswerCallbackCall2;
      CPPUNIT_ASSERT_EQUAL(sipxCallDestroy(&hCallDestroy), SIPX_RESULT_SUCCESS);

      validatorCalling.validateCallMediaStopEvents();
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Sides
      validatorCalled2.validateCallMediaStopEvents();
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled2.waitForCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Tear down conference
      CPPUNIT_ASSERT_EQUAL(sipxConferenceDestroy(hConf), SIPX_RESULT_SUCCESS);

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst1, UniversalEventValidatorCallback, &validatorCalling), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, AutoAnswerCallback, NULL), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, AutoAnswerCallback2, NULL), SIPX_RESULT_SUCCESS);

      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hLine), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine2), SIPX_RESULT_SUCCESS);

      OsTask::delay(TEST_DELAY);
   }

   OsTask::delay(TEST_DELAY);
   checkForLeaks();
}


// Setup conference and drop conference as a whole
void sipXtapiTestSuite::testConferenceDisplayName()
{
   bool bRC;
   EventValidator validatorCalling("testConferenceDisplayName.calling");
   EventValidator validatorCalled1("testConferenceDisplayName.called1");
   EventValidator validatorCalled2("testConferenceDisplayName.called2");

   for (int iStressFactor = 0; iStressFactor<STRESS_FACTOR; iStressFactor++)
   {
      printf("\ntestConferenceDisplayName (%2d of %2d)", iStressFactor+1, STRESS_FACTOR);

      SIPX_CALL hCall;
      SIPX_CALL hCall2;
      SIPX_CONF hConf;
      SIPX_LINE hLine;
      SIPX_LINE hLine_Alt1;
      SIPX_LINE hLine_Alt2;
      SIPX_LINE hReceivingLine1;
      SIPX_LINE hReceivingLine2;

      SIPX_CALL hCheckCalls[10];
      size_t nCheckCalls;

      validatorCalling.reset();
      validatorCalling.ignoreEventCategory(EVENT_CATEGORY_MEDIA);
      validatorCalled1.reset();
      validatorCalled1.ignoreEventCategory(EVENT_CATEGORY_MEDIA);
      validatorCalled2.reset();
      validatorCalled2.ignoreEventCategory(EVENT_CATEGORY_MEDIA);

      // Setup Auto-answer call back
      resetAutoAnswerCallback();
      resetAutoAnswerCallback2();
      sipxEventListenerAdd(g_hInst1, UniversalEventValidatorCallback, &validatorCalling);
      sipxEventListenerAdd(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1);
      sipxEventListenerAdd(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2);
      sipxEventListenerAdd(g_hInst2, AutoAnswerCallback, NULL);
      sipxEventListenerAdd(g_hInst3, AutoAnswerCallback2, NULL);

      // Create Lines
      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst1, "sip:foo@127.0.0.1:8000", &hLine), SIPX_RESULT_SUCCESS);
      bRC = validatorCalling.waitForLineEvent(hLine, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst2, "sip:foo@127.0.0.1:9100", &hReceivingLine1, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled1.waitForLineEvent(hReceivingLine1, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst3, "sip:xyzzy@127.0.0.1:10000", &hReceivingLine2, CONTACT_AUTO), SIPX_RESULT_SUCCESS);
      bRC = validatorCalled2.waitForLineEvent(hReceivingLine2, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);


      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst1, "\"Flibble Flabble\" <sip:bar@127.0.0.1:8000>", &hLine_Alt1), SIPX_RESULT_SUCCESS);
      bRC = validatorCalling.waitForLineEvent(hLine_Alt1, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // first, create a call

      createCall(hLine_Alt1, &hCall);
      sipxCallConnect(hCall, "sip:foo@127.0.0.1:9100");
      //CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hLine_Alt1), SIPX_RESULT_SUCCESS);

      bRC = validatorCalling.waitUntilCallEvent(hLine_Alt1, hCall, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side
      bRC = validatorCalled1.waitUntilCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxConferenceCreate(g_hInst1, &hConf), SIPX_RESULT_SUCCESS);

      CPPUNIT_ASSERT_EQUAL(sipxCallHold(hCall, TRUE), SIPX_RESULT_SUCCESS);
      OsTask::delay(CALL_DELAY);
      CPPUNIT_ASSERT_EQUAL(SIPX_RESULT_SUCCESS, sipxConferenceJoin(hConf, hCall));
      CPPUNIT_ASSERT_EQUAL(sipxCallUnhold(hCall, TRUE), SIPX_RESULT_SUCCESS);
      OsTask::delay(CALL_DELAY);

      bRC = validatorCalling.waitForCallEvent(hLine_Alt1, hCall, CALLSTATE_HELD, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine_Alt1, hCall, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_REMOTE_HELD, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled1.waitForCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      // reset events, we are not interested in hold/unhold check

      char szCallerId[256];
      sipxCallGetRemoteField(g_hAutoAnswerCallbackCall, szCallerId, 256);
      Url callerUrl(szCallerId);
      UtlString displayName;
      callerUrl.getDisplayName(displayName);

      const char* flabble = "\"Flibble Flabble\"";
      CPPUNIT_ASSERT(0 == strcmp(displayName, flabble));

      // Make sure we don't have any unexpected events
      CPPUNIT_ASSERT(!validatorCalling.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled1.validateNoWaitingEvent());
      CPPUNIT_ASSERT(!validatorCalled2.validateNoWaitingEvent());

      // Validate Get Calls
      CPPUNIT_ASSERT_EQUAL(sipxConferenceGetCalls(hConf, hCheckCalls, 10, &nCheckCalls), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(nCheckCalls == 1);
      CPPUNIT_ASSERT(hCheckCalls[0] == hCall);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst1, "\"Gooble Gobble\" <sip:ploo@127.0.0.1:8000>", &hLine_Alt2), SIPX_RESULT_SUCCESS);
      bRC = validatorCalling.waitForLineEvent(hLine_Alt2, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      sipxConferenceAdd(hConf, hLine_Alt2, "sip:xyzzy@127.0.0.1:10000", &hCall2);


      //CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hLine_Alt1), SIPX_RESULT_SUCCESS);
      bRC = validatorCalling.waitUntilCallEvent(hLine_Alt2, hCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side
      bRC = validatorCalled2.waitUntilCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);


      sipxCallGetRemoteField(g_hAutoAnswerCallbackCall2, szCallerId, 256);
      Url callerUrl2(szCallerId);
      callerUrl2.getDisplayName(displayName);

      const char* gooble = "\"Gooble Gobble\"";
      CPPUNIT_ASSERT(0 == strcmp(displayName, gooble));

      SIPX_CALL hDestroyed = hCall;
      sipxCallDestroy(&hCall);
      SIPX_CALL hDestroyed2 = hCall2;
      sipxCallDestroy(&hCall2);
      sipxConferenceDestroy(hConf);

      validatorCalling.waitUntilCallEvent(hLine_Alt1, hDestroyed, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL);
      validatorCalling.waitUntilCallEvent(hLine_Alt2, hDestroyed2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL);

      validatorCalled1.waitUntilCallEvent(g_hAutoAnswerCallbackLine, g_hAutoAnswerCallbackCall, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL);
      validatorCalled2.waitUntilCallEvent(g_hAutoAnswerCallbackLine2, g_hAutoAnswerCallbackCall2, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL);

      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst1, UniversalEventValidatorCallback, &validatorCalling), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, UniversalEventValidatorCallback, &validatorCalled1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, UniversalEventValidatorCallback, &validatorCalled2), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, AutoAnswerCallback, NULL), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst3, AutoAnswerCallback2, NULL), SIPX_RESULT_SUCCESS);

      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hLine), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine2), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hLine_Alt1), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hLine_Alt2), SIPX_RESULT_SUCCESS);
   }

   OsTask::delay(TEST_DELAY);
   checkForLeaks();
}


void sipXtapiTestSuite::testConferenceLegBusy()
{
   bool bRC;
   EventValidator validatorCalling("testConferenceLegBusy.calling");
   EventValidator validatorCalled("testConferenceLegBusy.calling");

   for (int iStressFactor = 0; iStressFactor<STRESS_FACTOR; iStressFactor++)
   {
      printf("\ntestConferenceLegBusy (%2d of %2d)", iStressFactor+1, STRESS_FACTOR);

      SIPX_CALL hCall;
      SIPX_LINE hLine;
      SIPX_LINE hReceivingLine;
      SIPX_CONF hConf;

      validatorCalling.reset();
      validatorCalling.ignoreEventCategory(EVENT_CATEGORY_MEDIA);
      validatorCalled.reset();
      validatorCalled.ignoreEventCategory(EVENT_CATEGORY_MEDIA);

      // Setup Auto-answer call back
      sipxEventListenerAdd(g_hInst2, AutoRejectCallback, NULL);
      sipxEventListenerAdd(g_hInst2, UniversalEventValidatorCallback, &validatorCalled);
      sipxEventListenerAdd(g_hInst1, UniversalEventValidatorCallback, &validatorCalling);

      sipxLineAdd(g_hInst2, "sip:foo@127.0.0.1:9100", &hReceivingLine, CONTACT_LOCAL);

      bRC = validatorCalled.waitForLineEvent(hReceivingLine, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxLineAdd(g_hInst1, "sip:bandreasen@pingtel.com", &hLine), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT(hLine);


      bRC = validatorCalling.waitForLineEvent(hLine, LINESTATE_PROVISIONED, LINESTATE_PROVISIONED_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      CPPUNIT_ASSERT_EQUAL(sipxConferenceCreate(g_hInst1, &hConf), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxConferenceAdd(hConf, hLine, "sip:blah@127.0.0.1:9100", &hCall), SIPX_RESULT_SUCCESS);

      // Validate Calling Side
      bRC = validatorCalling.waitForCallEvent(hLine, hCall, CALLSTATE_DIALTONE, CALLSTATE_CAUSE_CONFERENCE, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall, CALLSTATE_REMOTE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_BUSY, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalling.waitForCallEvent(hLine, hCall, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      // Validate Called Side (auto rejects)
      bRC = validatorCalled.waitForCallEvent(g_hAutoRejectCallbackLine, g_hAutoRejectCallbackCall, CALLSTATE_NEWCALL, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled.waitForCallEvent(g_hAutoRejectCallbackLine, g_hAutoRejectCallbackCall, CALLSTATE_OFFERING, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled.waitForCallEvent(g_hAutoRejectCallbackLine, g_hAutoRejectCallbackCall, CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_REJECTED, true);
      CPPUNIT_ASSERT(bRC);
      bRC = validatorCalled.waitForCallEvent(g_hAutoRejectCallbackLine, g_hAutoRejectCallbackCall, CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL, true);
      CPPUNIT_ASSERT(bRC);

      sipxConferenceDestroy(hConf);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst1, UniversalEventValidatorCallback, &validatorCalling), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, AutoRejectCallback, NULL), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxEventListenerRemove(g_hInst2, UniversalEventValidatorCallback, &validatorCalled), SIPX_RESULT_SUCCESS);

      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hLine), SIPX_RESULT_SUCCESS);
      CPPUNIT_ASSERT_EQUAL(sipxLineRemove(hReceivingLine), SIPX_RESULT_SUCCESS);
   }

   OsTask::delay(TEST_DELAY);

   checkForLeaks();
}
