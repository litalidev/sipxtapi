//
// Copyright (C) 2005-2006 SIPez LLC.
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


// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include "mi/CpMediaInterfaceFactory.h"
#include "os/OsLock.h"
#include "os/OsDatagramSocket.h"
#include "os/OsServerSocket.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
CpMediaInterfaceFactory::CpMediaInterfaceFactory()
    : mlockList(OsMutex::Q_FIFO)
{
    miStartRtpPort = 0 ;
    miLastRtpPort = 0;
    miNextRtpPort = miStartRtpPort ;
}

// Destructor
CpMediaInterfaceFactory::~CpMediaInterfaceFactory()
{
    OsLock lock(mlockList) ;
    
    mlistFreePorts.destroyAll() ;
    mlistBusyPorts.destroyAll() ;
}

/* =========================== DESTRUCTORS ================================ */

/**
 * public interface for destroying this media interface
 */ 
void CpMediaInterfaceFactory::release()
{
   delete this;
}

/* ============================ MANIPULATORS ============================== */


void CpMediaInterfaceFactory::setRtpPortRange(int startRtpPort, int lastRtpPort) 
{
    miStartRtpPort = startRtpPort ;
    if (miStartRtpPort < 0)
    {
        miStartRtpPort = 0 ;
    }
    miLastRtpPort = lastRtpPort ;
    miNextRtpPort = miStartRtpPort ;
}

#define MAX_PORT_CHECK_ATTEMPTS     miLastRtpPort - miStartRtpPort
#define MAX_PORT_CHECK_WAIT_MS      50
OsStatus CpMediaInterfaceFactory::getNextRtpPort(int &rtpPort) 
{
    OsLock lock(mlockList) ;
    bool bGoodPort = false ;
    int iAttempts = 0 ;

    // Re-add busy ports to end of free list
    while (mlistBusyPorts.entries())
    {
        UtlInt* pInt = (UtlInt*) mlistBusyPorts.first() ;
        mlistBusyPorts.remove(pInt) ;

        mlistFreePorts.append(pInt) ;
    }

    while (!bGoodPort && (iAttempts < MAX_PORT_CHECK_ATTEMPTS))
    {
        iAttempts++ ;

        // First attempt to get a free port for the free list, if that
        // fails, return a new one. 
        if (mlistFreePorts.entries())
        {
            UtlInt* pInt = (UtlInt*) mlistFreePorts.first() ;
            mlistFreePorts.remove(pInt) ;
            rtpPort = pInt->getValue() ;
            delete pInt ;
        }
        else
        {
            rtpPort = miNextRtpPort ;

            // Only allocate if the nextRtpPort is greater then 0 -- otherwise we
            // are allowing the system to allocate ports.
            if (miNextRtpPort > 0)
            {
                miNextRtpPort += 2 ; 
            }
        }

        bGoodPort = !isPortBusy(rtpPort, MAX_PORT_CHECK_WAIT_MS) ;
        if (!bGoodPort)
        {
            mlistBusyPorts.insert(new UtlInt(rtpPort)) ;
        }
    }

    // If unable to find a usable port, let the system pick one.
    if (!bGoodPort)
    {
        rtpPort = 0 ;
    }
    
    return OS_SUCCESS ;
}


OsStatus CpMediaInterfaceFactory::releaseRtpPort(const int rtpPort) 
{
    OsLock lock(mlockList) ;

    // Only bother noting the free port if the next port isn't 0 (OS selects 
    // port)
    if (miNextRtpPort != 0)
    {
        // if it is not already in the list...
        if (!mlistFreePorts.find(&UtlInt(rtpPort)))
        {
            // Release port to head of list (generally want to reuse ports)
            mlistFreePorts.insert(new UtlInt(rtpPort)) ;
        }
    }

    return OS_SUCCESS ;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

UtlBoolean CpMediaInterfaceFactory::isPortBusy(int iPort, int checkTimeMS) 
{
    UtlBoolean bBusy = FALSE ;

    if (iPort > 0)
    {
        OsDatagramSocket* pSocket = new OsDatagramSocket(0, NULL, iPort, NULL) ;
        if (pSocket != NULL)
        {
            if (!pSocket->isOk() || pSocket->isReadyToRead(checkTimeMS))
            {
                bBusy = true ;
            }
            pSocket->close() ;
            delete pSocket ;
        }
        
        // also check TCP port availability
        OsServerSocket* pTcpSocket = new OsServerSocket(64, iPort, 0, 1);
        if (pTcpSocket != NULL)
        {
            if (!pTcpSocket->isOk())
            {
                bBusy = TRUE;
            }
            pTcpSocket->close();
            delete pTcpSocket;
        }
    }

    return bBusy ;
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */


