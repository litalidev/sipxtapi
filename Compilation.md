# Windows #

Visual Studio 2003 or later is required. Solution and projects for VS2005 are provided, which can be upgraded to VS2008 projects by user upon opening. VS2003 projects are available for 3.2.0 release and trunk. VC6 is not supported anymore. VS2008 projects are available in experimental callllib-rewrite branch, and will be present in sipXtapi 3.3.0. In case problems are encountered in VS2005, try updating to SP1 or SP2.

### Libraries included in sipxtapi: ###
  * nspr-4.6.4 - will be removed in the future
  * nss-3.9.2 - will be removed in the future
  * pcre 7.0 (2006)
  * libGSM 06.10
  * libILBC
  * patched portaudio V19
  * patched portmixer from Audacity project
  * speex 1.2beta2
  * wxWidgets 2.6 - used by sipXezPhone (no upgrade planned, sipXezPhone is obsolete with wxCommunicator)

### Libraries not included in sipxtapi: ###
  * DirectX SDK - required by portaudio. If server version of sipxtapi is compiled, not needed. Download from http://msdn.microsoft.com/en-us/directx/default.aspx (version from November 2008 works, newer is not tested)
  * OpenSSL - needed for TLS support. Optional dependency. Download stable snapshot openssl-0.9.8-stable-SNAP from [ftp://ftp.openssl.org/snapshot/](ftp://ftp.openssl.org/snapshot/) and compile. OpenSSL needs to be located in openssl directory outside sipxtapi.
  * cppunit 1.12 or later - needed to run unittests. Optional dependency. Download from http://cppunit.sourceforge.net/

## Compilation steps ##
  1. Download DirectX SDK and setup VS2005 to be able to locate DirectX include and lib\x86 directories. This can be done in Tools->Options->Projects and Solutions->VC++ Directories.
  1. Optionally download and compile cppunit 1.12 and OpenSSL. Setup VS2005 to be able to find cppunit.
  1. Check out desired branch from SVN. See http://code.google.com/p/sipxtapi/wiki/AvailableBranches for description of available branches.
  1. Open sipX-msvc8.sln. Select Win32 solution platform, and Debug or Release solution configuration. Build sipXtapi-msvc8 project.
  1. Compile sample applications ReceiveCall, PlaceCall, sipXezPhone, SimpleSipxTerm

# Linux #

Guide how to build sipXtapi in Linux can be found in readme.txt. This is the shortened version extracted from readme.txt.

### Basic requirements: ###
  * gcc, g++, autoconf, automake, libtool, libssl-dev

In Debian run:
```
apt-get install libtool autoconf automake gcc g++ libssl-dev
```

The Linux build has been tested on Debian 4.0r2. Autoconf is required to generate the configure script. Pcre library is mandatory, while cppunit is only required for unittests.

In Debian run:
```
apt-get install libpcre3-dev libcppunit-dev
```

Speex library is currently not supported in Linux. Configure script will ignore it.

### Optional dependencies: ###
  * gsm-devel, doxygen, graphviz

In Debian run:
```
apt-get install libgsm1-dev doxygen graphviz
```

Possible problems:
If you get the following error during autoreconf then install libtool:

> configure.ac:14: error: possibly undefined macro: AC\_PROG\_LIBTOOL
> If this token and others are legitimate, please use m4\_pattern\_allow.
> See the Autoconf documentation.

## Build ##

run ./pre\_conf.sh to create symlinks for includes
```
cd sipXportLib
autoreconf -fi
./configure --prefix=/tmp/stage
make;make install
cd ..
```
```
cd sipXsdpLib
autoreconf -fi
./configure --prefix=/tmp/stage
make;make install
cd ..
```
```
cd sipXtackLib
autoreconf -fi
./configure --prefix=/tmp/stage --disable-sipviewer [--disable-codec-gsm]
make;make install
cd ..
```
For sipXmediaLib and sipXmediaAdapterLib, local (speaker, mic) audio must be disabled.
```
cd sipXmediaLib
autoreconf -fi
./configure --prefix=/tmp/stage --disable-local-audio [--disable-codec-gsm]
make;make install
cd ..
```
```
cd sipXmediaAdapterLib
autoreconf -fi
./configure --prefix=/tmp/stage --disable-local-audio [--disable-codec-gsm]
make;make install
cd ..
```
```
cd sipXcallLib
autoreconf -fi
./configure --prefix=/tmp/stage
make;make install
cd ..
```
### Test using PlaceCall ###
```
cd examples/PlaceCall
make
./PlaceCall IP
```
PlaceCall and ReceiveCall are known to build with this source tree.
sipXezPhone **should** work, but hasn't been tested with the latest source.

For sipXezPhone, wxWidgets 2.6.x is required. wxWidgets 2.8.x is not supported by sipXezPhone.