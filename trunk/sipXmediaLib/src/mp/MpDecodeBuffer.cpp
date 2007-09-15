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


#include "assert.h"
#include "string.h"

#include "mp/MpDecodeBuffer.h"
#include "mp/MpDecoderBase.h"
#include "mp/MpMisc.h"
#include <mp/MprDejitter.h>

static int debugCount = 0;

/* ============================ CREATORS ================================== */

MpDecodeBuffer::MpDecodeBuffer(MprDejitter* pDejitter)
: m_pMyDejitter(pDejitter)
{
   for (int i=0; i<JbPayloadMapSize; i++)
      payloadMap[i] = NULL;

   memset(m_pDecoderList, 0, sizeof(m_pDecoderList));

   JbQCount = 0;
   JbQIn = 0;
   JbQOut = 0;

   debugCount = 0;
}

// Destructor
MpDecodeBuffer::~MpDecodeBuffer()
{
}

/* ============================ MANIPULATORS ============================== */

int MpDecodeBuffer::getSamples(MpAudioSample *samplesBuffer, int samplesNumber)
{
   if (m_pMyDejitter)
   {
      // first do actions that need to be done regardless of whether we have enough
      // samples
      for (int i = 0; m_pDecoderList[i]; i++)
      {
         m_pDecoderList[i]->frameIncrement();
      }

      // now get more samples if we don't have enough of them
      if (JbQCount < samplesNumber)
      {
         // we don't have enough samples. pull some from jitter buffer
         for (int i = 0; m_pDecoderList[i]; i++)
         {
            // loop through all decoders and pull frames for them
            int payloadType = m_pDecoderList[i]->getPayloadType();
            MpRtpBufPtr rtp = m_pMyDejitter->pullPacket(payloadType);

            while (rtp.isValid())
            {
               // if buffer is valid, then there is undecoded data, we decode it
               // until we have enough samples here or jitter buffer has no more data
               pushPacket(rtp);

               if (JbQCount < samplesNumber)
               {
                  // still don't have enough, pull more data
                  rtp = m_pMyDejitter->pullPacket(payloadType);
               }
               else
               {
                  break;
                  // we cant break out of for loop, as we need to process rfc2833 too
               }
            }  
         }  
      }
   }
      
   // Check we have some available decoded data
   if (JbQCount != 0)
   {
      // We could not return more then we have
      samplesNumber = min(samplesNumber, JbQCount);

      memcpy(samplesBuffer, JbQ + JbQOut, samplesNumber * sizeof(MpAudioSample));

      JbQCount -= samplesNumber;
      JbQOut += samplesNumber;

      if (JbQOut >= JbQueueSize)
         JbQOut -= JbQueueSize;
   }

   return samplesNumber;
}


int MpDecodeBuffer::setCodecList(MpDecoderBase** codecList, int codecCount)
{
   memset(m_pDecoderList, 0, sizeof(m_pDecoderList));

   // For every payload type, load in a codec pointer, or a NULL if it isn't there
	for(int i = 0; (i < codecCount) && (i < JbPayloadMapSize); i++)
   {
		int payloadType = codecList[i]->getPayloadType();
   	payloadMap[payloadType] = codecList[i];
      m_pDecoderList[i] = codecList[i];
	}

   return 0;
}

int MpDecodeBuffer::pushPacket(MpRtpBufPtr &rtpPacket)
{
   int bufferSize;          // number of samples could be written to decoded buffer
   unsigned decodedSamples; // number of samples, returned from decoder
   uint8_t payloadType;     // RTP packet payload type
   MpDecoderBase* decoder;  // decoder for the packet

   payloadType = rtpPacket->getRtpPayloadType();

   // Ignore illegal payload types
   if (payloadType >= JbPayloadMapSize)
      return 0;

   // Get decoder
   decoder = payloadMap[payloadType];
   if (decoder == NULL)
      return 0; // If we can't decode it, we must ignore it?

   // Calculate space available for decoded samples
   if (JbQIn > JbQOut || JbQCount == 0)
   {
      bufferSize = JbQueueSize-JbQIn;
   } else {
      bufferSize = JbQOut-JbQIn;
   }
   // Decode packet
   decodedSamples = decoder->decode(rtpPacket, bufferSize, JbQ+JbQIn);
   // TODO:: If packet jitter buffer size is not integer multiple of decoded size,
   //        then part of the packet will be lost here. We should consider one of
   //        two ways: set JB size on creation depending on packet size, reported 
   //        by codec, OR push packet into decoder and then pull decoded data in
   //        chunks.

   // Update buffer state
   JbQCount += decodedSamples;
   JbQIn += decodedSamples;
   // Reset write pointer if we reach end of buffer
   if (JbQIn >= JbQueueSize)
      JbQIn = 0;

   return 0;
}
