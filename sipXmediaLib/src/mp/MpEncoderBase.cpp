//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


#include "mp/MpEncoderBase.h"

// must be the same as default VAD setting in sipXSdpLib codec factory
UtlBoolean MpEncoderBase::ms_bEnableVAD = TRUE;

/* ============================ CREATORS ================================== */

// Constructor
// Returns a new encoder object.
// param: payloadType - (in) RTP payload type associated with this encoder
MpEncoderBase::MpEncoderBase(int payloadType, const MpCodecInfo *pInfo) :
   mpCodecInfo(pInfo),
   mPayloadType(payloadType)
{
}
    
// Destructor
MpEncoderBase::~MpEncoderBase()
{
}

/* ============================ ACCESSORS ================================= */

// Get static information about the encoder
// Returns a pointer to an <i>MpCodecInfo</i> object that provides 
// static information about the encoder.
const MpCodecInfo* MpEncoderBase::getInfo(void) const
{
   return(mpCodecInfo);
}

//:Returns the RTP payload type associated with this encoder.
int MpEncoderBase::getPayloadType(void)
{
   return(mPayloadType);
}

UtlBoolean MpEncoderBase::isVADEnabled()
{
   return ms_bEnableVAD;
}

void MpEncoderBase::enableVAD(UtlBoolean bEnable)
{
   ms_bEnableVAD = bEnable;
}
