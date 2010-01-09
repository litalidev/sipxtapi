//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _OsNatDatagramSocket_h_
#define _OsNatDatagramSocket_h_

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "os/OsNatSocketBaseImpl.h"
#include "os/OsDatagramSocket.h"
#include "os/OsMsgQ.h"
#include "os/OsTimer.h"
#include "os/OsQueuedEvent.h"

// DEFINES
#define NAT_MSG_TYPE         (OsMsg::USER_START + 1) /**< Stun Msg type/Id */

// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class NatMsg ;
class OsEvent ;
class OsNatAgentTask;
class OsNatKeepaliveListener;
class OsNotification;


/**
 * NAT_STATUS is used internally to mark the end-state of a NAT attempt.  NAT
 * attempts range from a discovery lookup (what is my external IP address) to 
 * a TURN allocation (relaying).
 */
typedef enum
{
    NAT_STATUS_UNKNOWN,     /** Unknown NAT status -- either idle or pending */
    NAT_STATUS_SUCCESS,     /** NAT binding/allocation was successful */
    NAT_STATUS_FAILURE      /** NAT binding/allocation was unsuccessful (no 
                                response or negative response) */
} NAT_STATUS ;


typedef enum
{
   NO_BINDING,
   STUN_BINDING,
   TURN_BINDING,
   STUN_TURN_BINDING
} NAT_BINDING;

/**
 * STUN_STATE holds state information for a STUN discovery binding/attempt 
 */
typedef struct
{
    bool       bEnabled ;       /** Is STUN enabled? */
    NAT_STATUS status ;         /** What is the stun status? */
  
    UtlString  mappedAddress ;  /** Mapped STUN address */
    int        mappedPort ;     /** Mapped STUN port */
    
} STUN_STATE ;


/**
 * TURN_STATE holds state information for a TURN allocation
 */
typedef struct
{
    bool       bEnabled ;   /** Is TURN enabled? */
    NAT_STATUS status ;     /** What is the turn status */

    UtlString  relayAddress ;   /** TURN relay address */
    int        relayPort ;      /** TURN relay port */

} TURN_STATE ;

/**
 * OsNatDatagramSocket extends an OsDatagramSocket by adding an integrated
 * STUN and TURN client.  If STUN or TURN is enabled, request will be send 
 * to the designated server every refresh period.  The external addresses 
 * obtained by these mechanisms are retrieved by invoking getMappedIp and
 * getRelayIp.
 *
 * For this mechanism to work, someone must pump inbound socket data by 
 * calling one of the ::read() methods.  Otherwise, the packets will not be 
 * received/processed.  Internally, the implementation peeks at the read 
 * data and passes the message to the OsNatAgentTask for processing.
 */
class OsNatDatagramSocket : public OsDatagramSocket, public OsNatSocketBaseImpl
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
    friend class OsNatAgentTask ;

/* ============================ CREATORS ================================== */
    /**
     * Constructor accepting the remote host port, name, localhost port, 
     * name, and stun attributes.
     *
     * @param remoteHostPort Port of the remote host for connection-like use of
     *        OsDatagramSocket.  Alternatively, you can also use the ::WriteTo
     *        method and include the host/port at sent time.
     * @param remoteHostName Hostname of remote host for a connection-like use
     *        of OsDatagramSocket.
     * @param localHostPort Local port number for the socket,
     *        PORT_DEFAULT to autoselect
     * @param localHostName Local host name for the socket (e.g. which interface
     *        to bind on.
     * @param pStunNotificationQueue Optional queue for receiving initial STUN
     *        notifications.
     */
    OsNatDatagramSocket(int remoteHostPort, 
                        const char* remoteHostName, 
                        int localHostPort = PORT_DEFAULT,
                        const char* localHostName = NULL,
                        OsMsgQ* pStunNotificationQueue = NULL) ;

    /**
     * Standard Destructor
     */
    virtual ~OsNatDatagramSocket();

/* ============================ MANIPULATORS ============================== */

    virtual OsSocket* getSocket();
    
    /**
     * Standard read, see OsDatagramSocket for details.
     */
    virtual int read(char* buffer, int bufferLength) ;

    /**
     * Standard read, see OsDatagramSocket for details.
     */
    virtual int read(char* buffer, int bufferLength,
            UtlString* ipAddress, int* port);

    /**
     * Standard read, see OsDatagramSocket for details.
     */
    virtual int read(char* buffer, int bufferLength,
            struct in_addr* ipAddress, int* port);

    /**
     * Standard read, see OsDatagramSocket for details.
     */
    virtual int read(char* buffer, int bufferLength, long waitMilliseconds);


    /**
     * Standard write, see OsDatagramSocket for details.
     */
    virtual int write(const char* buffer, int bufferLength);

    /**
     * Standard write - used to invoke the base class write,
     * without timestamping.
     */
    virtual int socketWrite(const char* buffer, int bufferLength,
                      const char* ipAddress, int port, PacketType packetType=UNKNOWN_PACKET);

    /**
     * Standard write, see OsDatagramSocket for details.
     */
    virtual int write(const char* buffer, int bufferLength,
                      const char* ipAddress, int port);

    /**
     * Standard write, see OsDatagramSocket for details.
     */
    virtual int write(const char* buffer, int bufferLength, 
                      long waitMilliseconds);

    /** 
     * Enable STUN.  Enabling STUN will reset the the keep alive timer and 
     * will force a binding refresh.
     *
     * @param szStunServer
     * @param stunPort
     * @param iKeepAlive how often to resend STUN request
     * @param stunOptions
     * @param bReadFromSocket when true method will block until STUN result is available
     */
    virtual void enableStun(const char* szStunServer, 
                            int stunPort,
                            int iKeepAlive, 
                            int stunOptions, 
                            bool bReadFromSocket) ;

    /**
     * Disable STUN.  Disabling STUN will stop all keep alives and cause 
     * getMappedIp to fail.  
     */
    virtual void disableStun() ;


    /** 
     * Enable STUN.  Enabling STUN will reset the the keep alive timer and 
     * will force a binding refresh.
     *
     * @param szTurnSever
     * @param turnPort
     * @param iKeepAlive
     * @param username
     * @param password
     * @param bReadFromSocket
     */
    virtual void enableTurn(const char* szTurnSever,
                            int turnPort,
                            int iKeepAlive,
                            const char* username,
                            const char* password,  
                            bool bReadFromSocket) ;

    /**
     * Disable TURN.  Disabling TURN will stop all keep alives and cause 
     * getRelayIp to fail.  
     */
    virtual void disableTurn() ;

    /**
     * Waits for result of STUN/TURN binding on this socket. Must be
     * called after STUN/TURN is enabled and reading is not done from
     * socket.
     *
     * @param binding Binding type to wait for.
     * @param bWaitUntilReady Whether we should block until bindings are
     *        ready
     * @return True if this function needs to be called again, because
     *         some of bindings are not yet available. False if all requested
     *         bindings are ready.
     */
    virtual bool waitForBinding(NAT_BINDING binding, bool bWaitUntilReady);

    /**
     * When a stun packet is received this socket can either call read again
     * to obtain the next packet or return with zero bytes read.  By default
     * the socket will transparently call Read again and will block until a
     * non-stun packet is read. Calling this method will effect the next 
     * read -- in other words it will not unblock an active read.
     *
     * @param bEnable True to enable transparent stun reads and block until
     *        a non-stun packet is received (default) or False to return 
     *        with zero bytes read if a stun packet is received.
     */ 
    virtual void enableTransparentReads(bool bEnable) ;


    /**
     * Add an alternate destination to this OsNatDatagramSocket.  Alternate 
     * destinations are tested by sending stun packets.  If a stun response is
     * received and the priority is greater than what has already been selected
     * then that address is used.
     * 
     * @param szAddress IP address of the alternate destination
     * @param iPort port number of the alternate destination
     * @param priority priority of the alternate where a higher number 
     *        indicates a higher priority.
     */
    virtual void addAlternateDestination(const char* szAddress, int iPort, int priority) ;


    /**
     * Set a notification object to be signaled when the first the data 
     * packet is received from the socket.  Once this is signaled, the 
     * notification object is discarded.
     */
    virtual void setReadNotification(OsNotification* pNotification) ;


    /**
     * Prepares a destination under TURN usage.
     */
    virtual void readyDestination(const char* szAddress, int iPort) ;

    /**
     * Sets event queue which receives notifications about STUN success/failures.
     * Only one notification is sent per socket (either success or failure).
     *
     * @param pNotificationQueue queue which will receive notification events
     */ 
    virtual void setStunNotificationQueue(OsMsgQ* pNotificationQueue);

    virtual UtlBoolean addCrLfKeepAlive(const char* szRemoteIp,
                                        const int   remotePort, 
                                        const int   keepAliveSecs,
                                        OsNatKeepaliveListener* pListener) ;

    virtual UtlBoolean removeCrLfKeepAlive(const char* szRemoteIp, 
                                          const int   remotePort) ;

    virtual UtlBoolean addStunKeepAlive(const char* szRemoteIp, 
                                        const int   remotePort, 
                                        const int   keepAliveSecs,
                                        OsNatKeepaliveListener* pListener) ;

    virtual UtlBoolean removeStunKeepAlive(const char* szRemoteIp, 
                                          const int   remotePort) ;

/* ============================ ACCESSORS ================================= */

   /**
    * Return the external mapped IP address for this socket.  This method will 
    * return false if stun is disabled, it was unable to retrieve a stun 
    * binding, or both the ip and port parameters are null.
    *
    * @param ip Buffer to place STUN-discovered IP address
    * @param port Buffer to place STUN-discovered port number
    */
   virtual UtlBoolean getMappedIp(UtlString* ip, int* port) ;


    /**
     * Return the external relay IP address for this socket.  This method will
     * return false if stun is disabled, it was unable to retrieve a turn 
     * allocation, or both the ip and port parameters are null.
     *
     * @param ip Buffer to place TURN-discovered IP address
     * @param port Buffer to place TURN-discovered port number
     */
   virtual UtlBoolean getRelayIp(UtlString* ip, int* port) ;

   /**
    * TODO: DOCS
    */
   virtual UtlBoolean getBestDestinationAddress(UtlString& address, int& iPort) ;

   /**
    * TODO: DOCS
    */
   virtual UtlBoolean applyDestinationAddress(const char* szAddress, int iPort, UtlBoolean enableStunKeepalive, int stunKeepAliveSecs, UtlBoolean enableTurnKeepalive);
   
   virtual void destroy();



/* ============================ INQUIRY =================================== */


/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

    /**
     * Set the STUN-derived address for this socket.
     *
     * @param address STUN-derived hostname/IP address
     * @param iPort STUN-derived port address
     */ 
    void setStunAddress(const UtlString& address, const int iPort) ;


    /**
     * Set the TURN-dervied relay address for this socket. 
     *
     * @param address TURN-derived hostname/IP address
     * @param iPort TURN-derived port address
     */
    void setTurnAddress(const UtlString& address, const int iPort) ;


    /**
     * Report that a stun attempt failed.
     */
    void markStunFailure() ;


    /**
     * Report that a stun attempt succeeded.
     */
    void markStunSuccess(bool bAddressChanged) ;


    /**
     * Report that a stun attempt failed.
     */
    void markTurnFailure() ;


    /**
     * Report that a stun attempt succeeded.
     */
    void markTurnSuccess() ;


    /**
     * Reset the destination address for this OsNatDatagramSocket.  This
     * method is called by the OsStunAgentTask when a better address is 
     * found via STUN/ICE.
     *
     * @param address The new destination address
     * @param port The new destination port
     * @param priority Priority of the destination address
     */
    void evaluateDestinationAddress(const UtlString& address, int iPort, int priority) ;       

    /* ICE Settings */
    int miDestPriority ;        /**< Priority of destination address / port. */
    UtlString mDestAddress;     /**< Destination address */
    int miDestPort ;            /**< Destination port */      

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
    STUN_STATE mStunState ; /**< STUN status/state */
    TURN_STATE mTurnState ; /**< TURN status/state */


    /* Global Attributes */
    OsNatAgentTask* mpNatAgent;      /**< Pointer to Nat agent task (handles refreshes) */
    bool mbTransparentReads ;        /**< Block until a non-stun/turn packet is read */
    OsMsgQ* mpStunNotificationQueue; /** Notify on initial stun success or failure */
    bool mbStunNotified;     /** Have we notified the requestor? */
};

/* ============================ INLINE METHODS ============================ */


/* ///////////////////////// HELPER CLASSES /////////////////////////////// */



#endif  // _OsNatDatagramSocket_h_

