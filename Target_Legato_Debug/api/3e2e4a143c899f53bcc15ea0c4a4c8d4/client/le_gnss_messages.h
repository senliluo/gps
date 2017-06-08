/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef LE_GNSS_MESSAGES_H_INCLUDE_GUARD
#define LE_GNSS_MESSAGES_H_INCLUDE_GUARD


#include "legato.h"

#define PROTOCOL_ID_STR "ff84ab34602f8817fcf4a7debcb99100"

#ifdef MK_TOOLS_BUILD
    extern const char** le_gnss_ServiceInstanceNamePtr;
    #define SERVICE_INSTANCE_NAME (*le_gnss_ServiceInstanceNamePtr)
#else
    #define SERVICE_INSTANCE_NAME "le_gnss"
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

#define _MSGID_le_gnss_SetConstellation 0
#define _MSGID_le_gnss_GetConstellation 1
#define _MSGID_le_gnss_EnableExtendedEphemerisFile 2
#define _MSGID_le_gnss_DisableExtendedEphemerisFile 3
#define _MSGID_le_gnss_LoadExtendedEphemerisFile 4
#define _MSGID_le_gnss_GetExtendedEphemerisValidity 5
#define _MSGID_le_gnss_Start 6
#define _MSGID_le_gnss_Stop 7
#define _MSGID_le_gnss_ForceHotRestart 8
#define _MSGID_le_gnss_ForceWarmRestart 9
#define _MSGID_le_gnss_ForceColdRestart 10
#define _MSGID_le_gnss_ForceFactoryRestart 11
#define _MSGID_le_gnss_GetTtff 12
#define _MSGID_le_gnss_Enable 13
#define _MSGID_le_gnss_Disable 14
#define _MSGID_le_gnss_SetAcquisitionRate 15
#define _MSGID_le_gnss_GetAcquisitionRate 16
#define _MSGID_le_gnss_AddPositionHandler 17
#define _MSGID_le_gnss_RemovePositionHandler 18
#define _MSGID_le_gnss_GetPositionState 19
#define _MSGID_le_gnss_GetLocation 20
#define _MSGID_le_gnss_GetAltitude 21
#define _MSGID_le_gnss_GetTime 22
#define _MSGID_le_gnss_GetGpsTime 23
#define _MSGID_le_gnss_GetTimeAccuracy 24
#define _MSGID_le_gnss_GetDate 25
#define _MSGID_le_gnss_GetHorizontalSpeed 26
#define _MSGID_le_gnss_GetVerticalSpeed 27
#define _MSGID_le_gnss_GetDirection 28
#define _MSGID_le_gnss_GetSatellitesInfo 29
#define _MSGID_le_gnss_GetSatellitesStatus 30
#define _MSGID_le_gnss_GetDop 31
#define _MSGID_le_gnss_GetAltitudeOnWgs84 32
#define _MSGID_le_gnss_GetMagneticDeviation 33
#define _MSGID_le_gnss_GetLastSampleRef 34
#define _MSGID_le_gnss_ReleaseSampleRef 35
#define _MSGID_le_gnss_SetSuplAssistedMode 36
#define _MSGID_le_gnss_GetSuplAssistedMode 37
#define _MSGID_le_gnss_SetSuplServerUrl 38
#define _MSGID_le_gnss_InjectSuplCertificate 39
#define _MSGID_le_gnss_DeleteSuplCertificate 40
#define _MSGID_le_gnss_SetNmeaSentences 41
#define _MSGID_le_gnss_GetNmeaSentences 42


#endif // LE_GNSS_MESSAGES_H_INCLUDE_GUARD

