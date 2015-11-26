### To compile sipxtapi 3.3.0 with Intel IPP 6.0 ###

  1. Download w\_ipp\_ia32\_p\_6.0.1.070.exe, w\_ipp-samples\_p\_6.0.0.127.zip and install them into %IPPROOTDIR%, which is the path chosen in installation. Unzip speech samples, and call the directory "ipp-samples-6.0" instead of the default "ipp-samples".
  1. Add the following directories to PATH:
```
%IPPROOTDIR%\IPP\ipp-samples-6.0\tools\env
%IPPROOTDIR%\IPP\6.0.1.070\ia32\bin\
```
  1. Run "vcvarsall.bat x86" from visual studio (C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat)
  1. Go to IPP\ipp-samples-6.0\speech-codecs\, and type build32.bat. This will build libraries in
```
%IPPROOTDIR%\IPP\ipp-samples-6.0\speech-codecs\_bin\win32_cl9\lib
```
  1. Copy project file from [ipp\_6\_0\_project\_files.zip](http://sipxtapi.googlecode.com/files/ipp_6_0_project_files.zip) into %IPPROOTDIR%\IPP\ipp-samples-6.0\speech-codecs\application\usc\_speech\_codec\, and compile it. For visual studio 8, use the same file, but change Version="9,00" to Version="8,00" - it works well.
  1. Setup either global paths in Visual Studio (Tools->Options->Projects and solutions->VC++ directories), or add them to sipxmedialib (include) and sipxtapi (lib) project.
```
Include:
%IPPROOTDIR%\IPP\ipp-samples-6.0\speech-codecs\codec\speech_rtp\include
%IPPROOTDIR%\IPP\ipp-samples-6.0\speech-codecs\core\umc\include
%IPPROOTDIR%\IPP\ipp-samples-6.0\speech-codecs\core\usc\include
%IPPROOTDIR%\IPP\ipp-samples-6.0\speech-codecs\application\usc_speech_codec\include
%IPPROOTDIR%\IPP\ipp-samples-6.0\audio-video-codecs\core\vm\include
%IPPROOTDIR%\IPP\6.0.1.070\ia32\include

Lib:
%IPPROOTDIR%\IPP\ipp-samples-6.0\speech-codecs\_bin\win32_cl9\lib
%IPPROOTDIR%\IPP\6.0.1.070\ia32\lib
%IPPROOTDIR%\IPP\6.0.1.070\ia32\stublib
%IPPROOTDIR%\IPP\ipp-samples-6.0\speech-codecs\application\usc_speech_codec\Release
```
  1. Switch to Debug-IPP configuration for sipxmediadapterlib, sipxmedialib, sipxsdplib, sipxtapi.
  1. Compile sipXtapi library

To use internal encoder/decoder for G.711 a-law and u-law, gsmlib instead of Intel IPP, undefine PREFER\_INTEL\_IPP\_CODECS from sipXmediaLib project preprocessor definitions.

To disable wideband codecs, undefine ENABLE\_WIDEBAND\_AUDIO in sipXmediaLib, sipXmediaAdapterLib, sipXsdpLib.

To use Intel IPP 6.0 commercially or after trial expiration, you need to purchase a license from Intel. Some codecs like G.729 require additional licenses. Precompiled versions of sipXtapi do not ship with Intel IPP codecs.