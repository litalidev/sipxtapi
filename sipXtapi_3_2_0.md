sipXtapi 3.2.0 contains number of incompatible API changes in comparison to version 3.1

# Improvements from 3.1.0 #
  * adaptive jitter buffer
  * PLC (packet loss concealment). It helps a lot when sometimes 1 RTP frame is lost continuously over time, leading to jitter buffer starvation. It just stores last best match RTP frame and uses it again if packet loss is detected. Packet loss is assumed when 2nd hit is to be used. By default, we will conceal up to 3 consecutive lost frames. Maximum concealable frames by this technique is ~2, but 3 helps to prevent jitter buffer starvation.
  * windows multimedia timer is used to drive tick timer of MpMediaTask
  * support for Intel IPP 5.3 - G.729, G.723.1 support
  * portaudio V19 sound driver instead of old internal driver
  * paralel DNS lookup
  * multi core support in sipXmediaLib audio frame processing (causing most load). Set with FRAME\_PROCESSING\_THREADS in MpMediaTask.h. If set to 0, MpMediaTask processes frames. If 1, single thread is created that processes frames, if 2, then 2 threads process audio frames. It can be set to any number. It should remove bottleneck in audio frame processing. This parameter should only be used in server applications. In client applications with few calls (1~3), it will most probably slow down processing.
  * pause/resume of buffer/file playback including events
  * cookie parameter for buffer/file playback events
  * sip proxy server per line, not only per sipxtapi instance
  * support for VS 2003 - solution and projects are available
  * sipXtapi is compilable in Linux, 3.1.0 wasn't
  * asynchronous event listeners. All listeners inherit from OsSharedServerTask and share single thread. This makes future simplification in sipXcallLib possible, without fear of getting deadlocks. One thread should be enough for all event listeners.
  * many bugs fixed since 3.1.0

# API changes #
  * SIPX\_MEDIA\_EVENT - MEDIA\_PLAYBACK\_PAUSED and MEDIA\_PLAYBACK\_RESUMED were added and everything after it is therefore shifted
  * SIPX\_MEDIA\_INFO structure changed, cookie and playbufferindex were been added
  * SIPX\_CALLSTATE\_INFO new fields sipResponseCode, szSipResponseText
  * sipxCallAudioPlaybackPause function added
  * sipxCallAudioPlaybackResume function added
  * sipxCallPlayBufferStart asynchronous. Read documentation carefully to avoid illegal memory access due to early deletion of buffer! This will be fixed in next release.
  * many changes in sipxAudio functions setting/getting volume
  * sipxInitialize and sipxReInitialize don't take certNickname, certPassword, dbLocation parameters anymore.
  * removed sipxConfigSetSecurityParameters function
  * added sipxConfigSetTLSSecurityParameters function which can be used to configure TLS/SSL certificates, private key and CAs. This function can be called only once, and has to be called as the first function before sipxInitialize. Subsequent executions have no effect. Read documentation of sipxConfigSetTLSSecurityParameters on how to setup TLS/SSL certificate.
  * removed sipxConfigSetSubscribeExpiration, it was always ignored anyway
  * added new parameter subscriptionPeriod with default value 3600 to sipxConfigSubscribe and sipxCallSubscribe
  * new function sipxLineSetOutboundProxy. Overrides default outbound proxy.

# Future #
  * conferencing without holding calls
  * refactored sipXcallLib, with new functionality
  * more unit tests