/*
 * AUTO-GENERATED _componentMain.c for the gpsComponent component.

 * Don't bother hand-editing this file.
 */

#include "legato.h"
#include "../src/eventLoop.h"
#include "../src/log.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const char* _gpsComponent_le_mrc_ServiceInstanceName;
const char** le_mrc_ServiceInstanceNamePtr = &_gpsComponent_le_mrc_ServiceInstanceName;
void le_mrc_ConnectService(void);
extern const char* _gpsComponent_le_sms_ServiceInstanceName;
const char** le_sms_ServiceInstanceNamePtr = &_gpsComponent_le_sms_ServiceInstanceName;
void le_sms_ConnectService(void);
extern const char* _gpsComponent_le_posCtrl_ServiceInstanceName;
const char** le_posCtrl_ServiceInstanceNamePtr = &_gpsComponent_le_posCtrl_ServiceInstanceName;
void le_posCtrl_ConnectService(void);
extern const char* _gpsComponent_le_pos_ServiceInstanceName;
const char** le_pos_ServiceInstanceNamePtr = &_gpsComponent_le_pos_ServiceInstanceName;
void le_pos_ConnectService(void);
extern const char* _gpsComponent_le_gnss_ServiceInstanceName;
const char** le_gnss_ServiceInstanceNamePtr = &_gpsComponent_le_gnss_ServiceInstanceName;
void le_gnss_ConnectService(void);
extern const char* _gpsComponent_le_gpioPin2_ServiceInstanceName;
const char** le_gpioPin2_ServiceInstanceNamePtr = &_gpsComponent_le_gpioPin2_ServiceInstanceName;
void le_gpioPin2_ConnectService(void);
extern const char* _gpsComponent_le_gpioPin13_ServiceInstanceName;
const char** le_gpioPin13_ServiceInstanceNamePtr = &_gpsComponent_le_gpioPin13_ServiceInstanceName;
void le_gpioPin13_ConnectService(void);
extern const char* _gpsComponent_le_ulpm_ServiceInstanceName;
const char** le_ulpm_ServiceInstanceNamePtr = &_gpsComponent_le_ulpm_ServiceInstanceName;
void le_ulpm_ConnectService(void);
extern const char* _gpsComponent_le_bootReason_ServiceInstanceName;
const char** le_bootReason_ServiceInstanceNamePtr = &_gpsComponent_le_bootReason_ServiceInstanceName;
void le_bootReason_ConnectService(void);
// Component log session variables.
le_log_SessionRef_t gpsComponent_LogSession;
le_log_Level_t* gpsComponent_LogLevelFilterPtr;

// Component initialization function (COMPONENT_INIT).
void _gpsComponent_COMPONENT_INIT(void);

// Library initialization function.
// Will be called by the dynamic linker loader when the library is loaded.
__attribute__((constructor)) void _gpsComponent_Init(void)
{
    LE_DEBUG("Initializing gpsComponent component library.");

    // Connect client-side IPC interfaces.
    le_mrc_ConnectService();
    le_sms_ConnectService();
    le_posCtrl_ConnectService();
    le_pos_ConnectService();
    le_gnss_ConnectService();
    le_gpioPin2_ConnectService();
    le_gpioPin13_ConnectService();
    le_ulpm_ConnectService();
    le_bootReason_ConnectService();

    // Register the component with the Log Daemon.
    gpsComponent_LogSession = log_RegComponent("gpsComponent", &gpsComponent_LogLevelFilterPtr);

    //Queue the COMPONENT_INIT function to be called by the event loop
    event_QueueComponentInit(_gpsComponent_COMPONENT_INIT);
}


#ifdef __cplusplus
}
#endif
