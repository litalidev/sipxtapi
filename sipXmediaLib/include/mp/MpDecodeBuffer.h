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

#ifndef _MpDecodeBuffer_h_
#define _MpDecodeBuffer_h_

// SYSTEM INCLUDES
// APPLICATION INCLUDES

#include <mp/MpDefs.h>
#if defined(ENABLE_WIDEBAND_AUDIO) && defined(HAVE_SPEEX)
#include <speex/speex_resampler.h>
#endif
#include "mp/MpRtpBuf.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
static const int JbPayloadMapSize = 128;
static const int g_decodeBufferSize = (9 * (sizeof(MpAudioSample) * SAMPLES_PER_FRAME)); // 9 packets, 20 mS each
                                               // or 3 packets 60 mS each.

// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class MpDecoderBase;
class MprDejitter;

/// Class for managing dejitter/decode of incoming RTP.
class MpDecodeBuffer
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */
///@name Creators
//@{

     /// Constructor
   MpDecodeBuffer(MprDejitter* pDejitter,
                  int samplesPerFrame, // samples per frame of flowgraph
                  int samplesPerSec); // samples per sec of flowgraph

     /// Destructor
   virtual ~MpDecodeBuffer();

//@}

/* ============================ MANIPULATORS ============================== */
///@name Manipulators
//@{

     /// Get samples from jitter buffer
   int getSamples(MpAudioSample *samplesBuffer, int samplesNumber);
     /**<
     *  @param voiceSamples - (out) buffer for audio samples
     *  @param samplesNumber - (in) number of samples to write
     *  
     *  @return Number of samples written to samplesBuffer
     */
   
     /// Set available decoders.
   int setCodecList(MpDecoderBase** decoderList, int decoderCount);
     /**<
     *  This function iterates through the provided list of decoders and fills
     *  internal payload type to decoder mapping. See payloadMap.
     *  
     *  @returns Always return 0 for now.
     */

//@}

/* ============================ ACCESSORS ================================= */
///@name Accessors
//@{

//@}

/* ============================ INQUIRY =================================== */
///@name Inquiry
//@{

//@}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   /// Push packet into jitter buffer.
   int pushPacket(MpRtpBufPtr &rtpPacket);
   /**<
   *  Packet will be decoded and decoded data will be copied to internal buffer.
   *  If no decoder is available for this packet's payload type packet will be
   *  ignored.
   *
   *  @note This implementation does not check packets sequence number in any
   *  manner. So it behave very bad if packets come reordered or if some
   *  packets are missed.
   */

     /// Copy constructor
   MpDecodeBuffer(const MpDecodeBuffer& rMpJitterBuffer);

     /// Assignment operator
   MpDecodeBuffer& operator=(const MpDecodeBuffer& rhs);

   int m_decodeBufferCount; ///< number of decoded samples available in buffer
   int m_decodeBufferIn; ///< offset for writing next sample
   int m_decodeBufferOut; ///< offset for reading decoded sample
   MpAudioSample m_decodeBuffer[g_decodeBufferSize]; // buffer for storing decoded samples

   MpDecoderBase* m_pDecoderMap[JbPayloadMapSize];
   MpDecoderBase* m_pDecoderList[JbPayloadMapSize + 1];
   MprDejitter* m_pDejitter; ///< instance of dejitter with jitter buffers

   int m_samplesPerFrame;
   int m_samplesPerSec; // flowgraph sample rate

#if defined(ENABLE_WIDEBAND_AUDIO) && defined(HAVE_SPEEX)
   SpeexResamplerState* m_pResamplerMap[JbPayloadMapSize];
   MpAudioSample m_tmpBuffer[g_decodeBufferSize]; // buffer for storing samples after decoding, but before resampling

#endif

   void destroyResamplers();
   void setupResamplers(MpDecoderBase** decoderList, int decoderCount);
};

/* ============================ INLINE METHODS ============================ */

#endif  // _MpDecodeBuffer_h_
