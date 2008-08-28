//
// Copyright (C) 2007-2008 Jaroslav Libak
// Licensed under the LGPL license.
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
// APPLICATION INCLUDES
#include <utl/UtlInt.h>
#include <utl/UtlString.h>
#include <sipxunit/TestUtilities.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>
#include <net/SipLine.h>
#include <net/Url.h>
#include <net/SipMessage.h>
#include <CompareHelper.h>

// DEFINES
#define USER_ENTERED_URI_1 "\"John Doe\"<sip:user1:password1@host1:5060;urlparm=value1?headerParam=value1>;fieldParam=value1"
#define USER_ENTERED_URI_2 "\"Jane Doe\"<sip:user2:password2@host2:5061;urlparm=value2?headerParam=value2>;fieldParam=value2"
#define USER_ID_1 "user1"
#define USER_ID_2 "user2"
#define IDENTITY_URI_1 "<sip:user1:password1@host1:5060;urlparm=value1"
#define IDENTITY_URI_2 "<sip:user2:password2@host2:5061;urlparm=value2>"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

class SipLineTest : public CppUnit::TestCase
{

   CPPUNIT_TEST_SUITE(SipLineTest);
   CPPUNIT_TEST(testSipLineAssignOperator);
   CPPUNIT_TEST(testSipLineCopyConstructor);
   CPPUNIT_TEST(testSipLineContainableType);
   CPPUNIT_TEST(testSipLineContainableHash);
   CPPUNIT_TEST(testSipLineContainableCompareTo);
   CPPUNIT_TEST(testSipLineId);
   CPPUNIT_TEST(testSipLineState);
   CPPUNIT_TEST(testSipLineUserId);
   CPPUNIT_TEST(testSipLineIdentityUri);
   CPPUNIT_TEST(testSipLineCanonicalUri);
   CPPUNIT_TEST(testSipLineRealm);
   CPPUNIT_TEST(testSipLineContact);
   CPPUNIT_TEST(testSipLineCredentials);
   CPPUNIT_TEST_SUITE_END();

private:

public:
   SipLineTest()
   {
   }

   ~SipLineTest()
   {
   }

   void setUp()
   {
   }

   void tearDown()
   {
   }

   void testSipLineAssignOperator()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLine line2(USER_ENTERED_URI_2, IDENTITY_URI_2);

      line2 = line1;
      CPPUNIT_ASSERT(areTheSame(line1, line2));
   }

   void testSipLineCopyConstructor()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLine line2(line1);

      CPPUNIT_ASSERT(areTheSame(line1, line2));
   }

   void testSipLineContainableType()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);

      CPPUNIT_ASSERT(areTheSame(line1.getContainableType(), "SipLine"));
   }

   void testSipLineContainableHash()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLine line2(USER_ENTERED_URI_2, IDENTITY_URI_2);
      SipLine line3 = line1;

      CPPUNIT_ASSERT(!areTheSame(line1.hash(), line2.hash()));
      CPPUNIT_ASSERT(areTheSame(line1.hash(), line3.hash()));
   }

   void testSipLineContainableCompareTo()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLine line2(USER_ENTERED_URI_2, IDENTITY_URI_2);

      int result1 = line1.compareTo(&line2);
      int result2 = line2.compareTo(&line1);

      CPPUNIT_ASSERT((result1 > 0 && result2 < 0) || (result1 < 0 && result2 > 0));
   }

   void testSipLineId()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLine line2(USER_ENTERED_URI_2, IDENTITY_URI_2);
      SipLine line3 = line1;

      CPPUNIT_ASSERT(!areTheSame(line1.getLineId(), line2.getLineId()));
      CPPUNIT_ASSERT(areTheSame(line1.getLineId(), line3.getLineId()));
   }

   void testSipLineState()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1, SipLine::LINE_STATE_DISABLED);

      CPPUNIT_ASSERT_EQUAL(line1.getState(), SipLine::LINE_STATE_DISABLED);
      line1.setState(SipLine::LINE_STATE_REGISTERED);
      CPPUNIT_ASSERT_EQUAL(line1.getState(), SipLine::LINE_STATE_REGISTERED);
   }

   void testSipLineUserId()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);

      CPPUNIT_ASSERT(areTheSame(line1.getUserId(), USER_ID_1));
   }

   void testSipLineIdentityUri()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);

      CPPUNIT_ASSERT(areTheSame(line1.getIdentityUri(), Url(IDENTITY_URI_1)));
   }

   void testSipLineCanonicalUri()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);

      CPPUNIT_ASSERT(areTheSame(line1.getCanonicalUrl(), Url(USER_ENTERED_URI_1)));
   }

   void testSipLineRealm()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);

      line1.addCredential("realm", "username", "password", "type");
      CPPUNIT_ASSERT(line1.realmExists("realm"));
   }

   void testSipLineContact()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      Url correctContact(USER_ENTERED_URI_1);
      correctContact.setPassword(NULL);
      correctContact.setPath(NULL);
      correctContact.setUrlParameter(SIP_LINE_IDENTIFIER, line1.getLineId());
      correctContact.includeAngleBrackets();

      CPPUNIT_ASSERT(areTheSame(line1.getPreferredContactUri(), correctContact));
   }

   void testSipLineCredentials()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);

      line1.addCredential("realm1", "username1", "password", "type");
      line1.addCredential("realm2", "username2", "password", "type");

      CPPUNIT_ASSERT(line1.getNumOfCredentials() == 2);
   }

};

CPPUNIT_TEST_SUITE_REGISTRATION(SipLineTest);
