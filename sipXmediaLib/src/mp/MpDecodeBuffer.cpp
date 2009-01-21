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

#include "os/OsDefs.h"
#include "mp/MpDecodeBuffer.h"
#include "mp/MpDecoderBase.h"
#include "mp/MpMisc.h"
#include <mp/MprDejitter.h>

static int debugCount = 0;

/* ============================ CREATORS ================================== */

MpDecodeBuffer::MpDecodeBuffer(MprDejitter* pDejitter, int samplesPerFrame, int samplesPerSec)
: m_pDejitter(pDejitter)
, m_samplesPerFrame(samplesPerFrame)
, m_samplesPerSec(samplesPerSec)
, m_pNoiseState(NULL)
{
   for (int i=0; i<JbPayloadMapSize; i++)
      m_pDecoderMap[i] = NULL;

   memset(m_pDecoderList, 0, sizeof(m_pDecoderList));

#if defined(ENABLE_WIDEBAND_AUDIO) && defined(HAVE_SPEEX)
   memset(m_pResamplerMap, 0, sizeof(m_pResamplerMap));
   memset(m_resampleSrcBuffer, 0, sizeof(m_resampleSrcBuffer));
#endif

   m_decodeBufferCount = 0;
   m_decodeBufferIn = 0;
   m_decodeBufferOut = 0;

   debugCount = 0;

#ifdef HAVE_SPAN_DSP
   m_pNoiseState = noise_init_dbm0(NULL, 6513, NOISE_LEVEL, NOISE_CLASS_HOTH, 5);
#endif
}

// Destructor
MpDecodeBuffer::~MpDecodeBuffer()
{
   destroyResamplers();

#ifdef HAVE_SPAN_DSP
   if (m_pNoiseState)
   {
      free((void*)m_pNoiseState);
      m_pNoiseState = NULL;
   }
#endif
}

/* ============================ MANIPULATORS ============================== */

int MpDecodeBuffer::getSamples(MpAudioSample *samplesBuffer,
                               int requiredSamples) // required number of samples, for flowgraph sample rate
{
   if (m_pDejitter)
   {
      // first do actions that need to be done regardless of whether we have enough
      // samples
      m_pDejitter->frameIncrement();
      JitterBufferResult jbResult = MP_JITTER_BUFFER_ERROR;

      // now get more samples if we don't have enough of them
      if (m_decodeBufferCount < requiredSamples)
      {
         // we don't have enough samples. pull some from jitter buffer
         for (int i = 0; m_pDecoderList[i]; i++)
         {
            // loop through all decoders and pull frames for them
            int payloadType = m_pDecoderList[i]->getPayloadType();
            MpRtpBufPtr rtp = m_pDejitter->pullPacket(payloadType, jbResult);

            while (rtp.isValid())
            {
               // if buffer is valid, then there is undecoded data, we decode it
               // until we have enough samples here or jitter buffer has no more data
               // also pass jbResult, since codec may have PLC. In that case we will use codec PLC
               pushPacket(rtp, jbResult);

               if (m_decodeBufferCount < requiredSamples)
               {
                  // still don't have enough, pull more data
                  rtp = m_pDejitter->pullPacket(payloadType, jbResult);
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

   int suppliedSamples = 0;
   // Check we have some available decoded data
   if (m_decodeBufferCount != 0)
   {
      // We could not return more then we have
      suppliedSamples = min(requiredSamples, m_decodeBufferCount);
      int count1 = min(suppliedSamples, g_decodeBufferSize - m_decodeBufferOut); // samples to copy before wrap around occurs
      int count2 = suppliedSamples - count1; // number of samples to copy after wrap around
      memcpy(samplesBuffer, m_decodeBuffer + m_decodeBufferOut, count1 * sizeof(MpAudioSample));
      if (count2 > 0)
      {
         // handle wrap around, and copy the rest from the beginning of decode buffer
         memcpy(samplesBuffer + count1, m_decodeBuffer, count2 * sizeof(MpAudioSample));
      }

      m_decodeBufferCount -= suppliedSamples;
      m_decodeBufferOut += suppliedSamples;

      if (m_decodeBufferOut >= g_decodeBufferSize)
         m_decodeBufferOut -= g_decodeBufferSize;
   }

   if (suppliedSamples < requiredSamples)
   {
      int noiseFramesNeeded = requiredSamples - suppliedSamples;
      generateComfortNoise(samplesBuffer + suppliedSamples, noiseFramesNeeded);
   }

   return requiredSamples;
}


int MpDecodeBuffer::setCodecList(MpDecoderBase** decoderList, int decoderCount)
{
   memset(m_pDecoderList, 0, sizeof(m_pDecoderList));

   // For every payload type, load in a codec pointer, or a NULL if it isn't there
	for(int i = 0; (i < decoderCount) && (i < JbPayloadMapSize); i++)
   {
		int payloadType = decoderList[i]->getPayloadType();
   	m_pDecoderMap[payloadType] = decoderList[i];
      m_pDecoderList[i] = decoderList[i];
	}

   setupResamplers(decoderList, decoderCount);

   return 0;
}

int MpDecodeBuffer::pushPacket(MpRtpBufPtr &rtpPacket, JitterBufferResult jbResult)
{
   unsigned int availableBufferSize =0;          // number of samples could be written to decoded buffer
   unsigned producedSamples = 0; // number of samples, returned from decoder
   uint8_t payloadType = 0;     // RTP packet payload type
   MpDecoderBase* decoder = NULL;  // decoder for the packet

   payloadType = rtpPacket->getRtpPayloadType();

   // Ignore illegal payload types
   if (payloadType >= JbPayloadMapSize)
      return 0;

   // Get decoder
   decoder = m_pDecoderMap[payloadType];
   if (decoder == NULL)
      return 0; // If we can't decode it, we must ignore it?

   // Calculate space available for decoded samples
   if (m_decodeBufferIn > m_decodeBufferOut || m_decodeBufferCount == 0)
   {
      availableBufferSize = g_decodeBufferSize-m_decodeBufferIn;
   }
   else
   {
      availableBufferSize = m_decodeBufferOut-m_decodeBufferIn;
   }

   MpAudioSample* pTmpDstBuffer = NULL;
#if defined(ENABLE_WIDEBAND_AUDIO) && defined(HAVE_SPEEX)
   pTmpDstBuffer = m_resampleSrcBuffer;
#else
   pTmpDstBuffer = m_decodeHelperBuffer;
#endif
   // decode samples from decoder, and copy them into decode buffer
   producedSamples = decoder->decode(rtpPacket, g_decodeHelperBufferSize, pTmpDstBuffer,
      jbResult == MP_JITTER_BUFFER_PLC);

#if defined(ENABLE_WIDEBAND_AUDIO) && defined(HAVE_SPEEX)
   if (decoder->getInfo()->getCodecType() != SdpCodec::SDP_CODEC_TONES &&
      decoder->getInfo()->getSamplingRate() != m_samplesPerSec)
   {
      // need to resample
      SpeexResamplerState* pResamplerState = m_pResamplerMap[payloadType];
      if (pResamplerState)
      {
         if (producedSamples > 0)
         {
            unsigned int resampleSrcCount = producedSamples;// speex will overwrite resampleSrcCount
            unsigned int availableHelperBufferSize = g_decodeHelperBufferSize;
            int err = speex_resampler_process_int(pResamplerState, 0,
               pTmpDstBuffer, &resampleSrcCount,
               m_decodeHelperBuffer, &availableHelperBufferSize);
            assert(!err);
            producedSamples = availableHelperBufferSize; // speex overwrites availableHelperBufferSize with number of output samples
         }
      }
      else
      {
         // we can't resample, and can't decode
         assert(false);
      }
   }
#endif
   int addedSamples = 0;
   // now we have decoded & resampled samples in m_decodeHelperBuffer, with decodedSamples count
   // copy them into main buffer
   if (producedSamples > 0)
   {
      // count1 is number of samples to copy before wrapping occurs
      int count1 = min(producedSamples, availableBufferSize);
      // count 2 is number of samples to copy after wrapping
      int count2 = producedSamples - count1;
      memcpy(m_decodeBuffer + m_decodeBufferIn, m_decodeHelperBuffer, count1 * sizeof(MpAudioSample));
      addedSamples += count1;
      if (count2 > 0)
      {
         count2 = min(count2, m_decodeBufferOut); // reduce count2 by available space since start of array
         memcpy(m_decodeBuffer, m_decodeHelperBuffer + count1, count2 * sizeof(MpAudioSample)); // copy to beginning of buffer
         addedSamples += count2;
      }
   }

   // Update buffer state
   m_decodeBufferCount += addedSamples;
   m_decodeBufferIn += addedSamples;
   // Reset write pointer if we reach end of buffer
   if (m_decodeBufferIn >= g_decodeBufferSize)
      m_decodeBufferIn -= g_decodeBufferSize;

   return 0;
}

void MpDecodeBuffer::destroyResamplers()
{
#if defined(ENABLE_WIDEBAND_AUDIO) && defined(HAVE_SPEEX)
   for (int i = 0; i < JbPayloadMapSize; i++)
   {
      if (m_pResamplerMap[i])
      {
         speex_resampler_destroy(m_pResamplerMap[i]);
         m_pResamplerMap[i] = NULL;
      }      
   }
#endif
}

void MpDecodeBuffer::setupResamplers(MpDecoderBase** decoderList, int decoderCount)
{
#if defined(ENABLE_WIDEBAND_AUDIO) && defined(HAVE_SPEEX)
   destroyResamplers();

   for(int i = 0; (i < decoderCount) && (i < JbPayloadMapSize); i++)
   {
      int payloadType = decoderList[i]->getPayloadType();
      if (!m_pResamplerMap[payloadType])
      {
         int error;
         m_pResamplerMap[payloadType] = speex_resampler_init(1,
            decoderList[i]->getInfo()->getSamplingRate(), // resample from codec sample rate
            m_samplesPerSec, // into flowgraph sample rate
            SPEEX_RESAMPLER_QUALITY_VOIP, &error);
      }
   }
#endif
}

void MpDecodeBuffer::generateComfortNoise(MpAudioSample *samplesBuffer, unsigned sampleCount)
{
   UtlBoolean bGenerated = FALSE;
#ifdef HAVE_SPAN_DSP
   if (m_pNoiseState && samplesBuffer)
   {
      for (int i = 0; i < sampleCount; i++)
      {
         samplesBuffer[i] = noise(m_pNoiseState); // generate 1 sample of noise
      }
      bGenerated = TRUE;
   }
#endif

   if (!bGenerated)
   {
      // set all 0s
      memset(samplesBuffer, 0, sampleCount * sizeof(MpAudioSample));
   }
}
