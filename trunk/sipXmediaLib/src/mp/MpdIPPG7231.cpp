//
// Copyright (C) 2005 Pingtel Corp.
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

#ifdef HAVE_INTEL_IPP /* [ */

// APPLICATION INCLUDES
#include "mp/MpdIPPG7231.h"

extern "C" {
#include "ippcore.h"
#include "ipps.h"
#include "usccodec.h"
}

#define G723_PATTERN_LENGTH_5300 20
#define G723_PATTERN_LENGTH_6300 24

const MpCodecInfo MpdIPPG7231::smCodecInfo(
   SdpCodec::SDP_CODEC_G7231,    // codecType
   "Intel IPP 5.3",             // codecVersion
   true,                        // usesNetEq
   8000,                        // samplingRate
   16,                          // numBitsPerSample (not used)
   1,                           // numChannels
   160,                          // interleaveBlockSize
   8000,                        // bitRate
   20*8,                         // minPacketBits
   20*8,                         // avgPacketBits
   192,                         // maxPacketBits
   160,                          // numSamplesPerFrame
   6);                          // preCodecJitterBufferSize (should be adjusted)

MpdIPPG7231::MpdIPPG7231(int payloadType)
: MpDecoderBase(payloadType, &smCodecInfo)
{
   codec5300 = (LoadedCodec*)malloc(sizeof(LoadedCodec));
   memset(codec5300, 0, sizeof(LoadedCodec));
   codec6300 = (LoadedCodec*)malloc(sizeof(LoadedCodec));
   memset(codec6300, 0, sizeof(LoadedCodec));
}

MpdIPPG7231::~MpdIPPG7231()
{
   freeDecode();
   free(codec5300);
   free(codec6300);
}

OsStatus MpdIPPG7231::initDecode()
{
   int lCallResult;

   ippStaticInit();

   // Apply codec name and VAD to codec definition structure
   strcpy((char*)codec6300->codecName, "IPP_G723.1");
   codec6300->lIsVad = 1;
   strcpy((char*)codec5300->codecName, "IPP_G723.1");
   codec5300->lIsVad = 1;

   // Load codec by name from command line
   lCallResult = LoadUSCCodecByName(codec6300,NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   lCallResult = LoadUSCCodecByName(codec5300,NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // Get USC codec params
   lCallResult = USCCodecAllocInfo(&codec6300->uscParams, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   lCallResult = USCCodecAllocInfo(&codec5300->uscParams, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   lCallResult = USCCodecGetInfo(&codec6300->uscParams, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   lCallResult = USCCodecGetInfo(&codec5300->uscParams, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // Get its supported format details
   lCallResult = GetUSCCodecParamsByFormat(codec6300, BY_NAME, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // Get its supported format details
   lCallResult = GetUSCCodecParamsByFormat(codec5300, BY_NAME, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // Set params for decode
   USC_PCMType streamType;
   streamType.bitPerSample = getInfo()->getNumBitsPerSample();
   streamType.nChannels = getInfo()->getNumChannels();
   streamType.sample_frequency = getInfo()->getSamplingRate();

   // decoder doesn't need to know PCM type
   lCallResult = SetUSCDecoderPCMType(&codec6300->uscParams, -1, &streamType, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // instead of SetUSCDecoderParams(...)
   codec6300->uscParams.pInfo->params.direction = USC_DECODE;
   codec6300->uscParams.pInfo->params.law = 0;
   codec6300->uscParams.pInfo->params.modes.bitrate = 6300;
   codec6300->uscParams.pInfo->params.modes.vad =1;

   // decoder doesn't need to know PCM type
   lCallResult = SetUSCDecoderPCMType(&codec5300->uscParams, -1, &streamType, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // instead of SetUSCDecoderParams(...)
   codec5300->uscParams.pInfo->params.direction = USC_DECODE;
   codec5300->uscParams.pInfo->params.law = 0;
   codec5300->uscParams.pInfo->params.modes.bitrate = 5300;
   codec5300->uscParams.pInfo->params.modes.vad =1;

   // Alloc memory for the codec
   lCallResult = USCCodecAlloc(&codec6300->uscParams, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   lCallResult = USCCodecAlloc(&codec5300->uscParams, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // Init decoder
   lCallResult = USCDecoderInit(&codec6300->uscParams, NULL, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   lCallResult = USCDecoderInit(&codec5300->uscParams, NULL, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   return OS_SUCCESS;
}

OsStatus MpdIPPG7231::freeDecode(void)
{
   // Free codec memory
   USCFree(&codec6300->uscParams);
   USCFree(&codec5300->uscParams);

   return OS_SUCCESS;
}

int MpdIPPG7231::decode(const MpRtpBufPtr &rtpPacket,
                       unsigned int decodedBufferLength,
                       MpAudioSample *samplesBuffer) 
{

   int infrmLen, FrmDataLen;
   unsigned int decodedSamples;

   infrmLen = rtpPacket->getPayloadSize();

   // Prepare encoded buffer parameters
   if (infrmLen == G723_PATTERN_LENGTH_6300)
   {
      Bitstream.bitrate = 6300;
      Bitstream.frametype = 0;
      Bitstream.nbytes = 24;
      decodedSamples = 240;
   }
   else if (infrmLen == G723_PATTERN_LENGTH_5300)
   {
      Bitstream.bitrate = 5300;
      Bitstream.frametype = 0;
      Bitstream.nbytes = 20;
      decodedSamples = 240;
   } else return 0; // we ignore SID frames 4 bytes long, should generate comfort noise

   if (decodedBufferLength < decodedSamples)
   {
      osPrintf("MpdIPPG723::decode: Jitter buffer overloaded. Glitch!\n");
      return 0;
   }

   Bitstream.pBuffer = const_cast<char*>(rtpPacket->getDataPtr());
   PCMStream.pBuffer = reinterpret_cast<char*>(samplesBuffer);

   // Decode one frame
   if (infrmLen == G723_PATTERN_LENGTH_6300)
   {
      FrmDataLen = USCCodecDecode(&codec6300->uscParams, &Bitstream,
                                  &PCMStream, 0);
   }
   else
   {
      FrmDataLen = USCCodecDecode(&codec5300->uscParams, &Bitstream,
                                  &PCMStream, 0);
   }

   if (FrmDataLen < 0)
   {
      return 0;
   }

   // Return number of decoded samples
   return decodedSamples;
}

int MpdIPPG7231::decodeIn(const MpRtpBufPtr &rtpPacket)
{
   unsigned payloadSize = rtpPacket->getPayloadSize();

   if (payloadSize == G723_PATTERN_LENGTH_6300 ||
       payloadSize == G723_PATTERN_LENGTH_5300)
   {
      return payloadSize;
   }
   else
   {
      osPrintf("MpdIPPG723: Rejecting rtpPacket of size %i\n", payloadSize);
      return -1;
   }
}


#endif /* !HAVE_INTEL_IPP ] */
