//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifdef SIP_TLS

// SYSTEM INCLUDES
#include <assert.h>

// APPLICATION INCLUDES


#include <os/OsDateTime.h>
#include <os/HostAdapterAddress.h>
#include <utl/UtlHashMapIterator.h>

#ifdef SIP_TLS
#  include <tapi/sipxtapi.h>
#  include <os/OsSSLServerSocket.h>
#  include <os/OsSSLConnectionSocket.h>
#endif

#include <net/SipServerBroker.h>
#include <net/SipTcpServer.h>
#include <net/SipUserAgent.h>
#include <net/SipTlsServer.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
//#define TEST_PRINT
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
SipTlsServer::SipTlsServer(int port,
                           SipUserAgent* userAgent, 
                           UtlBoolean bUseNextAvailablePort,
                           UtlString certNickname,
                           UtlString certPassword,
                           UtlString dbLocation,
                           const char*  szBindAddr) :
 SipProtocolServerBase(userAgent, "TLS", "SipTlsServer %d"),
 mCertNickname(certNickname),
 mCertPassword(certPassword),
 mDbLocation(dbLocation),
 mTlsInitCode(OS_SUCCESS)
{
    OsSysLog::add(FAC_SIP, PRI_DEBUG,
                  "SipTlsServer::_ port = %d, bUseNextAvailablePort = %d",
                  port, bUseNextAvailablePort);

    mServerPort = port ;
    mpServerBrokerListener = new SipServerBrokerListener(this);

#ifdef _DISABLE_MULTIPLE_INTERFACE_SUPPORT
   szBindAddr = "0.0.0.0" ;
#endif

    OsSysLog::add(FAC_SIP,PRI_DEBUG,"SipTlsServer::~ port %d", port);

    if (szBindAddr && 0 != strcmp(szBindAddr, "0.0.0.0"))
    {
        mDefaultIp = szBindAddr;
        createServerSocket(szBindAddr, mServerPort, bUseNextAvailablePort);
    }
    else      
    {
        int numAddresses = MAX_IP_ADDRESSES;
        const HostAdapterAddress* adapterAddresses[MAX_IP_ADDRESSES];
        getAllLocalHostIps(adapterAddresses, numAddresses);

        for (int i = 0; i < numAddresses; i++)
        {
            createServerSocket(adapterAddresses[i]->mAddress.data(),
                               mServerPort,
                               bUseNextAvailablePort);
            if (0 == i)
            {
                // use the first IP address in the array
                // for the 'default ip'
                mDefaultIp = adapterAddresses[i]->mAddress.data();
            }
            delete adapterAddresses[i];
        }
    }

   mDefaultPort = SIP_TLS_PORT;
}

UtlBoolean SipTlsServer::startListener()
{
    UtlBoolean bRet(FALSE);
#       ifdef TEST_PRINT
        osPrintf("SIP Server binding to port %d\n", serverPort);
#       endif

    // iterate over the SipServerBroker map and call start
    UtlHashMapIterator iterator(mServerBrokers);
    UtlVoidPtr* pBrokerContainer = NULL;
    SipServerBroker* pBroker = NULL;
    UtlString* pKey = NULL;
    
    while(pKey = (UtlString*)iterator())
    {
        pBrokerContainer = (UtlVoidPtr*) iterator.value();
        if (pBrokerContainer)
        {
            pBroker = (SipServerBroker*)pBrokerContainer->getValue();
            if (pBroker)
            {
                pBroker->start();
                bRet = TRUE;
            }
        }
    }
    return bRet;
}

OsStatus SipTlsServer::createServerSocket(const char* szBindAddr,
                                int& port,
                                const UtlBoolean& bUseNextAvailablePort)
{
    OsStatus rc = OS_FAILED;                                

    if(portIsValid(port))
    {
#ifdef SIP_TLS
        OsServerSocket* pServerSocket = new OsSSLServerSocket(64, port);
#else
        OsServerSocket* pServerSocket = new OsServerSocket(64, port);
#endif

        // If the socket is busy or unbindable and the user requested using the
        // next available port, try the next SIP_MAX_PORT_RANGE ports.
        if (bUseNextAvailablePort && !pServerSocket->isOk())
        {
            for (int i=1; i<=SIP_MAX_PORT_RANGE; i++)
            {
                delete pServerSocket ;
#ifdef SIP_TLS
                pServerSocket = new OsSSLServerSocket(64, port+i);
#else
                pServerSocket = new OsServerSocket(64, port+i);
#endif                
                if (pServerSocket->isOk())
                {
                    break ;
                }
            }
        }
        
        if (pServerSocket && pServerSocket->isOk())
        {
            mServerPort = pServerSocket->getLocalHostPort() ;
            
            port = pServerSocket->getLocalHostPort();
            SIPX_CONTACT_ADDRESS contact;
            strcpy(contact.cIpAddress, szBindAddr);
            contact.iPort = port;
            contact.eContactType = CONTACT_LOCAL;
            UtlString szAdapterName;
            memset((void*)szAdapterName.data(), 0, sizeof(szAdapterName)); // null out the string
            

            getContactAdapterName(szAdapterName, contact.cIpAddress);
            SAFE_STRNCPY(contact.cInterface, szAdapterName.data(), sizeof(contact.cInterface));
            contact.eTransportType = TRANSPORT_TLS;
            mSipUserAgent->addContactAddress(contact);
       
            // add address and port to the maps
            mServerSocketMap.insertKeyAndValue(new UtlString(szBindAddr),
                                               new UtlVoidPtr((void*)pServerSocket));
            mServerPortMap.insertKeyAndValue(new UtlString(szBindAddr),
                                                   new UtlInt(pServerSocket->getLocalHostPort()));
            mServerBrokers.insertKeyAndValue(new UtlString(szBindAddr),
                                              new UtlVoidPtr(new SipServerBroker((OsServerTask*)mpServerBrokerListener,
                                                                                                pServerSocket)));                                                   
    
            rc = OS_SUCCESS;
        }
    }
    return rc;
}    

void SipTlsServer::shutdownListener()
{
    requestShutdown();
    shutdownClients();
}

int SipTlsServer::run(void* runArgument)
{

    while (!isShuttingDown())
    {
        OsTask::delay(500); // this method really shouldn't do anything
    }

    return(0);
}
// Destructor
SipTlsServer::~SipTlsServer()
{
    waitUntilShutDown();
    if (mpServerBrokerListener)
    {
        mpServerBrokerListener->requestShutdown();
        delete mpServerBrokerListener;
    }
    {
        SipServerBroker* pBroker = NULL;
        UtlHashMapIterator iterator(this->mServerBrokers);
        UtlVoidPtr* pBrokerContainer = NULL;
        UtlString* pKey = NULL;
        
        while (pKey = (UtlString*)iterator())
        {
            pBrokerContainer = (UtlVoidPtr*)iterator.value();
            if (pBrokerContainer)
            {
                pBroker = (SipServerBroker*)pBrokerContainer->getValue();
                if (pBroker)
                {
                    pBroker->requestShutdown();
                    delete pBroker;
                }
            }
        }
        mServerBrokers.destroyAll();
    }

/*
    {
        OsSocket* pSocket = NULL;
        UtlHashMapIterator iterator(mServerSocketMap);
        UtlVoidPtr* pSocketContainer = NULL;
        UtlString* pKey = NULL;
        
        while (pKey = (UtlString*)iterator())
        {
            pSocketContainer = (UtlVoidPtr*)iterator.value();
            if (pSocketContainer)
            {
                pSocket = (OsSocket*)pSocketContainer->getValue();
                if (pSocket)
                {
                    delete pSocket;
                }
            }
        }
        mServerSocketMap.destroyAll();
    }
*/
    mServerSocketMap.destroyAll();
    mServerPortMap.destroyAll();


}

/* ============================ MANIPULATORS ============================== */

OsSocket* SipTlsServer::buildClientSocket(int hostPort, const char* hostAddress, const char* localIp)
{
    OsSocket* socket = NULL;
#ifdef SIP_TLS
    socket = new OsSSLConnectionSocket(hostPort, hostAddress);
#else
    // Create the socket in non-blocking mode so it does not block
    // while conecting
    socket = new OsConnectionSocket(hostPort, hostAddress, FALSE, localIp);
#endif
   if (socket)
   {
      socket->makeBlocking();
   }
   return(socket);
}


/* ============================ ACCESSORS ================================= */

// The the local server port for this server
int SipTlsServer::getServerPort() const 
{
    return mServerPort ;
}

/* ============================ INQUIRY =================================== */


/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

#endif
