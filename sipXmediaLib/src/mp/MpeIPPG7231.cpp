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


#ifdef HAVE_INTEL_IPP // [

#include "assert.h"
// APPLICATION INCLUDES
#include "winsock2.h"
#include "mp/MpeIPPG7231.h"

extern "C" {
#include "ippcore.h"
#include "ipps.h"
#include "usccodec.h"
}

const MpCodecInfo MpeIPPG7231::smCodecInfo(
   SdpCodec::SDP_CODEC_G723,     // codecType
   "Intel IPP 5.3",              // codecVersion
   true,                         // usesNetEq
   8000,                         // samplingRate
   16,                           // numBitsPerSample
   1,                            // numChannels
   80,                           // interleaveBlockSize
   8000,                         // bitRate
   20*8,                          // minPacketBits
   20*8,                          // avgPacketBits
   24*8,                          // maxPacketBits
   80);                         // numSamplesPerFrame

MpeIPPG7231::MpeIPPG7231(int payloadType)
: MpEncoderBase(payloadType, &smCodecInfo)
, mStoredFramesCount(0)
, mpStoredFramesBuffer(NULL)
, mEncodedBuffer(NULL)
{
   codec5300 = (LoadedCodec*)malloc(sizeof(LoadedCodec));
   memset(codec5300, 0, sizeof(LoadedCodec));
   codec6300 = (LoadedCodec*)malloc(sizeof(LoadedCodec));
   memset(codec6300, 0, sizeof(LoadedCodec));
}

MpeIPPG7231::~MpeIPPG7231()
{
   freeEncode();
   free(codec5300);
   free(codec6300);
}

OsStatus MpeIPPG7231::initEncode(void)
{
   int lCallResult;

   ippStaticInit();
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

   lCallResult = GetUSCCodecParamsByFormat(codec5300, BY_NAME, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // Set params for encode
   USC_PCMType streamType;
   streamType.bitPerSample = getInfo()->getNumBitsPerSample();
   streamType.nChannels = getInfo()->getNumChannels();
   streamType.sample_frequency = getInfo()->getSamplingRate();

   lCallResult = SetUSCEncoderPCMType(&codec6300->uscParams, LINEAR_PCM, &streamType, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // instead of SetUSCEncoderParams(...)
   codec6300->uscParams.pInfo->params.direction = USC_ENCODE;
   codec6300->uscParams.pInfo->params.law = 0;
   codec6300->uscParams.pInfo->params.modes.bitrate = 6300;
   codec6300->uscParams.pInfo->params.modes.vad = 1;    

   // Set params for encode
   lCallResult = SetUSCEncoderPCMType(&codec5300->uscParams, LINEAR_PCM, &streamType, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // instead of SetUSCEncoderParams(...)
   codec5300->uscParams.pInfo->params.direction = USC_ENCODE;
   codec5300->uscParams.pInfo->params.law = 0;
   codec5300->uscParams.pInfo->params.modes.bitrate = 5300;
   codec5300->uscParams.pInfo->params.modes.vad = 1;

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
   lCallResult = USCEncoderInit(&codec6300->uscParams, NULL, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   lCallResult = USCEncoderInit(&codec5300->uscParams, NULL, NULL);
   if (lCallResult < 0)
   {
      return OS_FAILED;
   }

   // Allocate memory for the input buffer. Size of output buffer is equal
   // to the size of 1 frame
   mpStoredFramesBuffer = 
         ippsMalloc_8s(codec6300->uscParams.pInfo->params.framesize);

   ippsSet_8u(0, (Ipp8u *)mpStoredFramesBuffer,
              codec6300->uscParams.pInfo->params.framesize);

   mStoredFramesCount = 0;

   return OS_SUCCESS;
}

OsStatus MpeIPPG7231::freeEncode(void)
{
   // Free codec memory
   USCFree(&codec6300->uscParams);
   USCFree(&codec5300->uscParams);
   ippsFree(mpStoredFramesBuffer);

   return OS_SUCCESS;
}


OsStatus MpeIPPG7231::encode(const short* pAudioSamples,
                            const int numSamples,
                            int& rSamplesConsumed,
                            unsigned char* pCodeBuf,
                            const int bytesLeft,
                            int& rSizeInBytes,
                            UtlBoolean& sendNow,
                            MpAudioBuf::SpeechType& rAudioCategory)
{
   assert(80 == numSamples);

   if (mStoredFramesCount == 2)
   {
      ippsCopy_8u((unsigned char *)pAudioSamples,
                  (unsigned char *)mpStoredFramesBuffer+mStoredFramesCount*160,
                  numSamples*sizeof(MpAudioSample));     


      int frmlen, infrmLen, FrmDataLen;
      USC_PCMStream PCMStream;
      USC_Bitstream Bitstream;

      // Do the pre-procession of the frame
      infrmLen = USCEncoderPreProcessFrame(&codec5300->uscParams,
                                           mpStoredFramesBuffer,
                                           mEncodedBuffer,
                                           &PCMStream,
                                           &Bitstream);
      // Encode one frame
      FrmDataLen = USCCodecEncode(&codec5300->uscParams,
                                  &PCMStream,
                                  &Bitstream,
                                  0);
      if(FrmDataLen < 0)
      {
         return OS_FAILED;
      }

      infrmLen += FrmDataLen;
      // Do the post-procession of the frame
      frmlen = USCEncoderPostProcessFrame(&codec5300->uscParams,
                                          mpStoredFramesBuffer,
                                          mEncodedBuffer,
                                          &PCMStream,
                                          &Bitstream);


      ippsSet_8u(0, pCodeBuf, 20); 

      if (Bitstream.nbytes <= 20)
      {
         for(int k = 0; k < Bitstream.nbytes; ++k)
         {
            pCodeBuf[k] = mEncodedBuffer[6 + k];
         }
      }
      else
      {
         frmlen =0;
      }

      sendNow = TRUE;

      rAudioCategory = MpAudioBuf::MP_SPEECH_UNKNOWN;
      rSamplesConsumed = numSamples;
      mStoredFramesCount = 0;

      ippsSet_8u(0, (unsigned char *)mpStoredFramesBuffer,
                 codec5300->uscParams.pInfo->params.framesize);

      if (Bitstream.nbytes <= 20)
      {
         rSizeInBytes = Bitstream.nbytes;
      }
      else
      {
         rSizeInBytes = 0;
      }
   }
   else
   {
      ippsCopy_8u((unsigned char *)pAudioSamples,
                  (unsigned char *)mpStoredFramesBuffer+mStoredFramesCount*160,
                  numSamples * sizeof(MpAudioSample));  

      mStoredFramesCount++;

      sendNow = FALSE;
      rSizeInBytes = 0;
      rSamplesConsumed = numSamples;
      rAudioCategory = MpAudioBuf::MP_SPEECH_UNKNOWN;
   }

   return OS_SUCCESS;
}

#endif // HAVE_INTEL_IPP ]
