### Project description ###

This is the homepage of customized sipXtapi library originally developed on SIP Foundry. The goal of this project is to produce a stable, commercially usable SIP SDK for client/server side SIP applications. API is designed to be simple but usable to enable rapid development. It is distributed under GNU LGPL license.

Sample applications are available to demonstrate capabilities of SDK. Customized sipXtapi is used by wxCommunicator and a number of other client and server applications. For details ask in mailinglist.

[More details](http://code.google.com/p/sipxtapi/wiki/Introduction)

### Features ###
Following features are supported in sipXtapi 3.2.0:
  * multiple sip lines, several calls per line.
  * sip proxy per line.
  * conferencing
  * blind, consultative call transfer
  * SUBSCRIBE, NOTIFY, basic MESSAGE support
  * supported codecs - G711a, G711u, GSM, iLBC 30ms, Speex, G729a, G723.1 if user has Intel IPP 5.3 license. 8000Hz audio only.
  * adaptive jitter buffer. Packet loss concealment (PLC).
  * support for sending RFC 2833 DTMF, reception of RFC 2833, decoding of in-band DTMF.
  * portaudio V19 for client side applications for audio recording/playback
  * NAT traversal, STUN, TURN, ICE support, periodic SIP OPTIONS, CRLF keepalive

### Documentation ###

Public API documentation is available from downloads section (sipXtapi). Older version is also available in source code in sipXcallLib\doc\sipXtapi\html\index.html.

### Stable releases ###

There are 2 stable releases available:
  1. _[sipXtapi 3.2.0](http://code.google.com/p/sipxtapi/wiki/sipXtapi_3_2_0)_ from October 2008
  1. _sipXtapi 3.1.0_ from August 2007

### Future plans ###

The next planned release is 3.3.0:
  1. _[sipXtapi 3.3.0](http://code.google.com/p/sipxtapi/wiki/sipXtapi_3_3_0)_