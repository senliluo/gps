/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef LE_GPIOPIN2_MESSAGES_H_INCLUDE_GUARD
#define LE_GPIOPIN2_MESSAGES_H_INCLUDE_GUARD


#include "legato.h"

#define PROTOCOL_ID_STR "53b428577c18e3558699d57262acfc23"

#ifdef MK_TOOLS_BUILD
    extern const char** le_gpioPin2_ServiceInstanceNamePtr;
    #define SERVICE_INSTANCE_NAME (*le_gpioPin2_ServiceInstanceNamePtr)
#else
    #define SERVICE_INSTANCE_NAME "le_gpio"
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

#define _MSGID_le_gpioPin2_SetInput 0
#define _MSGID_le_gpioPin2_SetPushPullOutput 1
#define _MSGID_le_gpioPin2_SetTriStateOutput 2
#define _MSGID_le_gpioPin2_SetOpenDrainOutput 3
#define _MSGID_le_gpioPin2_EnablePullUp 4
#define _MSGID_le_gpioPin2_EnablePullDown 5
#define _MSGID_le_gpioPin2_DisableResistors 6
#define _MSGID_le_gpioPin2_Activate 7
#define _MSGID_le_gpioPin2_Deactivate 8
#define _MSGID_le_gpioPin2_SetHighZ 9
#define _MSGID_le_gpioPin2_Read 10
#define _MSGID_le_gpioPin2_AddChangeEventHandler 11
#define _MSGID_le_gpioPin2_RemoveChangeEventHandler 12
#define _MSGID_le_gpioPin2_SetEdgeSense 13
#define _MSGID_le_gpioPin2_DisableEdgeSense 14
#define _MSGID_le_gpioPin2_IsOutput 15
#define _MSGID_le_gpioPin2_IsInput 16
#define _MSGID_le_gpioPin2_GetEdgeSense 17
#define _MSGID_le_gpioPin2_GetPolarity 18
#define _MSGID_le_gpioPin2_IsActive 19
#define _MSGID_le_gpioPin2_GetPullUpDown 20


#endif // LE_GPIOPIN2_MESSAGES_H_INCLUDE_GUARD

