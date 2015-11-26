# Available branches #

Development of sipxtapi is split into trunk and several side branches. There are 2 types of branches - experimental for implementing new features and stable branches which only receive bug fixes. Trunk is not used for developing features requiring big changes in code. Effort is made to keep trunk stable as well.

### trunk ###

Trunk is used for developing features requiring only small changes in code, fixing bugs that are later merged to stable branches. It should be quite stable most of the time, although from time to time it might happen that certain targets become uncompilable for short time. It is stable enough to use for development.

### sipxtapi-3.1.0 ###

Contains stable branch 3.1.0. Receives only bugfixes. No new features are implemented. Uses original sipxtapi audio driver, and uses slightly older (and different) API than trunk.

### sipxtapi-3.2.0 ###

Contains stable branch 3.2.0. Receives only bugfixes. No new features are implemented. Unlike sipxtapi-3.1.0, it is compilable in Linux.

### ipp-5.3-upgrade ###

This branch was used to upgrade Intel IPP G.729a codec implementation from version 5.1 to version 5.3. Not used anymore.

### audio-refactoring ###

This branch was used for replacement of old sipxmedialib audio driver with portaudio and portmixer. Not used anymore.

### gips-support ###

This branch will be used for adding GIPS support to sipxtapi again.

### tacklib-refactoring ###

Was used for minor refactoring in sipXtackLib - especially in line area, SipDialog, some minor changes in SipMessage and Url class to make it possible to use sipXtackLib in refactored sipXcallLib correctly without hacks.
Contains also beginning of complete sipXcallLib rewrite.

### calllib-refactoring ###

This branch was meant to be used for big changes in sipxcalllib that never happened. Only unused code in sipxcalllib was removed, to make bugfixing easier. Not used anymore.

### calllib-rewrite ###
Was used for complete replacement of CpCall, CpPeerCall, CallManager, CpCallManager, SipConnection, CpGhostConnection, Connection classes. Changes were merged into trunk.

See [sipXtapi 3.3.0](http://code.google.com/p/sipxtapi/wiki/sipXtapi_3_3_0)