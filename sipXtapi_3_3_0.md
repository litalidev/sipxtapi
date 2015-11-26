sipXtapi 3.3.0 is currently in development in trunk.

To compile sipXtapi with Intel IPP 6.0 support, follow steps from [Intel IPP 6.0 support](http://code.google.com/p/sipxtapi/wiki/intel_ipp_6_0_support).

### Code changes from 3.2.0 ###
#### sipXtapi ####
  * new API for getting all supported codecs. This was not available in 3.2.0 and had to be worked around (sipxConfigGetNumAvailableAudioCodecs, sipxConfigGetAvailableAudioCodec, sipxConfigGetNumSelectedAudioCodecs, sipxConfigGetSelectedAudioCodec)
  * public and private conferences. Private conferences work like in 3.2.0. Public conferences are assigned a conference SIP uri. Dialing this SIP uri results in call automatically being created in a conference.
  * sipxConferenceJoin, sipxConferenceSplit don't require call hold
  * conference events - see SIPX\_CONFERENCE\_EVENT, SIPX\_CONFERENCE\_CAUSE in sipXtapiEvents.h
  * call/media events will not be 100% compatible with 3.2.0. Any changes will be described to make upgrade to 3.3.0 easy.
  * changes in call transfer events - transferee side (recepient of REFER request) gets DIALTONE event instead of NEWCALL, when creating new call, since that call is outbound.
  * possibility to accept/reject call transfer request (sipxCallAcceptTransfer, sipxCallRejectTransfer), no support for out of dialog REFER
  * safer sipxCallPlayBufferStart, buffer can be deleted immediately after function returns
  * selection of codecs by bandwidth in sipXtapi will not be supported. Only selection by name will be supported. User may display codecs along with bandwidth requirements, and then choose codecs by name.
  * separate functions sipxCallLimitCodecPreferences, sipxCallRenegotiateCodecPreferences
  * sip proxy per line, using sipxLineSetOutboundProxy
  * better audio focus control when connecting call with SIPX\_FOCUS\_CONFIG
  * contactId now works the way it should - when its SIPX\_AUTOMATIC\_CONTACT\_ID then SipUserAgent will choose the best contact, otherwise contact selection will be respected (user knows better which contact is the best)
  * transport selection is functional - SIPX\_CALL\_OPTIONS has transportType which in cooperation with contactId selects transport. It is now possible to initiate a call using TCP transport which was not possible in 3.2.0. Also sipxConfigSubscribe, sipxCallSubscribe, sipxLineAdd, sipxPIMSendPagerMessage now have transport parameter.
#### sipXcallLib ####
  * conferencing without having to invoke hold/unhold on conference or call.
  * public and private conferences. Public conferences will be bound to line, and any accepted inbound calls will be added to conference.
  * big refactoring in sipXcallLib, CpCall/CpPeerCall, Connection/SipConnection, CallManager/CpCallManager will be gone.
  * support for session timer (rfc4028), 100rel (rfc3262), norefersub (rfc4488), replaces (rfc3891), from-change (rfc4916), join (rfc3911)
  * UPDATE, PRACK method support
  * early and late SDP negotiation (SDP in INVITE, or ACK), configurable
  * sending SDP in unreliable and reliable 18x responses - possibility to send early audio
  * configurable sending of reliable 18x responses, session timer and usage of UPDATE method for hold/unhold/codec renegotiation
  * refactored handling of inbound INFO
  * special message classes for messages instead of usage of OsIntPtrMsg or CpMultiStringMessage
  * less circular dependencies, various OO design defects fixed
  * old sipXcallLib is completely rewritten, since old code prevented development of new features
#### sipXsdpLib ####
  * minor refactoring of sipXtackLib and sipXsdpLib - SDP handling (SdpCodecList, SdpCodecFactory), usage of SdpCodecList in sipXmediaAdapterLib whenever possible, avoid passing of arrays
  * much easier to add new codecs. Codecs are still static, but the complexity of adding one codecs has been reduced.
#### sipXtackLib ####
  * removed support for PING method, it never became a standard (http://tools.ietf.org/html/draft-fwmiller-ping-03)
  * SipMessage class supports parsing and setting new fields
  * much smarter selection of sip contacts, it's no more needed to select them manually. This makes usage of sipXtapi in multi ip/multi sip proxy environment much easier
#### sipXmediaLib ####
  * wide band audio support with 48Khz internal sampling rate. Old 8Khz narrow band is recommended for server applications due to lower CPU load.
  * VAD (voice activity detection), DTX (discontinuous transmission) for all codecs, CNG (comfort noise generation)
  * support for additional speex modes (11,000 bps and 18,200 bps), almost all speex wide band (16Khz), speex ultra band modes (32Khz)
  * iLBC 20ms and 30ms modes are supported for sending and receiving - 8Khz
  * G.711 u-law and a-law via built-in or Intel IPP 6.0 encoder/decoder
  * G.722 codec 64 Kbps via Span DSP library - 16Khz
  * G.722.1 codec (16, 24, 32 Kbps modes) via Intel IPP - 16Khz
  * G.723.1 codec (5.3, 6.3 Kbps, sending 5.3 Kbps) via Intel IPP - 8Khz
  * G.726 codec (16, 24, 32, 40 Kbps modes) via Span DSP library - 8Khz
  * G.728 sending 16 Kbps, receiving 12.8 Kbps and 16 Kbps via Intel IPP - 8Khz
  * G.729 annex A, B, D and E (with VAD or without) via Intel IPP - 8Khz
  * G.729.1 codec (8, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32 KBps, sending and receving any rate) via Intel IPP - 16Khz
  * L16 audio - uncompressed 16bit mono audio in network byte order. All recommended sampling rates up to 48Khz.
  * GSM FR codec (13 Kbps) - via gsmlib or Intel IPP 6.0 encoder/decoder - 8Khz
  * AMR codec (4.75, 5.15, 5.9, 6.7, 7.4, 7.95, 10.2, 12.2 Kbps - sending 4.75/10.2 Kbps, receiving any rate), octet aligned and bandwidth efficient mode, via Intel IPP - 8Khz
  * AMR wideband codec (6.6, 8.85, 12.65, 14.25, 15.85, 18.25, 19.85, 23.05 or 23.85 Kbps - sending 12.65/23.85 Kbps, receiving any rate), octet aligned and bandwidth efficient mode, via Intel IPP - 16Khz
  * improved old inbound DTMF detector, and new Span DSP DTMF detector (enabled by default in Windows). Span DSP DTMF detector only available when sipXtapi is compiled with only narrow band suport. DTMF detection is mostly effective at 8Khz.
  * upgrade to Intel IPP 6.0 (compatibility with Intel IPP 5.3 maintained)
  * upgrade to speex 1.2rc1 library
  * usage of synchronous Portaudio API instead of asynchronous. Reduction of latency by ~160ms by this change.
  * possibility to adjust audio driver latency
  * linear complexity bridge (less CPU cost)
  * introduction of Span DSP 0.0.6pre3
  * media events MEDIA\_REMOTE\_SILENT, MEDIA\_REMOTE\_ACTIVE work

#### sipXportLib ####
  * minor improvements in sipXportLib - OsSharedServerTask, OsRWMutex inherits from OsRWSyncBase and is write recursive. OsQueuedNotification superseeding OsQueuedEvent.
  * usage of OsTimerNotification for sending timer event messages. Custom notification subclass for each timer, avoiding any memory leaks.

### Will not be addressed ###
  * TLS code, custom transport, SRTP, RTP over TCP, S/MIME

### Standards ###
#### Supported requests for comments ####
SipXtapi 3.3.0 will support the following RFCs:
  * RFC 2782 - A DNS RR for specifying the location of services (DNS SRV) - http://tools.ietf.org/html/rfc2782
  * RFC 2833 - RTP Payload for DTMF Digits - http://tools.ietf.org/html/rfc2833
  * RFC 2976 - The SIP INFO Method - http://tools.ietf.org/html/rfc2976
  * RFC 3261 - SIP: Session Initiation Protocol - http://tools.ietf.org/html/rfc3261
  * RFC 3262 - Reliability of Provisional Responses in SIP - http://tools.ietf.org/html/rfc3262
  * RFC 3263 - Session Initiation Protocol (SIP): Locating SIP Servers - http://tools.ietf.org/html/rfc3263
  * RFC 3264 - An Offer/Answer Model with the Session Description Protocol (SDP) - http://tools.ietf.org/html/rfc3264
  * RFC 3265 - Session Initiation Protocol (SIP)- Specific Event Notification - http://tools.ietf.org/html/rfc3265
  * RFC 3311 - The Session Initiation Protocol (SIP) UPDATE Method - http://tools.ietf.org/html/rfc3311
  * RFC 3326 - The Reason Header Field for the Session Initiation Protocol (SIP) - http://tools.ietf.org/html/rfc3326
  * RFC 3420 - Internet Media Type message/sipfrag - http://tools.ietf.org/html/rfc3420
  * RFC 3428 - Session Initiation Protocol (SIP) Extension for Instant Messaging - http://tools.ietf.org/html/rfc3428
  * RFC 3489 - Simple Traversal of UDP Through Network Address Translators (NATs) - http://tools.ietf.org/html/rfc3489
  * RFC 3515 - The Session Initiation Protocol (SIP) Refer Method - http://tools.ietf.org/html/rfc3515
  * RFC 3550 - RTP: A Transport Protocol for Real-Time Applications - http://tools.ietf.org/html/rfc3550
  * RFC 3551 - RTP Profile for Audio and Video Conferences with Minimal Control - http://tools.ietf.org/html/rfc3551
  * RFC 3581 - An extension to SIP for Symmetric Response Routing - http://tools.ietf.org/html/rfc3581
  * RFC 3891 - The Session Initiation Protocol (SIP) "Replaces" Header - http://tools.ietf.org/html/rfc3891
  * RFC 3903 - Session Initiation Protocol (SIP) Extension for Event State Publication - http://tools.ietf.org/html/rfc3903
  * RFC 3951 - Internet Low Bit Rate Codec (iLBC) - http://tools.ietf.org/html/rfc3951
  * RFC 3952 - Real-time Transport Protocol (RTP) Payload Format for internet Low Bit Rate Codec (iLBC) Speech - http://tools.ietf.org/html/rfc3952
  * RFC 4028 - Session Timers in the Session Initiation Protocol (SIP) - http://tools.ietf.org/html/rfc4028
  * RFC 4488 - Suppression of Session Initiation Protocol (SIP) REFER Method Implicit Subscription - http://tools.ietf.org/html/rfc4488
  * RFC 4566 - SDP: Session Description Protocol - http://tools.ietf.org/html/rfc4566
  * RFC 4916 - Connected Identity in the Session Initiation Protocol (SIP) - http://tools.ietf.org/html/rfc4916
  * RFC 5057 - Multiple Dialog Usages in the Session Initiation Protocol - http://tools.ietf.org/html/rfc5057

#### Supported internet drafts ####
SipXtapi 3.3.0 will support the same drafts for ICE and TURN as sipXtapi 3.2.0:
  * draft-ietf-mmusic-ice-04 - Interactive Connectivity Establishment (ICE)
  * draft-rosenberg-midcom-turn-04 - Traversal Using Relay NAT (TURN)

#### Candidates for future enhancements ####
We may consider supporting the following RFCs and drafts in the future:
  * RFC 3680 - A Session Initiation Protocol (SIP) Event Package for Registrations - http://tools.ietf.org/html/rfc3680
  * RFC 3842 - A Message Summary and Message Waiting Indication Event Package for the Session Initiation Protocol (SIP) - http://tools.ietf.org/html/rfc3842
  * RFC 3856 - A Presence Event Package for the Session Initiation Protocol (SIP) - http://tools.ietf.org/html/rfc3856
  * RFC 3863 - Presence Information Data Format (PIDF) - http://tools.ietf.org/html/rfc3863
  * RFC 3911 - The Session Initiation Protocol (SIP) "Join" Header http://tools.ietf.org/html/rfc3911
  * RFC 3994 - Indication of Message Composition for Instant Messaging - http://tools.ietf.org/html/rfc3994
  * RFC 4235 - An INVITE-Initiated Dialog Event Package for the Session Initiation Protocol (SIP) - http://tools.ietf.org/html/rfc4235
  * RFC 4575 - A Session Initiation Protocol (SIP) Event Package for Conference State - http://tools.ietf.org/html/rfc4575
  * draft-kaplan-sipping-dtmf-package-00 - DTMF Info-Event Package - http://tools.ietf.org/html/draft-kaplan-sipping-dtmf-package-00
  * draft-ietf-sipcore-info-events-05 - Session Initiation Protocol (SIP) INFO Method and Package Framework - http://tools.ietf.org/html/draft-ietf-sipcore-info-events-05
  * draft-ietf-mmusic-ice-19 - Interactive Connectivity Establishment (ICE) - http://tools.ietf.org/html/draft-ietf-mmusic-ice-19
  * draft-ietf-behave-turn-16 - Traversal Using Relays around NAT (TURN) - http://tools.ietf.org/html/draft-ietf-behave-turn-16

Draft draft-kaplan-sipping-dtmf-package-00 is obsolete, but part of it dealing with transport of DTMF digits in INFO messages might be supported by many commercial applications. Capability negotiation in INVITE would not be implemented.