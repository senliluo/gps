/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef LE_SMS_MESSAGES_H_INCLUDE_GUARD
#define LE_SMS_MESSAGES_H_INCLUDE_GUARD


#include "legato.h"

#define PROTOCOL_ID_STR "d26b22be0fe85bc15a90af10f7d8a763"

#ifdef MK_TOOLS_BUILD
    extern const char** le_sms_ServiceInstanceNamePtr;
    #define SERVICE_INSTANCE_NAME (*le_sms_ServiceInstanceNamePtr)
#else
    #define SERVICE_INSTANCE_NAME "le_sms"
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

#define _MSGID_le_sms_AddRxMessageHandler 0
#define _MSGID_le_sms_RemoveRxMessageHandler 1
#define _MSGID_le_sms_AddFullStorageEventHandler 2
#define _MSGID_le_sms_RemoveFullStorageEventHandler 3
#define _MSGID_le_sms_Create 4
#define _MSGID_le_sms_SetTimeout 5
#define _MSGID_le_sms_SetDestination 6
#define _MSGID_le_sms_SetText 7
#define _MSGID_le_sms_SetBinary 8
#define _MSGID_le_sms_SetUCS2 9
#define _MSGID_le_sms_SetPDU 10
#define _MSGID_le_sms_Send 11
#define _MSGID_le_sms_SendAsync 12
#define _MSGID_le_sms_Get3GPP2ErrorCode 13
#define _MSGID_le_sms_GetErrorCode 14
#define _MSGID_le_sms_GetPlatformSpecificErrorCode 15
#define _MSGID_le_sms_SendText 16
#define _MSGID_le_sms_SendPdu 17
#define _MSGID_le_sms_Delete 18
#define _MSGID_le_sms_GetFormat 19
#define _MSGID_le_sms_GetType 20
#define _MSGID_le_sms_GetCellBroadcastId 21
#define _MSGID_le_sms_GetCellBroadcastSerialNumber 22
#define _MSGID_le_sms_GetSenderTel 23
#define _MSGID_le_sms_GetTimeStamp 24
#define _MSGID_le_sms_GetUserdataLen 25
#define _MSGID_le_sms_GetText 26
#define _MSGID_le_sms_GetBinary 27
#define _MSGID_le_sms_GetUCS2 28
#define _MSGID_le_sms_GetPDU 29
#define _MSGID_le_sms_GetPDULen 30
#define _MSGID_le_sms_DeleteFromStorage 31
#define _MSGID_le_sms_CreateRxMsgList 32
#define _MSGID_le_sms_DeleteList 33
#define _MSGID_le_sms_GetFirst 34
#define _MSGID_le_sms_GetNext 35
#define _MSGID_le_sms_GetStatus 36
#define _MSGID_le_sms_MarkRead 37
#define _MSGID_le_sms_MarkUnread 38
#define _MSGID_le_sms_GetSmsCenterAddress 39
#define _MSGID_le_sms_SetSmsCenterAddress 40
#define _MSGID_le_sms_SetPreferredStorage 41
#define _MSGID_le_sms_GetPreferredStorage 42
#define _MSGID_le_sms_ActivateCellBroadcast 43
#define _MSGID_le_sms_DeactivateCellBroadcast 44
#define _MSGID_le_sms_ActivateCdmaCellBroadcast 45
#define _MSGID_le_sms_DeactivateCdmaCellBroadcast 46
#define _MSGID_le_sms_AddCellBroadcastIds 47
#define _MSGID_le_sms_RemoveCellBroadcastIds 48
#define _MSGID_le_sms_ClearCellBroadcastIds 49
#define _MSGID_le_sms_AddCdmaCellBroadcastServices 50
#define _MSGID_le_sms_RemoveCdmaCellBroadcastServices 51
#define _MSGID_le_sms_ClearCdmaCellBroadcastServices 52


#endif // LE_SMS_MESSAGES_H_INCLUDE_GUARD

