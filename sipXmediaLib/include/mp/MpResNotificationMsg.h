//  
// Copyright (C) 2007 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _MpResNotificationMsg_h_
#define _MpResNotificationMsg_h_

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include "os/OsMsg.h"
#include "utl/UtlString.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

/// Message object used to communicate with the media processing task
class MpResNotificationMsg : public OsMsg
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   /// Phone set message types
   typedef enum
   {
      MPRNM_MESSAGE_INVALID, ///< Message type is invalid (similar to NULL)
      MPRNM_MESSAGE_ALL = MPRNM_MESSAGE_INVALID, ///< Select all message types (used in enabling/disabling)

      // MPRNM_MIXER_NEWFOCUS,

      // Add new built in resource notification messages above

      // Non-builtin resource notification messages
      MPRNM_EXTERNAL_MESSAGE_START = 128
      // Do not add new message types after this
   } RNMsgType;

   /* ============================ CREATORS ================================== */
   ///@name Creators
   //@{

   /// Constructor
   MpResNotificationMsg(RNMsgType msg, 
                        const UtlString& namedResourceOriginator);

   /// Copy constructor
   MpResNotificationMsg(const MpResNotificationMsg& rMpResNotifyMsg);

   /// Create a copy of this msg object (which may be of a derived type)
   virtual OsMsg* createCopy(void) const;

   /// Destructor
   virtual
      ~MpResNotificationMsg();

   //@}

   /* ============================ MANIPULATORS ============================== */
   ///@name Manipulators
   //@{

   /// Assignment operator
   MpResNotificationMsg& operator=(const MpResNotificationMsg& rhs);

   /// Set the name of the resource this message applies to.
   void setOriginatingResourceName(const UtlString& resourceOriginator);
   /**<
   *  Sets the name of the intended recipient for this message.
   */
   //@}

   /* ============================ ACCESSORS ================================= */
   ///@name Accessors
   //@{

   /// Returns the type of the media resource notification message
   int getMsg(void) const;

   /// Get the name of the resource that originated this message.
   UtlString getOriginatingResourceName(void) const;
   /**<
   *  Returns the name of the MpResource object that originated this
   *  message.
   */

   //@}

   /* ============================ INQUIRY =================================== */
   ///@name Inquiry
   //@{

   //@}

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   UtlString mMsgOriginatorName; ///< Name of the resource that 
   ///< originated this message.
};

/* ============================ INLINE METHODS ============================ */

#endif  // _MpResNotificationMsg_h_
