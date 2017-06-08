/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * @page c_ulpm Ultra Low Power Mode
 *
 * @ref le_ulpm_interface.h "API Reference"
 *
 * <HR>
 *
 * This API is used to set the boot sources and switch the device to ultra-low power state. Ultra-
 * low power mode is achieved by shutting down major components (e.g. app processor, modem, etc.)
 * while keeping an ultra-low power component alive. This ultra-low power component is used to
 * monitor boot (or wakeup) sources that are set before switching to ultra-low power mode.
 *
 * @section API_Usage_usage Typical Usage
 *
 * Typically, this API is used like this:
 *
 *   - Boot sources are set by calling le_ulpm_BootOnGpio()/le_ulpm_BootOnTimer(). If multiple
 *      boot sources are configured, the module will boot if any of the boot sources are triggered.
 *
 *   - After configuring boot source, le_ulpm_ShutDown() can be called to initiate shutdown
 *      (i.e. shutt down all major components like the app processor, modem, etc.).
 *
 * @section ulpm_example Sample Code
 *
 * This C code sample calls low power manager API to switch low power mode:
 *
 *  @code
 *
 *  void SwitchToLowPowerMode
 *  (
 *      void
 *  )
 *  {
 *      char version[LE_ULPM_MAX_VERS_LEN+1];
 *
 *      // Get ultra low power manager firmware version
 *      LE_FATAL_IF(le_ulpm_GetFirmwareVersion(version, sizeof(version)) != LE_OK,
 *          "Failed to get ultra low power firmware version");
 *
 *       LE_INFO("Ultra Low Power Manager Firmware version: %s", version);
 *
 *      // Boot after 1000 second of shutdown.
 *      LE_ERROR_IF(le_ulpm_BootOnTimer(1000) != LE_OK, "Can't set timer as boot source");
 *
 *      // Boot if GPIO36 voltage level is high.
 *      LE_ERROR_IF(le_ulpm_BootOnGpio(36, LE_ULPM_GPIO_HIGH) != LE_OK, "Can't set gpio36 as boot source");
 *
 *      // Boot if GPIO38 voltage level is low.
 *      LE_ERROR_IF(le_ulpm_BootOnGpio(38, LE_ULPM_GPIO_LOW) != LE_OK, "Can't set gpio38 as boot source");
 *
 *      // Initiate shutdown.
 *      LE_ERROR_IF(le_ulpm_ShutDown() != LE_OK, "Can't initiate shutdown");
 *  }
 *
 *  @endcode
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
/**
 * @file le_ulpm_interface.h
 *
 * Legato @ref c_ulpm include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LE_ULPM_INTERFACE_H_INCLUDE_GUARD
#define LE_ULPM_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 *
 * Connect the current client thread to the service providing this API. Block until the service is
 * available.
 *
 * For each thread that wants to use this API, either ConnectService or TryConnectService must be
 * called before any other functions in this API.  Normally, ConnectService is automatically called
 * for the main thread, but not for any other thread. For details, see @ref apiFilesC_client.
 *
 * This function is created automatically.
 */
//--------------------------------------------------------------------------------------------------
void le_ulpm_ConnectService
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * Try to connect the current client thread to the service providing this API. Return with an error
 * if the service is not available.
 *
 * For each thread that wants to use this API, either ConnectService or TryConnectService must be
 * called before any other functions in this API.  Normally, ConnectService is automatically called
 * for the main thread, but not for any other thread. For details, see @ref apiFilesC_client.
 *
 * This function is created automatically.
 *
 * @return
 *  - LE_OK if the client connected successfully to the service.
 *  - LE_UNAVAILABLE if the server is not currently offering the service to which the client is bound.
 *  - LE_NOT_PERMITTED if the client interface is not bound to any service (doesn't have a binding).
 *  - LE_COMM_ERROR if the Service Directory cannot be reached.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ulpm_TryConnectService
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * Disconnect the current client thread from the service providing this API.
 *
 * Normally, this function doesn't need to be called. After this function is called, there's no
 * longer a connection to the service, and the functions in this API can't be used. For details, see
 * @ref apiFilesC_client.
 *
 * This function is created automatically.
 */
//--------------------------------------------------------------------------------------------------
void le_ulpm_DisconnectService
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Maximum version length of mcu firmware.
 */
//--------------------------------------------------------------------------------------------------
#define LE_ULPM_MAX_VERS_LEN 8


//--------------------------------------------------------------------------------------------------
/**
 * State of gpio pin. This state will be used to exit from low power state.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_ULPM_GPIO_LOW = 0,
        ///< Gpio voltage level low.

    LE_ULPM_GPIO_HIGH = 1,
        ///< Gpio voltage level high.

    LE_ULPM_GPIO_RISING = 2,
        ///< Gpio edge rising.

    LE_ULPM_GPIO_FALLING = 3,
        ///< Gpio edge falling.

    LE_ULPM_GPIO_BOTH = 4,
        ///< Gpio edge either rising or falling.

    LE_ULPM_GPIO_OFF = 5
        ///< Gpio off.
}
le_ulpm_GpioState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Boot on changing of a gpio state. Gpio number is specified as parameter.
 *
 * @return
 *      - LE_OK if specified gpio is configured as boot source.
 *      - LE_FAULT if failed to configure.
 *
 * @note The process exits if invalid gpio number is passed.  Check corresponding device documents
 * for valid list of gpio.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ulpm_BootOnGpio
(
    uint32_t gpioNum,
        ///< [IN] Gpio number to boot.

    le_ulpm_GpioState_t state
        ///< [IN] State which should cause boot.
);

//--------------------------------------------------------------------------------------------------
/**
 * Boot after expiration of timer interval.
 *
 * @return
 *      - LE_OK if specified timer is configured as boot source.
 *      - LE_FAULT if failed to configure.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ulpm_BootOnTimer
(
    uint32_t expiryVal
        ///< [IN] Expiration time(in second) to boot. This is relative time from
        ///<      modem/app processor shutdown.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the ultra low power manager firmware version.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if version string to big to fit in provided buffer
 *      - LE_FAULT for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ulpm_GetFirmwareVersion
(
    char* version,
        ///< [OUT] Firmware version string

    size_t versionNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Initiate shutting down of app processor/modem etc.
 *
 * @return
 *      - LE_OK if entry to ultra low power mode initiates properly.
 *      - LE_NOT_POSSIBLE if shutting is not possible now. Try again.
 *      - LE_FAULT if failed to initiate.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ulpm_ShutDown
(
    void
);


#endif // LE_ULPM_INTERFACE_H_INCLUDE_GUARD

