//
// Copyright (C) 2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
//  
// Copyright (C) 2007 SIPfoundry Inc. 
// Licensed by SIPfoundry under the LGPL license. 
//  
// $$ 
////////////////////////////////////////////////////////////////////////////// 

#ifdef HAVE_ILBC // [

// SYSTEM INCLUDES
#include "assert.h"

// APPLICATION INCLUDES
#include "mp/MpeSipxILBC.h"
#include "mp/NetInTask.h"
#include <limits.h>  
extern "C" {
#include <iLBC_define.h>
#include <iLBC_encode.h>
}

const MpCodecInfo MpeSipxILBC::smCodecInfo30ms(
    SdpCodec::SDP_CODEC_ILBC,   // codecType
    "iLBC",                     // codecVersion
    false,                      // usesNetEq
    8000,                       // samplingRate
    8,                          // numBitsPerSample
    1,                          // numChannels
    240,                        // interleaveBlockSize
    13334,                      // bitRate. It doesn't matter right now.
    NO_OF_BYTES_30MS*8,         // minPacketBits
    NO_OF_BYTES_30MS*8,         // avgPacketBits
    NO_OF_BYTES_30MS*8,         // maxPacketBits
    240);                       // numSamplesPerFrame

const MpCodecInfo MpeSipxILBC::smCodecInfo20ms(
   SdpCodec::SDP_CODEC_ILBC_20MS,   // codecType
   "iLBC",                     // codecVersion
   false,                      // usesNetEq
   8000,                       // samplingRate
   8,                          // numBitsPerSample
   1,                          // numChannels
   160,                        // interleaveBlockSize
   13334,                      // bitRate. It doesn't matter right now.
   NO_OF_BYTES_20MS*8,         // minPacketBits
   NO_OF_BYTES_20MS*8,         // avgPacketBits
   NO_OF_BYTES_20MS*8,         // maxPacketBits
   160);                       // numSamplesPerFrame

MpeSipxILBC::MpeSipxILBC(int payloadType, int mode)
: MpEncoderBase(payloadType, mode == 20 ? &smCodecInfo20ms : &smCodecInfo30ms)
, mpState(NULL)
, mBufferLoad(0)
, m_mode(mode)
, m_samplesPerFrame(0)
, m_packetBytes(0)
{
   m_samplesPerFrame = getInfo()->getNumSamplesPerFrame();
   m_packetBytes = getInfo()->getMaxPacketBits() / 8;
}

MpeSipxILBC::~MpeSipxILBC()
{
   freeEncode();
}

OsStatus MpeSipxILBC::initEncode(void)
{
   assert(NULL == mpState);
   mpState = new iLBC_Enc_Inst_t();
   memset(mpState, 0, sizeof(*mpState));
   ::initEncode(mpState, m_mode);

   return OS_SUCCESS;
}

OsStatus MpeSipxILBC::freeEncode(void)
{
   delete mpState;
   mpState = NULL;

   return OS_SUCCESS;
}

OsStatus MpeSipxILBC::encode(const MpAudioSample* pAudioSamples,
                              const int numSamples,
                              int& rSamplesConsumed,
                              unsigned char* pCodeBuf,
                              const int bytesLeft,
                              int& rSizeInBytes,
                              UtlBoolean& sendNow,
                              MpAudioBuf::SpeechType& rAudioCategory)
{
   memcpy(&mpBuffer[mBufferLoad], pAudioSamples, sizeof(MpAudioSample)*numSamples);
   mBufferLoad += numSamples;
   assert(mBufferLoad <= m_samplesPerFrame);

   if (mBufferLoad == m_samplesPerFrame)
   {
      float buffer[240];
      for (unsigned int i = 0; i < m_samplesPerFrame; ++i)
         buffer[i] =  float(mpBuffer[i]);

      iLBC_encode((unsigned char*)pCodeBuf, buffer, mpState);

      mBufferLoad = 0;
      rSizeInBytes = m_packetBytes;
      sendNow = true;
   }
   else
   {
      rSizeInBytes = 0;
      sendNow = false;
   }

   rSamplesConsumed = numSamples;
   rAudioCategory = MpAudioBuf::MP_SPEECH_UNKNOWN;
   return OS_SUCCESS;
}

#endif // HAVE_ILBC ]
