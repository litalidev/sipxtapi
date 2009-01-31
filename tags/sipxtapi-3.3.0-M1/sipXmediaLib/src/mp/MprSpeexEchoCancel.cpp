//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2006 ProfitFuel Inc.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//  
// Copyright (C) 2006 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_SPEEX // [

// SYSTEM INCLUDES
#include <speex/speex_echo.h>
#include <speex/speex_types.h>

// APPLICATION INCLUDES
#include "os/OsDefs.h"
#include "mp/MpMisc.h"
#include "mp/MpBuf.h"
#include "mp/MpBufPool.h"
#include "mp/MpBufferMsg.h"
#include "mp/MprSpeexEchoCancel.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
MprSpeexEchoCancel::MprSpeexEchoCancel(const UtlString& rName,
                                       int samplesPerFrame,
                                       int samplesPerSec,
                                       int filterLength,
                                       int echoQueueLatency)
: MpAudioResource(rName, 1, 1, 1, 2, samplesPerFrame, samplesPerSec)
, m_echoQueueLatency(echoQueueLatency)
{
   //Initilize Speex Echo state with framesize and number of frames for length of buffer
   mpEchoState = speex_echo_state_init(samplesPerFrame, 
                                       samplesPerSec*filterLength/1000);

   mStartedCanceling = false; // Debug Use only
}

// Destructor
MprSpeexEchoCancel::~MprSpeexEchoCancel()
{
   if (mpEchoState)
   {
      speex_echo_state_destroy(mpEchoState);
      mpEchoState = NULL;
   }
}

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

SpeexEchoState* MprSpeexEchoCancel::getSpeexEchoState()
{
   return mpEchoState;
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

UtlBoolean MprSpeexEchoCancel::doProcessFrame(MpBufPtr inBufs[],
                                              MpBufPtr outBufs[],
                                              int inBufsSize,
                                              int outBufsSize,
                                              UtlBoolean isEnabled,
                                              int samplesPerFrame,
                                              int samplesPerSecond)
{
   MpAudioBufPtr   outBuffer;
   MpAudioBufPtr   inputBuffer;
   MpAudioBufPtr   echoRefBuffer;
   MpBufferMsg*    bufferMsg;
   bool            res = false;

   // We don't need to do anything if we don't have an output.
   if (!isOutputConnected(0))
      return FALSE;

   // If disabled pass buffer through
   if (!isEnabled)
   {
      outBufs[0].swap(inBufs[0]);
      return TRUE;
   }

   // Get incoming data
   inputBuffer.swap(inBufs[0]);

   // If the object is not enabled or we don't have valid input,
   // pass input to output
   if (inputBuffer.isValid()
       && (inputBuffer->getSamplesNumber() == samplesPerFrame))
   {
      // This buffer will be modified in place. Make sure we're the only owner.
      res = inputBuffer.requestWrite();
      assert(res);

      int echoMsgs = MpMisc.m_pEchoQ->numMsgs();
      // Try to get a reference frame for echo cancellation.  21 = MAX_SPKR_BUFFERS(12) +
      if (echoMsgs >= m_echoQueueLatency)
      {
         // Flush queue to prevent old frames being used in echo canceller. We need to 
         // pass echo frame from speaker before it is read from microphone
         while ((MpMisc.m_pEchoQ->receive((OsMsg*&) bufferMsg, OsTime::NO_WAIT_TIME) == OS_SUCCESS)
               && MpMisc.m_pEchoQ->numMsgs() > m_echoQueueLatency)
         {
            bufferMsg->releaseMsg();
         }

         // Get buffer from the message and free message
         echoRefBuffer = bufferMsg->getBuffer();
         assert(echoRefBuffer.isValid());
         bufferMsg->releaseMsg();

         if (echoRefBuffer->getSamplesNumber() == samplesPerFrame)
         {
            mStartedCanceling = true;

            // Get new buffer
            outBuffer = MpMisc.m_pRawAudioPool->getBuffer();
            assert(outBuffer.isValid());
            outBuffer->setSamplesNumber(samplesPerFrame);
            outBuffer->setSpeechType(inputBuffer->getSpeechType());

            // Do echo cancelation
            speex_echo_cancellation(mpEchoState,
                              (spx_int16_t*)inputBuffer->getSamplesPtr(),
                              (spx_int16_t*)echoRefBuffer->getSamplesPtr(),
                              (spx_int16_t*)outBuffer->getSamplesPtr());
         }
         else
         {
            //The sample count didn't match so we can't echo cancel.  Pass the frame.
            outBuffer = inputBuffer;
            assert(!mStartedCanceling);
         }
      }
      else
      {
         // There was no speaker data to match.  Pass the frame.
         outBuffer = inputBuffer;
//         osPrintf("SpeexEchoCancel: No frame to match...\n");
      }

      outBufs[0].swap(outBuffer);
   }
   else
   {
      outBufs[0].swap(inputBuffer);
   }

   return TRUE;
}

/* ============================ FUNCTIONS ================================= */

#endif // HAVE_SPEEX ]

