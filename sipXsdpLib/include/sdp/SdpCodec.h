//  
// Copyright (C) 2006 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2007 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _SdpCodec_h_
#define _SdpCodec_h_

// SYSTEM INCLUDES
//#include <...>

// APPLICATION INCLUDES
#include <utl/UtlInt.h>
#include <utl/UtlString.h>

// DEFINES

// Mime major types
#define MIME_TYPE_AUDIO "audio"
#define MIME_TYPE_VIDEO "video"

// Mime Sub types
#define MIME_SUBTYPE_PCMU "PCMU"
#define MIME_SUBTYPE_PCMA "PCMA"
#define MIME_SUBTYPE_G729 "G729"
#define MIME_SUBTYPE_G723 "G723"
#define MIME_SUBTYPE_DTMF_TONES "telephone-event"
#define MIME_SUBTYPE_ILBC "iLBC"
#define MIME_SUBTYPE_GSM "GSM"
#define MIME_SUBTYPE_SPEEX "speex"
#define MIME_SUBTYPE_VP71 "VP71"
#define MIME_SUBTYPE_IYUV "IYUV"
#define MIME_SUBTYPE_I420 "I420"
#define MIME_SUBTYPE_RGB24 "RGB24"
#define MIME_SUBTYPE_H263 "H263"

// Bandwidth requirements for SDP Codecs
#define SDP_CODEC_BANDWIDTH_VARIABLE 0
#define SDP_CODEC_BANDWIDTH_LOW      1
#define SDP_CODEC_BANDWIDTH_NORMAL   2
#define SDP_CODEC_BANDWIDTH_HIGH     3

// Video formats - must be bitmap values
#define SDP_VIDEO_FORMAT_SQCIF       0x0001
#define SDP_VIDEO_FORMAT_QCIF        0x0002
#define SDP_VIDEO_FORMAT_CIF         0x0004
#define SDP_VIDEO_FORMAT_QVGA        0x0008

// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

/// Container for SDP/RTP codec specification
/**
*  This class holds the SDP definition of a codec
*  Included in this information is: sample rate,
*  number of channels, the mapping from an internal
*  pingtel codec id to the public SDP format and RTP
*  payload type id.
*
*  This is the base class.  Specific codec types
*  should implement sub classes which define the
*  codec specific parameters.  All specific codec
*  types MUST be registered with the SdpCodecFactory
*  to be useable.  Generally codecs are constructed
*  ONLY by the SdpCodecFactory.
*
*  The method that we have been using on
*  SdpCodec::getCodecType() retrieved
*  the static codec type/id.  I have defined
*  an enum in SdpCodec which contains the
*  current values as well as some additional
*  ones.  The idea is that these are private,
*  internally assigned ids to the codecs we
*  support.
*
*  I have added a new method
*  SdpCodec::getCodecPayloadFormat()
*  which returns the RTP payload id to be
*  used in RTP and the SDP.  For static
*  codec id the returned value for both of these
*  methods would typically be the same.
*  However for the dynamic codecs they
*  will mostly be different.
*
*  The intent is that eventually we will
*  support a factory which will allow
*  registration of new codec types.
*/
class SdpCodec : public UtlInt
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

    /// Unique identifier used for each supported codec
    /**
    *  Note it is possible that the format id/type used
    *  in the SDP "m" field and RTP header is different
    *  than these internally used ids.
    */
    enum SdpCodecTypes
    {
        SDP_CODEC_UNKNOWN = -1,    ///< dynamic payload id will be used
        SDP_CODEC_PCMU = 0,        ///< G.711 mu-law
        SDP_CODEC_GSM = 3,         ///< GSM codec
        SDP_CODEC_G723 = 4,
        SDP_CODEC_PCMA = 8,        ///< G.711 a-law
        SDP_CODEC_L16_STEREO = 10, ///< PCM 16 bit/sample 44100 samples/sec.
        SDP_CODEC_L16_MONO = 11,   ///< PCM 16 bit/sample 44100 samples/sec.
        SDP_CODEC_G729 = 18,       ///< G.729, with or without Annexes A or B
        SDP_CODEC_H263 = 34,       ///< H.263 video codec
        SDP_CODEC_MAXIMUM_STATIC_CODEC = 95,
        SDP_CODEC_TONES,     ///< AVT/DTMF Tones, RFC 2833
        SDP_CODEC_SPEEX,     ///< Speex, mono 8bits 8000samples/sec
        SDP_CODEC_SPEEX_5,   ///< Speex Profile 1
        SDP_CODEC_SPEEX_15,  ///< Speex Profile 2
        SDP_CODEC_SPEEX_24,  ///< Speex Profile 3
        SDP_CODEC_L16_8K,    ///< Mono PCM 16 bit/sample 8000 samples/sec.
        SDP_CODEC_ILBC,      ///< Internet Low Bit Rate Codec, 30ms (RFC3951)
        SDP_CODEC_ILBC_20MS, ///< Internet Low Bit Rate Codec, 20ms (RFC3951)

        // video codecs
        SDP_CODEC_VP71_CIF,
        SDP_CODEC_VP71_QCIF,
        SDP_CODEC_VP71_SQCIF,
        SDP_CODEC_VP71_QVGA,
        SDP_CODEC_IYUV_CIF,
        SDP_CODEC_IYUV_QCIF,
        SDP_CODEC_IYUV_SQCIF,
        SDP_CODEC_IYUV_QVGA,
        SDP_CODEC_I420_CIF,
        SDP_CODEC_I420_QCIF,
        SDP_CODEC_I420_SQCIF,
        SDP_CODEC_I420_QVGA,
        SDP_CODEC_RGB24_CIF,
        SDP_CODEC_RGB24_QCIF,
        SDP_CODEC_RGB24_SQCIF,
        SDP_CODEC_RGB24_QVGA,
        SDP_CODEC_H263_CIF,
        SDP_CODEC_H263_QCIF,
        SDP_CODEC_H263_SQCIF,
        SDP_CODEC_H263_QVGA,
    };


    /// Identifies the relative CPU cost for a SDP Codec.
    enum SdpCodecCPUCost
    {
       SDP_CODEC_CPU_LOW = 0,
       SDP_CODEC_CPU_HIGH = 1
    };

/* ============================ CREATORS ================================== */
///@name Creators
//@{

     ///Default constructor
   SdpCodec(enum SdpCodecTypes sdpCodecType = SDP_CODEC_UNKNOWN,
            int payloadId = SDP_CODEC_UNKNOWN, ///< if SDP_CODEC_UNKNOWN then it is dynamic
            const UtlString& sCodecName = NULL, ///< codec name which can be used in factory
            const char* mimeType = MIME_TYPE_AUDIO,
            const char* mimeSubtype = "",
            int sampleRate = 8000,             ///< samples per second
            int preferredPacketLength = 20000, ///< micro seconds
            int numChannels = 1,
            const char* formatSpecificData = "",
            const int CPUCost = SDP_CODEC_CPU_LOW,
            const int BWCost = SDP_CODEC_BANDWIDTH_NORMAL,
            const int videoFormat = SDP_VIDEO_FORMAT_QCIF,
            const int videoFmtp = 0);

     ///Copy constructor
   SdpCodec(const SdpCodec& rSdpCodec);

     ///Destructor
   virtual
   ~SdpCodec();

//@}

/* ============================ MANIPULATORS ============================== */
///@name Manipulators
//@{

     ///Assignment operator
   SdpCodec& operator=(const SdpCodec& rhs);

   UtlCopyableContainable* clone() const { return new SdpCodec(*this); }

//@}

/* ============================ ACCESSORS ================================= */
///@name Accessors
//@{

   /// Get the internal/Pingtel codec type id
   enum SdpCodecTypes getCodecType() const;
   /**<
   *  Note it is possible that the format id/type used
   *  in the SDP "m" field and RTP header is different
   *  than these internally used ids.
   */

   /// Get the SDP/RTP payload id to be used for this codec
   int getCodecPayloadId() const;
   /**<
   *  This is the id used in the SDP "m" format sub-field
   *  and RTP header.
   */

   /// Set the SDP/RTP payload id to be used for this codec
   void setCodecPayloadId(int formatId);

   /// Get the format specific parameters for the SDP
   virtual void getSdpFmtpField(UtlString& formatSpecificData) const;
   /**<
   *  This is what goes in the SDP "a" field in the
   *  format: "a=fmtp <payloadFormat> <formatSpecificData>
   */

   /// Get the media type for the codec
   void getMediaType(UtlString& mimeMajorType) const;
   /**<
   *  This is the mime major type (i.e. video, audio, etc)
   */

   /// MimeSubtype used as encoding name
   void getEncodingName(UtlString& mimeSubtype) const;
   /**<
   *  This is the encoding name used in the SDP
   *  "a=rtpmap: <payloadFormat> <mimeSubtype/sampleRate[/numChannels]"
   *  field.
   */

   /// Get the number of samples per second
   int getSampleRate() const;

   /// Return the video format
   int getVideoFormat() const;

   /// Get the number of channels
   int getNumChannels() const;

   /// Get the preferred packet size in micro seconds
   int getPacketLength() const;
   /**<
   *  Get the preferred (not mandated) packet
   *  size.  This measure is in microseconds and
   *  is independent of whether this is frame or
   *  sample based codec
   */

   /// Get a string dump of this codecs definition
   void toString(UtlString& sdpCodecContents) const;

   ///Get the CPU cost for this codec.
   int getCPUCost() const;
   /**<
   *  @returns SDP_CODEC_CPU_HIGH or SDP_CODEC_CPU_LOW
   */

   ///Get the bandwidth cost for this codec.
   int getBWCost() const;
   /**<
   *  @returns SDP_CODEC_BANDWIDTH_HIGH, SDP_CODEC_BANDWIDTH_NORMAL or
   *           SDP_CODEC_BANDWIDTH_LOW
   */

   ///Get the video format bitmap
   int getVideoFmtp() const;

   ///Set the video format bitmap
   void setVideoFmtp(const int videoFmtp);

   /// Get the video format string
   void getVideoFmtpString(UtlString& fmtpString) const;

   /// Set the video format string
   void setVideoFmtpString(int videoFmtp);

   /// Clears the format string
   void clearVideoFmtpString();

   ///Set the packet size
   void setPacketSize(const int packetSize);

   /** Gets codec name. This is not the same as mime subtype. */
   void getCodecName(UtlString& codecName) const { codecName = m_sCodecName; }
   UtlString getCodecName() const { return m_sCodecName; }

//@}

/* ============================ INQUIRY =================================== */
///@name Inquiry
//@{

   /// Returns TRUE if this codec is the same definition as the given codec
   UtlBoolean isSameDefinition(const SdpCodec& codec) const;
   /**<
   *  That is the encoding type and its characteristics, not the payload type.
   */

   /** Converts the short codec name into an enum */
   static SdpCodec::SdpCodecTypes getCodecType(const UtlString& shortCodecName);

//@}

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

//    enum SdpCodecTypes mCodecType; ///< Internal id
    int mCodecPayloadId;       ///< The id which appears in SDP & RTP
    UtlString mMimeType;           ///< audio, video, etc.
    UtlString mMimeSubtype;        ///< a=rtpmap mime subtype value
    int mSampleRate;               ///< samples per second
    int mPacketLength;             ///< micro seconds
    int mNumChannels;
    UtlString mFormatSpecificData; ///< a=fmtp parameter
    int mCPUCost;                  ///< relative cost of a SDP codec
    int mBWCost;
    int mVideoFormat;
    int mVideoFmtp;
    UtlString mVideoFmtpString;    ///< video format string
    UtlString m_sCodecName; ///< codec name which can be used in factory
};

/* ============================ INLINE METHODS ============================ */

#endif  // _SdpCodec_h_
