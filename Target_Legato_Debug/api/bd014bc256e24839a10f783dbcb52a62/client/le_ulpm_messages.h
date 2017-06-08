/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef LE_ULPM_MESSAGES_H_INCLUDE_GUARD
#define LE_ULPM_MESSAGES_H_INCLUDE_GUARD


#include "legato.h"

#define PROTOCOL_ID_STR "0396910cefbe1c3cb64549056dee1dfa"

#ifdef MK_TOOLS_BUILD
    extern const char** le_ulpm_ServiceInstanceNamePtr;
    #define SERVICE_INSTANCE_NAME (*le_ulpm_ServiceInstanceNamePtr)
#else
    #define SERVICE_INSTANCE_NAME "le_ulpm"
#endif


// todo: This will need to depend on the particular protocol, but the exact size is not easy to
//       calculate right now, so in the meantime, pick a reasonably large size.  Once interface
//       type support has been added, this will be replaced by a more appropriate size.
#define _MAX_MSG_SIZE 1100

// Define the message type for communicating between client and server
typedef struct
{
    uint32_t id;
    uint8_t buffer[_MAX_MSG_SIZE];
}
_Message_t;

#define _MSGID_le_ulpm_BootOnGpio 0
#define _MSGID_le_ulpm_BootOnTimer 1
#define _MSGID_le_ulpm_GetFirmwareVersion 2
#define _MSGID_le_ulpm_ShutDown 3


#endif // LE_ULPM_MESSAGES_H_INCLUDE_GUARD

