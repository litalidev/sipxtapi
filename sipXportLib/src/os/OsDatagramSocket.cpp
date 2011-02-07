//  
// Copyright (C) 2007 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


// SYSTEM INCLUDES
#include <assert.h>
#include <stdio.h>

#define OPTIONAL_CONST const
#define OPTIONAL_VXW_CHARSTAR_CAST
#define resolvGetHostByName(a, b, c) gethostbyname(a)
#if defined(_WIN32)
#   include <winsock2.h>
#   include <time.h>
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#elif defined(_VXWORKS)
#   undef resolvGetHostByName
#   include <inetLib.h>
#   include <netdb.h>
#   include <resolvLib.h>
#   include <sockLib.h>
#   undef OPTIONAL_CONST
#   define OPTIONAL_CONST
#   undef OPTIONAL_VXW_CHARSTAR_CAST
#   define OPTIONAL_VXW_CHARSTAR_CAST (char*)
#elif defined(__pingtel_on_posix__)
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <netdb.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#else
#error Unsupported target platform.
#endif

// APPLICATION INCLUDES
#include "os/OsDatagramSocket.h"
#include "os/OsSysLog.h"

// DEFINES
#define SOCKET_LEN_TYPE
#ifdef __pingtel_on_posix__
#undef SOCKET_LEN_TYPE
#define SOCKET_LEN_TYPE (socklen_t *)
#endif

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS

#define MIN_REPORT_SECONDS 10

// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
OsDatagramSocket::OsDatagramSocket(int remoteHostPortNum, const char* remoteHost,
                                   int localHostPortNum, const char* localHost)
: mNumTotalWriteErrors(0)
, mNumRecentWriteErrors(0)
, mSimulatedConnect(FALSE)     // Simulated connection is off until
                               // activated in doConnect.
{
    int error = ctorCommonCode();
    if (error != 0)
    {
        OsSysLog::add(FAC_KERNEL, PRI_DEBUG,
                      "OsDatagramSocket::OsDatagramSocket( %s:%d %s:%d) failed w/ errno %d)",
                      remoteHost, remoteHostPortNum, localHost ? localHost : "NULL", 
                      localHostPortNum, error);
        goto EXIT;
    }

    // Bind to the socket
    bind(localHostPortNum, localHost);
    if (!isOk())
    {
        goto EXIT;
    }

    // Kick off connection
    doConnect(remoteHostPortNum, remoteHost, mSimulatedConnect);

EXIT:
    return;
}

// Bare-bones Constructor
// Callers will need to call bind & doConnect on their own when this returns
// (This allows for children to set socket options before the bind call)
OsDatagramSocket::OsDatagramSocket()
: mNumTotalWriteErrors(0)
, mNumRecentWriteErrors(0)
{
    int error = ctorCommonCode();
    if (error != 0)
    {
        OsSysLog::add(FAC_KERNEL, PRI_DEBUG,
                "OsDatagramSocket::OsDatagramSocket() failed w/ errno %d)", error);
    }
}

// Common code for both constructors
// Returns 0 on success
int OsDatagramSocket::ctorCommonCode()
{
    socketDescriptor = OS_INVALID_SOCKET_DESCRIPTOR;    

    // Verify socket layer is initialized.
    if (!socketInit())
    {
        return -1;
    }

    // Obtain time for throttling logging of write errors
    time(&mLastWriteErrorTime);

    // Initialize state settings
    mToSockaddrValid = FALSE;
    mpToSockaddr = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
    assert(NULL != mpToSockaddr);
    memset(mpToSockaddr, 0, sizeof(struct sockaddr_in));

    // Create the socket
    socketDescriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketDescriptor == OS_INVALID_SOCKET_DESCRIPTOR)
    {
        int error = OsSocketGetERRNO();
        close();
        return error;
    }

    makeICMPPortUnreachableResistant();

    return 0;
}

// Destructor
OsDatagramSocket::~OsDatagramSocket()
{
    close();
    free(mpToSockaddr);
}

/* ============================ MANIPULATORS ============================== */

// Assignment operator
OsDatagramSocket&
OsDatagramSocket::operator=(const OsDatagramSocket& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   return *this;
}

int OsDatagramSocket::bind(int localHostPortNum, const char* localHost)
{
    int error = 0;

    localHostPort = localHostPortNum;    
    if (localHost)
    {
        localHostName = localHost ;
    }

    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(localHostPort == PORT_DEFAULT ? 0 : localHostPort);

#ifdef WIN32
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    mLocalIp = localHost;
#else
    // Should use host address (if specified in localHost) but for now use any
    if (!localHost)
    {
        localAddr.sin_addr.s_addr=OsSocket::getDefaultBindAddress(); // $$$
        mLocalIp = inet_ntoa(localAddr.sin_addr);
    }
    else
    {
        struct in_addr  ipAddr;
        ipAddr.s_addr = inet_addr (localHost);
        localAddr.sin_addr.s_addr= ipAddr.s_addr;
        mLocalIp = localHost;
    }
#endif
    
    error = ::bind( socketDescriptor, (struct sockaddr*) &localAddr,
                    sizeof(localAddr));

    if(error == OS_INVALID_SOCKET_DESCRIPTOR)
    {
        error = OsSocketGetERRNO();
        close();
        OsSysLog::add(FAC_KERNEL, PRI_ERR, "OsDatagramSocket::bind to %s:%d failed w/errno %d\n",
                 mLocalIp.data(), localHostPortNum, error);
    }
    else
    {
        sockaddr_in addr ;
        int addrSize = sizeof(struct sockaddr_in);
        error = getsockname(socketDescriptor, (struct sockaddr*) &addr, SOCKET_LEN_TYPE& addrSize);
        localHostPort = htons(addr.sin_port);
    }

    return error;
}

UtlBoolean OsDatagramSocket::reconnect()
{
    osPrintf("WARNING: reconnect NOT implemented!\n");
    return(FALSE);
}

void OsDatagramSocket::doConnect(int remoteHostPortNum, const char* remoteHost,
                                 UtlBoolean simulateConnect)
{
    struct hostent* server;

    mToSockaddrValid = FALSE;
    memset(mpToSockaddr, 0, sizeof(struct sockaddr_in));
    remoteHostPort = remoteHostPortNum;

    // Store host name
    if(remoteHost)
    {
        remoteHostName = remoteHost ;
        getHostIpByName(remoteHostName, &mRemoteIpAddress);
    }
    else
    {
        remoteHostName.remove(0) ;
    }

    // Connect to a remote host if given
    if(portIsValid(remoteHostPort) && remoteHost && !simulateConnect)
    {
#if defined(_WIN32) || defined(__pingtel_on_posix__)
        unsigned long ipAddr;

        ipAddr = inet_addr(remoteHost);

        if (ipAddr != INADDR_NONE) 
        {
           server = gethostbyaddr((char * )&ipAddr,sizeof(ipAddr),AF_INET);
        }

        if ( server == NULL ) 
        { 
           server = gethostbyname(remoteHost);
        }

#elif defined(_VXWORKS)
        char hostentBuf[512];
        server = resolvGetHostByName((char*) remoteHost,
                                      hostentBuf, sizeof(hostentBuf));
#else
#  error Unsupported target platform.
#endif //_VXWORKS

        if (server)
        {
            struct in_addr* serverAddr = (in_addr*) (server->h_addr);
            struct sockaddr_in serverSockAddr;
            serverSockAddr.sin_family = server->h_addrtype;
            serverSockAddr.sin_port = htons(remoteHostPort);
            serverSockAddr.sin_addr.s_addr = (serverAddr->s_addr);

            // Set the default destination address for the socket
#if defined(_WIN32) || defined(__pingtel_on_posix__)
            if(connect(socketDescriptor, (const struct sockaddr*) 
                    &serverSockAddr, sizeof(serverSockAddr)))
#elif defined(_VXWORKS)
            if(connect(socketDescriptor, (struct sockaddr*) &serverSockAddr,
                       sizeof(serverSockAddr)))
#else
#  error Unsupported target platform.
#endif



            {
                int error = OsSocketGetERRNO();
                close();
                OsSysLog::add(FAC_KERNEL, PRI_DEBUG,
                        "OsDatagramSocket::doConnect( %s:%d ) failed w/ errno %d)",
                        remoteHost, remoteHostPortNum, error);
            }
            else
            {
                mIsConnected = TRUE;
            }
        }
        else
        {
            close();
            OsSysLog::add(FAC_KERNEL, PRI_DEBUG,
                    "OsDatagramSocket::doConnect( %s:%d ) failed host lookup)",
                    remoteHost, remoteHostPortNum);           

            goto EXIT;
        }
    }
    else if(portIsValid(remoteHostPort) && remoteHost && simulateConnect)
    {
        mIsConnected = TRUE;
        mSimulatedConnect = TRUE;
    }
EXIT:
   ;
}

int OsDatagramSocket::write(const char* buffer, int bufferLength)
{
    int returnCode;

    if(mSimulatedConnect)
    {
        returnCode = writeTo(buffer, bufferLength);
    }
    else
    {
        returnCode = OsSocket::write(buffer, bufferLength);
    }

    return(returnCode);
}

int OsDatagramSocket::write(const char* buffer, int bufferLength,
      const char* ipAddress, int port)
{
    int bytesSent = 0;

    struct sockaddr_in toSockAddress;
    memset(&toSockAddress, 0, sizeof(toSockAddress));
    toSockAddress.sin_family = AF_INET;
    toSockAddress.sin_port = htons(port);

    if(ipAddress == NULL || !strcmp(ipAddress, "0.0.0.0") ||
        strlen(ipAddress) == 0 ||
        (toSockAddress.sin_addr.s_addr = inet_addr(ipAddress)) ==
            OS_INVALID_INET_ADDRESS)
    {
        osPrintf("OsDatagramSocket::write invalid IP address: \"%s\"\n",
            ipAddress);
    }
    else
    {
        // Why isn't this abstracted into OsSocket, as is done in ::write(2)?
        bytesSent = sendto(socketDescriptor,
#ifdef _VXWORKS
            (char*)
#endif
            buffer, bufferLength,
            0,
            (struct sockaddr*) &toSockAddress, sizeof(struct sockaddr_in));

        if(bytesSent != bufferLength)
        {
           OsSysLog::add(FAC_KERNEL, PRI_ERR,
                         "OsDatagramSocket::write(4) bytesSent = %d, "
                         "bufferLength = %d, errno = %d",
                         bytesSent, bufferLength, errno);
            time_t rightNow;

            (void) time(&rightNow);

            mNumRecentWriteErrors++;

            if (MIN_REPORT_SECONDS <= (rightNow - mLastWriteErrorTime)) {

                mNumTotalWriteErrors += mNumRecentWriteErrors;
                if (0 == mNumTotalWriteErrors) {
                    mLastWriteErrorTime = rightNow;
                }
                osPrintf("OsDataGramSocket::write:\n"
                    "     In last %ld seconds: %d errors; total %d errors;"
                    " last errno=%d\n",
                    (rightNow - mLastWriteErrorTime), mNumRecentWriteErrors,
                    mNumTotalWriteErrors, OsSocketGetERRNO());

                mLastWriteErrorTime = rightNow;
                mNumRecentWriteErrors = 0;
            }
        }
    }
    return(bytesSent);
}

UtlBoolean OsDatagramSocket::getToSockaddr()
{
    const char* ipAddress = mRemoteIpAddress.data();

    if (!mToSockaddrValid) 
    {
        mpToSockaddr->sin_family = AF_INET;
        mpToSockaddr->sin_port = htons(remoteHostPort);

        if (ipAddress == NULL || !strcmp(ipAddress, "0.0.0.0") ||
            strlen(ipAddress) == 0 ||
            (mpToSockaddr->sin_addr.s_addr = inet_addr(ipAddress)) ==
                OS_INVALID_INET_ADDRESS)
        {
/*
            osPrintf(
               "OsDatagramSocket::getToSockaddr: invalid IP address: \"%s\"\n",
                ipAddress);
*/
        } else {
            mToSockaddrValid = TRUE;
        }
    }
    return mToSockaddrValid;
}

int OsDatagramSocket::writeTo(const char* buffer, int bufferLength)
{
    int bytesSent = 0;

    if (getToSockaddr()) {
        bytesSent = sendto(socketDescriptor,
#ifdef _VXWORKS
            (char*)
#endif
            buffer, bufferLength,
            0,
            (struct sockaddr*) mpToSockaddr, sizeof(struct sockaddr_in));

        if(bytesSent != bufferLength)
        {
            time_t rightNow;

            (void) time(&rightNow);

            mNumRecentWriteErrors++;

            if (MIN_REPORT_SECONDS <= (rightNow - mLastWriteErrorTime)) {

                mNumTotalWriteErrors += mNumRecentWriteErrors;
                if (0 == mNumTotalWriteErrors) {
                    mLastWriteErrorTime = rightNow;
                }
                osPrintf("OsDataGramSocket::write:\n"
                    "     In last %ld seconds: %d errors; total %d errors;"
                    " last errno=%d\n",
                    (rightNow - mLastWriteErrorTime), mNumRecentWriteErrors,
                    mNumTotalWriteErrors, OsSocketGetERRNO());

                mLastWriteErrorTime = rightNow;
                mNumRecentWriteErrors = 0;
            }
        }
    }
    return(bytesSent);
}

#ifdef WANT_GET_FROM_INFO /* [ */
#ifdef _VXWORKS /* [ */
static int getFromInfo = 0;
extern "C" { extern int setFromInfo(int x);};
int setFromInfo(int getIt) {
   int save = getFromInfo;
   getFromInfo = getIt ? 1 : 0;
   return save;
}
#define GETFROMINFO getFromInfo
#endif /* _VXWORKS ] */
#endif /* WANT_GET_FROM_INFO ] */

int OsDatagramSocket::read(char* buffer, int bufferLength)
{
    int bytesRead;

    // If the remote end is not "connected" we cannot use recv
    if(mSimulatedConnect || !portIsValid(remoteHostPort) || remoteHostName.isNull())
    {
#ifdef GETFROMINFO /* [ */
        if (GETFROMINFO) {
          int fromPort;
          UtlString fromAddress;
          bytesRead = OsSocket::read(buffer, bufferLength,
             &fromAddress, &fromPort);
          fromAddress.remove(0);
        } else
#endif /* GETFROMINFO ] */
        {
           bytesRead = OsSocket::read(buffer, bufferLength,
              (struct in_addr*) NULL, NULL);
        }
    }
    else
    {
        bytesRead = OsSocket::read(buffer, bufferLength);
    }
    return(bytesRead);
}

/* ============================ ACCESSORS ================================= */
OsSocket::IpProtocolSocketType OsDatagramSocket::getIpProtocol() const
{
    return(UDP);
}

void OsDatagramSocket::getRemoteHostIp(struct in_addr* remoteHostAddress,
                               int* remotePort)
{
#ifdef __pingtel_on_posix__
    socklen_t len;
#else
    int len;
#endif
    struct sockaddr_in remoteAddr;
    const struct sockaddr_in* pAddr;

    if (mSimulatedConnect) {
        getToSockaddr();
        pAddr = mpToSockaddr;
    } else {
        pAddr = &remoteAddr;

        len = sizeof(struct sockaddr_in);

        if (getpeername(socketDescriptor, (struct sockaddr *)pAddr, &len) != 0)
        {
            memset(&remoteAddr, 0, sizeof(struct sockaddr_in));
        }
    }
    memcpy(remoteHostAddress, &(pAddr->sin_addr), sizeof(struct in_addr));

#ifdef TEST_PRINT
    {
        int p = ntohs(pAddr->sin_port);
        UtlString o;
        inet_ntoa_pt(*remoteHostAddress, o);
        osPrintf("getRemoteHostIP: Remote name: %s:%d\n"
           "   (pAddr->sin_addr) = 0x%X, sizeof(struct in_addr) = %d\n",
           o.data(), p, (pAddr->sin_addr), sizeof(struct in_addr));
    }
#endif

    if (NULL != remotePort)
    {
        *remotePort = ntohs(pAddr->sin_port);
    }
}

// Return the external IP address for this socket.
UtlBoolean OsDatagramSocket::getMappedIp(UtlString* ip, int* port) 
{
    return FALSE ;
}
    
void OsDatagramSocket::makeICMPPortUnreachableResistant()
{
#if defined(_WIN32)
    int newValue = FALSE;
    unsigned long bytesReturned = 0;

    int error = WSAIoctl(socketDescriptor, SIO_UDP_CONNRESET, &newValue, sizeof(newValue), NULL, 0, &bytesReturned, NULL, NULL);
    if(error == SOCKET_ERROR)
    {
        osPrintf("WSAIoctl call failed with error: %d in OsSocket::OsSocket\n", error);
    }
#endif
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
