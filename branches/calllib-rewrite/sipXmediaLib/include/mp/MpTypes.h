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

#ifndef _INCLUDED_MPTYPES_H /* [ */
#define _INCLUDED_MPTYPES_H

/**************************************************************************/
/* One of these should be defined, and the other should undefined!!!!!    */
/* These are used to determine whether 16 bit samples need to be byte     */
/* swapped.  All hosts (SA1100 and WinTel) are currently little endian    */
#ifdef _VXWORKS /* [ */
#undef  _BYTE_ORDER_IS_LITTLE_ENDIAN
#undef  _BYTE_ORDER_IS_BIG_ENDIAN
#define _BYTE_ORDER_IS_LITTLE_ENDIAN
#else /* _VXWORKS ] [ */
#undef  _BYTE_ORDER_IS_LITTLE_ENDIAN
#undef  _BYTE_ORDER_IS_BIG_ENDIAN
#define _BYTE_ORDER_IS_LITTLE_ENDIAN
#endif /* _VXWORKS ] */
/*/////////////////////////////////////////////////////////////////////// */

#ifdef _VXWORKS /* [ */
#include "vxWorks.h"
#else /* _VXWORKS ] [ */
typedef int STATUS;
typedef void * MSG_Q_ID;
typedef void * SEM_ID;
#endif /* _VXWORKS ] */

#include <os/OsIntTypes.h>

typedef int16_t MpAudioSample;    ///< Audio sample (16 bit, signed, little-endian)

typedef int MpConnectionID;

#endif /* _INCLUDED_MPTYPES_H ] */
