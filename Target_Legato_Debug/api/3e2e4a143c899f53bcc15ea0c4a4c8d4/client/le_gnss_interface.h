/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * @page c_gnss GNSS
 *
 * @ref le_gnss_interface.h "API Reference" <br>
 * @ref platformConstraintsGnss <br>
 * @ref howToGNSS "How To Use GNSS" <br>
 * @ref toolsTarget_gnss "GNSS target tool" <br>
 *
 * <HR>
 *
 * This API provides access to the GNSS device.
 *
 * GNSS or Global Navigation Satellite System is a satellite navigation system with global coverage.
 *
 * This API provides function to configure the GNSS device and retrieve position information.
 *
 * @section le_gnss_binding IPC interfaces binding
 *
 * All the functions of this API are provided by the @b positioningService application service.
 *
 * Here's a code sample binding to Positioning services:
 * @verbatim
   bindings:
   {
      clientExe.clientComponent.le_gnss -> positioningService.le_gnss
   }
   @endverbatim
 *
 * @section le_gnss_ControlApi GNSS Control API
 *
 * @subsection le_gnss_EnableDisable Enable/Disable GNSS device
 *
 * The application can enable/disable the GNSS device with the le_gnss_Enable()
 * / le_gnss_Disable() functions.
 * By default the GNSS device is enabled for the positioningService application service.
 * Also see @ref howToGNSS.
 *
 * A sample code can be seen in the following page:
 * - @subpage c_gnssSampleCodeEnableDisable
 *
 * @subsection le_gnss_StartStop Start/Stop GNSS device
 *
 * The application can start/stop the GNSS device with the le_gnss_Start() / le_gnss_Stop()
 * functions.
 * The default "HOT" start condition is applied and all assistance data are used.
 * Also see @ref howToGNSS.
 *
 * A sample code can be seen in the following page:
 * - @subpage c_gnssSampleCodeStartStop
 *
 * @subsubsection le_gnss_StartingMode Starting mode
 *
 * Starting modes are used only for test purposes and allow start performance measurement.
 * @note For more information about start performances, please refer to your specific
 * platform documentation.
 *
 * @subsubsection le_gnss_Data GNSS data
 * The following table describes the <b>minimum</b> required data for those starting modes:
 *
 * | GNSS Data / Starting mode  | HOT  | WARM  | COLD  | FACTORY |
 *| -----------------------| -----| ------| ----- | ------- |
 *| Broadcasted Ephemeris  | Used |  |  |  |
 *| Extended Ephemeris     | Used(1) | Used(2) | Used(2) | Removed (3) |
 *| Approximate Time and Position  | Used | Used |  |  |
 *| Almanac   | Used | Used | Used | Used (Factory) |
 *
 * For example, a requested HOT start without valid broadcasted ephemeris will be treated
 * as a WARM start.
 *
 * @note (1) Extended Ephemeris can be used if Broadcasted Ephemeris are not valid.
 * The Extended Ephemeris could be loaded using the le_gnss_LoadExtendedEphemerisFile()
 * function.
 * @note (2) Extended Ephemeris is used if the Extended Ephemeris file is loaded and valid.
 * @note (3) Extended Ephemeris are removed when a FACTORY start is requested.
 * The Extended Ephemeris could be loaded again using the le_gnss_LoadExtendedEphemerisFile()
 * function.
 *
 * @subsubsection le_gnss_TTFF Time To First Fix (TTFF)
 * The le_gnss_GetTtff() function provides the TTFF (first between 2-Dimensional or 3-Dimensional
 * position fix) of the last position fix.
 * Please refer to @ref le_gnss_APIcallsRequirements. Also see @ref howToGNSS.
 *
 * A sample code can be seen in the following page:
 * - @subpage c_gnssSampleCodeReStart
 *
 * @subsubsection le_gnss_ForceHotRestart Force HOT restart
 * The le_gnss_ForceHotRestart() function performs a "HOT" restart of the GNSS device. The current
 * GNSS session is stopped, then started using the available @ref le_gnss_Data.
 * Please refer to @ref le_gnss_APIcallsRequirements. Also see @ref howToGNSS.
 *
 * @subsubsection le_gnss_ForceWarmRestart Force WARM restart
 * The le_gnss_ForceWarmRestart() function performs a "WARM" restart of the GNSS device. The current
 * GNSS session is stopped, then started using the available @ref le_gnss_Data.
 * Please refer to @ref le_gnss_APIcallsRequirements. Also see @ref howToGNSS.
 *
 * @subsubsection le_gnss_ForceColdRestart Force COLD restart
 * The le_gnss_ForceColdRestart() function performs a "COLD" restart of the GNSS device. The current
 * GNSS session is stopped, then started using the available @ref le_gnss_Data.
 * Please refer to @ref le_gnss_APIcallsRequirements. Also see @ref howToGNSS.
 *
 * @subsubsection le_gnss_ForceFactoryRestart Force FACTORY restart
 * The le_gnss_ForceFactoryRestart() function performs a "FACTORY" restart of the GNSS device.
 * The current GNSS session is stopped, then started using the available @ref le_gnss_Data.
 * Please refer to @ref le_gnss_APIcallsRequirements. Also see @ref howToGNSS.
 *
 * @subsection le_gnss_AcquisitionRate Acquisition rate
 * The GNSS position is computed and delivered each acquisition rate. Default value is 1 second.
 * The application can configure/retrieve the GNSS device acquisition rate with the
 * le_gnss_SetAcquisitionRate() / le_gnss_GetAcquisitionRate() functions.
 *
 * Please refer to @ref le_gnss_APIcallsRequirements.
 *
 * A sample code can be seen in the following page:
 * - @subpage c_gnssSampleCodeAcquisitionRate
 *
 * @subsection le_gnss_SetGetConstellation GNSS constellation selection
 *
 * The le_gnss_SetConstellation() function selects the GNSS constellation(s) used in solution.
 * If all GNSS constellations are disabled, the GNSS engine is disabled.
 *
 * @warning Your platform may require a reboot to take into account this change.
 * Please refer to your platform documentation for further details.
 *
 * The following configurations are currently supported:
 *   - GPS
 *   - GPS + GLONASS
 *
 * All supported GNSS constellations are enabled by default.
 * The le_gnss_GetConstellation() function gets the GNSS constellation(s) enabled to be used
 * in solution.
 *
 * A sample code can be seen in the following page:
 * - @subpage c_gnssSampleCodeConstellation
 *
 * @subsection le_gnss_NmeaSentences NMEA sentences selection
 *
 * The le_gnss_SetNmeaSentences() function selects the enabled NMEA sentences in the
 * @ref le_gnss_NMEA with a bit mask. The supported values are listed in @ref le_gnss_NmeaBitMask_t.
 *
 * @warning Your GNSS device may require a restart to take into account this change.
 * Please refer to your platform documentation for further details.
 *
 * @note All supported NMEA sentences are enabled by default.
 *
 * The le_gnss_GetNmeaSentences() function gets the bit mask of the enabled NMEA sentences in the
 * @ref le_gnss_NMEA.
 *
 * Please refer to @ref le_gnss_APIcallsRequirements.
 *
 * A sample code can be seen in the following page:
 * - @subpage c_gnssSampleCodeNmeaSentences
 *
 * @subsection le_gnss_APIcallsRequirements API calls Requirements
 * The following table shows the pre-requisites when using the GNSS service API function set.
 * ''LE_OK or error code'' means the function is authorized in the corresponding state, the request
 * is performed and the result is returned; otherwise the returned error is indicated for each state
 *.
 * | Function / GNSS state  | UNINITIALIZED  | READY  | ACTIVE  | DISABLED |
 *| ----------------------------- | --------------------------- | -----------------------------| --------------------------- | --------------------------- |
 *| le_gnss_Start() | LE_NOT_PERMITTED | LE_OK or error code | LE_DUPLICATE | LE_NOT_PERMITTED |
 *| le_gnss_Stop() | LE_NOT_PERMITTED | LE_DUPLICATE | LE_OK or error code | LE_NOT_PERMITTED |
 *| le_gnss_ForceHotRestart() | LE_NOT_PERMITTED | LE_NOT_PERMITTED | LE_OK or error code | LE_NOT_PERMITTED |
 *| le_gnss_ForceWarmRestart() | LE_NOT_PERMITTED | LE_NOT_PERMITTED | LE_OK or error code | LE_NOT_PERMITTED |
 *| le_gnss_ForceColdRestart() | LE_NOT_PERMITTED | LE_NOT_PERMITTED | LE_OK or error code | LE_NOT_PERMITTED |
 *| le_gnss_ForceFactoryRestart() | LE_NOT_PERMITTED | LE_NOT_PERMITTED | LE_OK or error code | LE_NOT_PERMITTED |
 *| le_gnss_Disable() | LE_NOT_PERMITTED | LE_OK or error code | LE_NOT_PERMITTED | LE_DUPLICATE |
 *| le_gnss_Enable() | LE_NOT_PERMITTED | LE_DUPLICATE | LE_DUPLICATE | LE_OK or error code |
 *| le_gnss_SetConstellation() | LE_NOT_PERMITTED |  LE_OK or error code | LE_NOT_PERMITTED | LE_NOT_PERMITTED |
 *| le_gnss_GetConstellation() | LE_NOT_PERMITTED |  LE_OK or error code | LE_NOT_PERMITTED | LE_NOT_PERMITTED |
 *| le_gnss_GetTtff() | LE_NOT_PERMITTED |  LE_OK or error code | LE_OK or error code | LE_NOT_PERMITTED |
 *| le_gnss_SetAcquisitionRate() | LE_NOT_PERMITTED | LE_OK or error code | LE_NOT_PERMITTED | LE_NOT_PERMITTED |
 *| le_gnss_GetAcquisitionRate() | LE_NOT_PERMITTED | LE_OK or error code | LE_NOT_PERMITTED | LE_NOT_PERMITTED |
 *| le_gnss_SetNmeaSentences() | LE_NOT_PERMITTED | LE_OK or error code | LE_NOT_PERMITTED | LE_NOT_PERMITTED |
 *| le_gnss_GetNmeaSentences() | LE_NOT_PERMITTED | LE_OK or error code | LE_NOT_PERMITTED | LE_NOT_PERMITTED |
 *
 * Also see @ref howToGNSS.
 *
 * @section le_gnss_Information GNSS position information
 * @ref le_gnss_NMEA
 *
 * @ref le_gnss_GetInfo
 *
 * @subsection le_gnss_NMEA NMEA Flow
 * The National Marine Electronics Association (NMEA) standard defines an electrical interface
 * and data protocol for communications between marine instrumentation.
 *
 * The NMEA-0183, scope of this document, defines a set of frame prefixed by
 * $GP (concerning Global Positioning System),
 * $GL (concerning GLONASS)
 * and $GN (concerning combination of navigation systems).
 * For more details about NMEA standards, please refer to http://www.nmea.org/.
 *
 * That NMEA frames flow can be retrieved from the "/dev/nmea" device folder, using for example
 * the shell command $<EM> cat /dev/nmea | grep '$G'</EM>
 *
 * @subsection le_gnss_GetInfo Get position information
 * The position information is referenced to a position sample object.
 *
 * An application can register a handler to be notified of the
 * updated position each @ref le_gnss_AcquisitionRate, returning a position sample object.
 *
 * The GNSS information commonly used such as
 * position, time and date, satellites information and accuracy
 * can be queried using the following functions:
 * - le_gnss_GetPositionState()
 * - le_gnss_GetLocation()
 * - le_gnss_GetAltitude()
 * - le_gnss_GetDate()
 * - le_gnss_GetTime()
 * - le_gnss_GetGpsTime()
 * - le_gnss_GetTimeAccuracy()
 * - le_gnss_GetHorizontalSpeed()
 * - le_gnss_GetVerticalSpeed()
 * - le_gnss_GetDirection()
 * - le_gnss_GetSatellitesInfo()
 * - le_gnss_GetSatellitesStatus()
 * - le_gnss_GetDop()
 * - le_gnss_GetAltitudeOnWgs84()
 * - le_gnss_GetMagneticDeviation()

 * The handler can be managed using le_gnss_AddPositionHandler()
 * and le_gnss_RemovePositionHandler().
 * When a position is computed, the handler is called.
 *
 * The application has to release each position sample object received by the handler,
 * using the le_gnss_ReleaseSampleRef().
 *
 * A sample code can be seen in the following page:
 * - @subpage c_gnssSampleCodePosition
 *
 * @section le_gnss_Assisted_GNSS Assisted GNSS
 *
 * @ref le_gnss_Assisted_GNSS_EE
 * @ref le_gnss_Assisted_GNSS_UP
 *
 * @subsection le_gnss_Assisted_GNSS_EE Server based Extended Ephemeris
 *
 * @todo Add detailed documentation.
 *
 * With le_gnss_LoadExtendedEphemerisFile() , you can load an 'Extended Ephemeris' file into
 * the GNSS device from the filesystem.
 * You have to download the file before loading it.
 * @warning Ensure to check that the downloaded file is supported for your specific platform.
 *
 * With le_gnss_GetExtendedEphemerisValidity(), you will to get the validity of the last injected
 * Extended Ephemeris.
 *
 * You can enable/disable the use of the 'Extended Ephemeris' file into the GNSS device with
 * le_gnss_EnableExtendedEphemerisFile() / le_gnss_DisableExtendedEphemerisFile() functions.
 * @warning Ensure to check configuration capabilities for your specific platform.
 * A reboot must be required if your platform doesn't allow run-time configuration.
 *
 * A sample code can be seen in the following page:
 * - @subpage c_gnssSampleCodeXtra
 *
 * @subsection le_gnss_Assisted_GNSS_UP 3GPP User Plane (OMA SUPL)
 *
 * That 3GPP User Plane A-GNSS (Assisted GNSS) protocol is defined by two different standardization
 * bodies, 3GPP and Open Mobile Alliance (OMA). For more information, please refer to the standard.
 *
 * Both MS-Assisted and MS-Based position determination methods are supported in the User Plane.
 *
 * In MS-Assisted mode, the MS (Mobile Station) measures the signals from the GNSS satellites
 *, then returns the retrieved GNSS data to the SUPL (Secure User Plan Location) server, where the
 * position calculation is performed.
 *
 * In MS-Based mode, the MS gets the assistance data from the SUPL (Secure User Plan Location)
 * server. The MS measures the signals from the GNSS satellites and makes the position calculation.
 *
 * The data transport over User Plan is done using the TCP/IP protocol.
 *
 * The Assisted-GNSS mode can be configured thru the le_gnss_SetSuplAssistedMode() function.
 *
 * The supported modes are the following:
 * - Standalone mode: That 3GPP User Plane A-GNSS feature is deactivated.
 * - MS-Based mode
 * - MS-Assisted mode
 *
 * Moreover, the le_gnss_GetSuplAssistedMode() function reads the configured Assisted-GNSS mode.
 *
 * The SUPL server is configured using the le_gnss_SetSuplServerUrl() function. That function sets
 * the SUPL server URL and optionally the port number.
 *
 * The SUPL certificate to be used in A-GNSS sessions is injected through the
 * le_gnss_InjectSuplCertificate() function and deleted through the le_gnss_DeleteSuplCertificate()
 * function.
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
/**
 * @page c_gnssSampleCode Sample code for GNSS device
 *
 * @include "apps/test/positioning/gnssTest/gnssTest/gnssTest.c"
 */
/**
 * @page c_gnssSampleCodeEnableDisable Sample code for Enable/Disable GNSS device
 *
 * @snippet "apps/test/positioning/gnssTest/gnssTest/gnssTest.c" GnssEnable
 */
/**
 * @page c_gnssSampleCodeStartStop Sample code for Start/Stop GNSS device
 *
 * @snippet "apps/test/positioning/gnssTest/gnssTest/gnssTest.c" GnssControl
 */
/**
 * @page c_gnssSampleCodeReStart Sample code for restart GNSS device and get TTFF
 *
 * @snippet "apps/test/positioning/gnssTest/gnssTest/gnssTest.c" GnssReStart
 */
/**
 * @page c_gnssSampleCodeAcquisitionRate Sample code for GNSS acquisition rate configuration
 *
 * @snippet "apps/test/positioning/gnssTest/gnssTest/gnssTest.c" GnssControl
 */
/**
 * @page c_gnssSampleCodeConstellation Sample code for GNSS constellation selection
 *
 * @snippet "apps/test/positioning/gnssTest/gnssTest/gnssTest.c" GnssControl
 */
/**
 * @page c_gnssSampleCodeNmeaSentences Sample code for GNSS NMEA sentences selection
 *
 * @snippet "apps/test/positioning/gnssTest/gnssTest/gnssTest.c" GnssControl
 */
/**
 * @page c_gnssSampleCodePosition Sample code for GNSS position information
 *
 * @snippet "apps/test/positioning/gnssTest/gnssTest/gnssTest.c" GnssPosition
 */
/**
 * @page c_gnssSampleCodeXtra Sample code for GNSS Server based Extended Ephemeris
 *
 * @include "apps/test/positioning/gnssXtraTest/gnssXtraTest/xtraTest.c"
 */
/**
 * @file le_gnss_interface.h
 *
 * Legato @ref c_gnss include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LE_GNSS_INTERFACE_H_INCLUDE_GUARD
#define LE_GNSS_INTERFACE_H_INCLUDE_GUARD


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
void le_gnss_ConnectService
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
le_result_t le_gnss_TryConnectService
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
void le_gnss_DisconnectService
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for dealing with GNSS position samples.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_gnss_Sample* le_gnss_SampleRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of the SUP Server URL string.
 */
//--------------------------------------------------------------------------------------------------
#define LE_GNSS_SUPL_SERVER_URL_MAX_LEN 256


//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of the SUP Server URL string.
 * One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define LE_GNSS_SUPL_SERVER_URL_MAX_BYTES 257


//--------------------------------------------------------------------------------------------------
/**
 * Maximum SUPL certificate size.
 */
//--------------------------------------------------------------------------------------------------
#define LE_GNSS_SUPL_CERTIFICATE_MAX_LEN 2000


//--------------------------------------------------------------------------------------------------
/**
 * Maximum SUPL certificate string size.
 * One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define LE_GNSS_SUPL_CERTIFICATE_MAX_BYTES 2001


//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum length of the Satellites Vehicle information list
 */
//--------------------------------------------------------------------------------------------------
#define LE_GNSS_SV_INFO_MAX_LEN 80


//--------------------------------------------------------------------------------------------------
/**
 * Define the maximal bit mask for enabled NMEA sentences
 *
 * @note This maximal value should be coherent with @ref le_gnss_NmeaBitMask_t
 */
//--------------------------------------------------------------------------------------------------
#define LE_GNSS_NMEA_SENTENCES_MAX 32767


//--------------------------------------------------------------------------------------------------
/**
 *  GNSS fix position states.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_GNSS_STATE_FIX_NO_POS = 0,
        ///< The GNSS fix position is not fixed.

    LE_GNSS_STATE_FIX_2D = 1,
        ///< 2-Dimensional position fix.

    LE_GNSS_STATE_FIX_3D = 2,
        ///< 3-Dimensional position fix.

    LE_GNSS_STATE_FIX_ESTIMATED = 3
        ///< Estimated (i.e. forward predicted) position fix.
}
le_gnss_FixState_t;


//--------------------------------------------------------------------------------------------------
/**
 *  GNSS constellation type.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_GNSS_SV_CONSTELLATION_UNDEFINED = 0,
        ///< GNSS constellation field not defined.

    LE_GNSS_SV_CONSTELLATION_GPS = 1,
        ///< Satellite Vehicle of GPS constellation.
        ///< North American satellite navigation system

    LE_GNSS_SV_CONSTELLATION_SBAS = 2,
        ///< Satellite Vehicle of SBAS constellation.
        ///< Satellite-based augmentation system

    LE_GNSS_SV_CONSTELLATION_GLONASS = 3,
        ///< Satellite Vehicle of Glonass constellation.
        ///< Russian satellite navigation system

    LE_GNSS_SV_CONSTELLATION_GALILEO = 4,
        ///< Satellite Vehicle of Galileo constellation.
        ///< European Union satellite navigation system

    LE_GNSS_SV_CONSTELLATION_BEIDOU = 5,
        ///< Satellite Vehicle of Beidou constellation.
        ///< Chinese satellite navigation system

    LE_GNSS_SV_CONSTELLATION_MAX = 6
        ///< Maximum value.
}
le_gnss_Constellation_t;


//--------------------------------------------------------------------------------------------------
/**
 * Assisted-GNSS mode
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_GNSS_STANDALONE_MODE = 0,
        ///< Standalone mode

    LE_GNSS_MS_BASED_MODE = 1,
        ///< MS-Based mode.

    LE_GNSS_MS_ASSISTED_MODE = 2
        ///< MS-Assisted mode.
}
le_gnss_AssistedMode_t;


//--------------------------------------------------------------------------------------------------
/**
 * GNSS constellation Bit Mask (8 bits) indicating the GNSS constellation(s)
 * used in solution.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_GNSS_CONSTELLATION_GPS = 0x1,
        ///< GPS constellation used in solution.
        ///< North American satellite navigation system

    LE_GNSS_CONSTELLATION_GLONASS = 0x2,
        ///< GLONASS constellation used in solution.
        ///< Russian satellite navigation system

    LE_GNSS_CONSTELLATION_BEIDOU = 0x4,
        ///< BEIDOU constellation used in solution.
        ///< Chinese satellite navigation system

    LE_GNSS_CONSTELLATION_GALILEO = 0x8
        ///< GALILEO constellation used in solution.
        ///< European Union satellite navigation system
}
le_gnss_ConstellationBitMask_t;


//--------------------------------------------------------------------------------------------------
/**
 * NMEA sentences Bit Mask indicating the NMEA sentences enabled in the NMEA flow.
 *
 * @warning The supported NMEA sentences depend on the platform. Please refer to your platform
 * documentation for further details.
 *
 * @note The bit mask values should be coherent with @ref LE_GNSS_NMEA_SENTENCES_MAX
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_GNSS_NMEA_MASK_GPGGA = 0x1,
        ///< GPGGA type enabled: GPS fix data.

    LE_GNSS_NMEA_MASK_GPGSA = 0x2,
        ///< GPGSA type enabled: GPS DOP and active satellites.

    LE_GNSS_NMEA_MASK_GPGSV = 0x4,
        ///< GPGSV type enabled: GPS satellites in view.

    LE_GNSS_NMEA_MASK_GPRMC = 0x8,
        ///< GPRMC type enabled: GPS recommended minimum data.

    LE_GNSS_NMEA_MASK_GPVTG = 0x10,
        ///< GPVTG type enabled: GPS vector track and speed over the ground.

    LE_GNSS_NMEA_MASK_GLGSV = 0x20,
        ///< GLGSV type enabled: GLONASS satellites in view.

    LE_GNSS_NMEA_MASK_GNGNS = 0x40,
        ///< GNGNS type enabled: GNSS fix data.

    LE_GNSS_NMEA_MASK_GNGSA = 0x80,
        ///< GNGSA type enabled: GNSS DOP and active satellites.

    LE_GNSS_NMEA_MASK_GAGGA = 0x100,
        ///< GAGGA type enabled: Galileo fix data.

    LE_GNSS_NMEA_MASK_GAGSA = 0x200,
        ///< GAGSA type enabled: Galileo DOP and active satellites.

    LE_GNSS_NMEA_MASK_GAGSV = 0x400,
        ///< GAGSV type enabled: Galileo satellites in view.

    LE_GNSS_NMEA_MASK_GARMC = 0x800,
        ///< GARMC type enabled: Galileo recommended minimum data.

    LE_GNSS_NMEA_MASK_GAVTG = 0x1000,
        ///< GAVTG type enabled: Galileo vector track and speed over the ground.

    LE_GNSS_NMEA_MASK_PSTIS = 0x2000,
        ///< PSTIS type enabled: GPS session start indication.

    LE_GNSS_NMEA_MASK_PQXFI = 0x4000
        ///< PQXFI type enabled: Proprietary Qualcomm eXtended Fix Information.
}
le_gnss_NmeaBitMask_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'le_gnss_Position'
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_gnss_PositionHandler* le_gnss_PositionHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Handler for position information.
 *
 *
 * @param positionSampleRef
 *        Position's sample reference
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_gnss_PositionHandlerFunc_t)
(
    le_gnss_SampleRef_t positionSampleRef,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the GNSS constellation bit mask
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_UNSUPPORTED   If the request is not supported.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized, disabled or active.
 *  - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetConstellation
(
    le_gnss_ConstellationBitMask_t constellationMask
        ///< [IN] GNSS constellation used in solution.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the GNSS constellation bit mask
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetConstellation
(
    le_gnss_ConstellationBitMask_t* constellationMaskPtr
        ///< [OUT] GNSS constellation used in solution.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function enables the use of the 'Extended Ephemeris' file into the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_EnableExtendedEphemerisFile
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function disables the use of the 'Extended Ephemeris' file into the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_DisableExtendedEphemerisFile
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to load an 'Extended Ephemeris' file into the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed to inject the 'Extended Ephemeris' file.
 *  - LE_TIMEOUT       A time-out occurred.
 *  - LE_FORMAT_ERROR  'Extended Ephemeris' file format error.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_LoadExtendedEphemerisFile
(
    int fd
        ///< [IN] Extended ephemeris file descriptor
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the validity of the last injected Extended Ephemeris.
 *
 * @return
 *  - LE_FAULT         The function failed to get the validity
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetExtendedEphemerisValidity
(
    uint64_t* startTimePtr,
        ///< [OUT] Start time in seconds (since Jan. 1, 1970)

    uint64_t* stopTimePtr
        ///< [OUT] Stop time in seconds (since Jan. 1, 1970)
);

//--------------------------------------------------------------------------------------------------
/**
 * This function starts the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_DUPLICATE     If the GNSS device is already started.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized or disabled.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_Start
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function stops the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_DUPLICATE     If the GNSS device is already stopped.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized or disabled.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_Stop
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function performs a "HOT" restart of the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_ForceHotRestart
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function performs a "WARM" restart of the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_ForceWarmRestart
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function performs a "COLD" restart of the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_ForceColdRestart
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function performs a "FACTORY" restart of the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_ForceFactoryRestart
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the TTFF in milliseconds
 *
 * @return
 *  - LE_BUSY          The position is not fixed and TTFF can't be measured.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetTtff
(
    uint32_t* ttffPtr
        ///< [OUT] TTFF in milliseconds
);

//--------------------------------------------------------------------------------------------------
/**
 * This function enables the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_DUPLICATE     If the GNSS device is already enabled.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_Enable
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function disables the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_DUPLICATE     If the GNSS device is already disabled.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized or started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_Disable
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the GNSS device acquisition rate.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED request not supported
 *  - LE_TIMEOUT a time-out occurred
 *  - LE_NOT_PERMITTED If the GNSS device is not in "ready" state.
 *
 * @warning This function may be subject to limitations depending on the platform. Please refer to
 *          the @ref platformConstraintsGnss page.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetAcquisitionRate
(
    uint32_t rate
        ///< [IN] Acquisition rate in milliseconds.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the GNSS device acquisition rate.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_NOT_PERMITTED If the GNSS device is not in "ready" state.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetAcquisitionRate
(
    uint32_t* ratePtr
        ///< [OUT] Acquisition rate in milliseconds.
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_gnss_Position'
 *
 * This event provides information on position.
 *
 *  - A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_gnss_PositionHandlerRef_t le_gnss_AddPositionHandler
(
    le_gnss_PositionHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_gnss_Position'
 */
//--------------------------------------------------------------------------------------------------
void le_gnss_RemovePositionHandler
(
    le_gnss_PositionHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the position sample's fix state
 *
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetPositionState
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    le_gnss_FixState_t* statePtr
        ///< [OUT] Position fix state.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the location's data (Latitude, Longitude, Horizontal accuracy).
 *
 * @return
 *  - LE_FAULT         Function failed to get the location's data
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note latitudePtr, longitudePtr, hAccuracyPtr, altitudePtr, vAccuracyPtr can be set to NULL
 *       if not needed.
 *
 * @note: The latitude and longitude values are based on the WGS84 standard coordinate system.
 *
 * @note The latitude and longitude are given in degrees with 6 decimal places like:
 *    Latitude +48858300 = 48.858300 degrees North
 *    Longitude +2294400 = 2.294400 degrees East
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetLocation
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* latitudePtr,
        ///< [OUT] WGS84 Latitude in degrees, positive North [resolution 1e-6].

    int32_t* longitudePtr,
        ///< [OUT] WGS84 Longitude in degrees, positive East [resolution 1e-6].

    int32_t* hAccuracyPtr
        ///< [OUT] Horizontal position's accuracy in meters [resolution 1e-2].
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's altitude.
 *
 * @return
 *  - LE_FAULT         Function failed to get the altitude.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note Altitude is in meters, above Mean Sea Level, with 3 decimal places (3047 = 3.047 meters).
 *
 * @note For a 2D position fix, the altitude will be indicated as invalid and set to INT32_MAX
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note altitudePtr, altitudeAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetAltitude
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* altitudePtr,
        ///< [OUT] Altitude in meters, above Mean Sea Level [resolution 1e-3].

    int32_t* vAccuracyPtr
        ///< [OUT] Vertical position's accuracy in meters [resolution 1e-1].
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's time.
 *
 * @return
 *  - LE_FAULT         Function failed to get the time.
 *  - LE_OUT_OF_RANGE  The retrieved time is invalid (all fields are set to 0).
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetTime
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint16_t* hoursPtr,
        ///< [OUT] UTC Hours into the day [range 0..23].

    uint16_t* minutesPtr,
        ///< [OUT] UTC Minutes into the hour [range 0..59].

    uint16_t* secondsPtr,
        ///< [OUT] UTC Seconds into the minute [range 0..59].

    uint16_t* millisecondsPtr
        ///< [OUT] UTC Milliseconds into the second [range 0..999].
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's GPS time.
 *
 * @return
 *  - LE_FAULT         Function failed to get the time.
 *  - LE_OUT_OF_RANGE  The retrieved time is invalid (all fields are set to 0).
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetGpsTime
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* gpsWeekPtr,
        ///< [OUT] GPS week number from midnight, Jan. 6, 1980.

    uint32_t* gpsTimeOfWeekPtr
        ///< [OUT] Amount of time in milliseconds into the GPS week.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's time accurary.
 *
 * @return
 *  - LE_FAULT         Function failed to get the time.
 *  - LE_OUT_OF_RANGE  The retrieved time accuracy is invalid (set to UINT16_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetTimeAccuracy
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* timeAccuracyPtr
        ///< [OUT] Estimated time accuracy in milliseconds
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's date.
 *
 * @return
 *  - LE_FAULT         Function failed to get the date.
 *  - LE_OUT_OF_RANGE  The retrieved date is invalid (all fields are set to 0).
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetDate
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint16_t* yearPtr,
        ///< [OUT] UTC Year A.D. [e.g. 2014].

    uint16_t* monthPtr,
        ///< [OUT] UTC Month into the year [range 1...12].

    uint16_t* dayPtr
        ///< [OUT] UTC Days into the month [range 1...31].
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's horizontal speed.
 *
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to UINT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note hSpeedPtr, hSpeedAccuracyPtr can be set to NULL if not needed.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetHorizontalSpeed
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* hspeedPtr,
        ///< [OUT] Horizontal speed in meters/second [resolution 1e-2].

    uint32_t* hspeedAccuracyPtr
        ///< [OUT] Horizontal speed's accuracy estimate
        ///<       in meters/second [resolution 1e-1].
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's vertical speed.
 *
 * @return
 *  - LE_FAULT         The function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX).
 *  - LE_OK            The function succeeded.
 *
 * @note vSpeedPtr, vSpeedAccuracyPtr can be set to NULL if not needed.
 *
 * @note For a 2D position Fix, the vertical speed will be indicated as invalid
 * and set to INT32_MAX.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetVerticalSpeed
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* vspeedPtr,
        ///< [OUT] Vertical speed in meters/second [resolution 1e-2],
        ///<       positive up.

    int32_t* vspeedAccuracyPtr
        ///< [OUT] Vertical speed's accuracy estimate
        ///<       in meters/second [resolution 1e-1].
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's direction. Direction of movement is the direction that the vehicle or
 * person is actually moving.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to UINT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note Direction is given in degrees with 1 decimal place: 1755 = 175,5 degrees.
 *       Direction ranges from 0 to 359.9 degrees, where 0 is True North.
 *
 * @note directionPtr, directionAccuracyPtr can be set to NULL if not needed.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetDirection
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* directionPtr,
        ///< [OUT] Direction in degrees [resolution 1e-1].
        ///<       Range: 0 to 359.9, where 0 is True North

    uint32_t* directionAccuracyPtr
        ///< [OUT] Direction's accuracy estimate
        ///<       in degrees [resolution 1e-1].
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Satellites Vehicle information.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid.
 *  - LE_OK            Function succeeded.
 *
 * @note satId[] can be set to 0 if that information list index is not configured, so
 * all satellite parameters (satConst[], satSnr[],satAzim[], satElev[]) are fixed to 0.
 *
 * @note For LE_OUT_OF_RANGE returned code, invalid value depends on field type:
 * UINT16_MAX for satId, LE_GNSS_SV_CONSTELLATION_UNDEFINED for satConst, false for satUsed,
 * UINT8_MAX for satSnr, UINT16_MAX for satAzim, UINT8_MAX for satElev.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetSatellitesInfo
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint16_t* satIdPtr,
        ///< [OUT] Satellites in View ID number, referring
        ///<       to NMEA standard.

    size_t* satIdNumElementsPtr,
        ///< [INOUT]

    le_gnss_Constellation_t* satConstPtr,
        ///< [OUT] GNSS constellation type.

    size_t* satConstNumElementsPtr,
        ///< [INOUT]

    bool* satUsedPtr,
        ///< [OUT] TRUE if satellite in View Used
        ///<       for Navigation.

    size_t* satUsedNumElementsPtr,
        ///< [INOUT]

    uint8_t* satSnrPtr,
        ///< [OUT] Satellites in View Signal To
        ///<       Noise Ratio [dBHz].

    size_t* satSnrNumElementsPtr,
        ///< [INOUT]

    uint16_t* satAzimPtr,
        ///< [OUT] Satellites in View Azimuth [degrees].
        ///<       Range: 0 to 360
        ///<       If Azimuth angle is currently unknown,
        ///<       the value is set to UINT16_MAX.

    size_t* satAzimNumElementsPtr,
        ///< [INOUT]

    uint8_t* satElevPtr,
        ///< [OUT] Satellites in View Elevation [degrees].
        ///<       Range: 0 to 90
        ///<       If Elevation angle is currently unknown,
        ///<       the value is set to UINT8_MAX.

    size_t* satElevNumElementsPtr
        ///< [INOUT]
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Satellites Vehicle status.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid.
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetSatellitesStatus
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint8_t* satsInViewCountPtr,
        ///< [OUT] Number of satellites expected to be in view.

    uint8_t* satsTrackingCountPtr,
        ///< [OUT] Number of satellites in view, when tracking.

    uint8_t* satsUsedCountPtr
        ///< [OUT] Number of satellites in view used for Navigation.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the DOP parameters (Dilution Of Precision) for the fixed position.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT16_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note The DOP values are given with 3 decimal places like: DOP value 2200 = 2.20
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetDop
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint16_t* hdopPtr,
        ///< [OUT] Horizontal Dilution of Precision [resolution 1e-3].

    uint16_t* vdopPtr,
        ///< [OUT] Vertical Dilution of Precision [resolution 1e-3].

    uint16_t* pdopPtr
        ///< [OUT] Position Dilution of Precision [resolution 1e-3].
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's altitude with respect to the WGS-84 ellipsoid
 *
 * @return
 *  - LE_FAULT         Function failed to get the altitude.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note altitudeOnWgs84 is in meters, between WGS-84 earth ellipsoid and mean sea level
 *       with 3 decimal places (3047 = 3.047 meters).
 *
 * @note For a 2D position fix, the altitude with respect to the WGS-84 ellipsoid will be indicated
 *       as invalid and set to INT32_MAX.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetAltitudeOnWgs84
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* altitudeOnWgs84Ptr
        ///< [OUT] Altitude in meters, between WGS-84 earth ellipsoid
        ///<       and mean sea level [resolution 1e-3].
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's magnetic deviation. It is the difference between the bearing to
 * true north and the bearing shown on a magnetic compass. The deviation is positive when the
 * magnetic north is east of true north.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note magneticDeviation is in degrees, with 1 decimal places (47 = 4.7 degree).
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetMagneticDeviation
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* magneticDeviationPtr
        ///< [OUT] MagneticDeviation in degrees [resolution 1e-1].
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the last updated position sample object reference.
 *
 * @return A reference to last Position's sample.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_gnss_SampleRef_t le_gnss_GetLastSampleRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release the position sample.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_gnss_ReleaseSampleRef
(
    le_gnss_SampleRef_t positionSampleRef
        ///< [IN] Position sample's reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the SUPL Assisted-GNSS mode.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED request not supported
 *  - LE_TIMEOUT a time-out occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetSuplAssistedMode
(
    le_gnss_AssistedMode_t assistedMode
        ///< [IN] Assisted-GNSS mode.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the SUPL Assisted-GNSS mode.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetSuplAssistedMode
(
    le_gnss_AssistedMode_t* assistedModePtr
        ///< [OUT] Assisted-GNSS mode.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the SUPL server URL.
 * That server URL is a NULL-terminated string with a maximum string length (including NULL
 * terminator) equal to 256. Optionally the port number is specified after a colon.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_BUSY service is busy
 *  - LE_TIMEOUT a time-out occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetSuplServerUrl
(
    const char* suplServerUrl
        ///< [IN] SUPL server URL.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function injects the SUPL certificate to be used in A-GNSS sessions. Certificates must
 * be encoded in DER. Other certificate encryptions (e.g., PEM, CER and CRT)
 * aren't supported.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_BUSY service is busy
 *  - LE_TIMEOUT a time-out occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_InjectSuplCertificate
(
    uint8_t suplCertificateId,
        ///< [IN] ID of the SUPL certificate.
        ///<      Certificate ID range is 0 to 9

    uint16_t suplCertificateLen,
        ///< [IN] SUPL certificate size in Bytes.

    const char* suplCertificate
        ///< [IN] SUPL certificate contents.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes the SUPL certificate.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_BUSY service is busy
 *  - LE_TIMEOUT a time-out occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_DeleteSuplCertificate
(
    uint8_t suplCertificateId
        ///< [IN] ID of the SUPL certificate.
        ///<      Certificate ID range is 0 to 9
);

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the enabled NMEA sentences using a bit mask.
 *
 * @return
 *  - LE_OK             Success
 *  - LE_BAD_PARAMETER  Bit mask exceeds the maximal value
 *  - LE_FAULT          Failure
 *  - LE_BUSY           Service is busy
 *  - LE_TIMEOUT        Timeout occurred
 *  - LE_NOT_PERMITTED  GNSS device is not in "ready" state
 *
 * @warning This function may be subject to limitations depending on the platform. Please refer to
 *          the @ref platformConstraintsGnss page.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetNmeaSentences
(
    le_gnss_NmeaBitMask_t nmeaMask
        ///< [IN] Bit mask for enabled NMEA sentences.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the bit mask for the enabled NMEA sentences.
 *
 * @return
 *  - LE_OK             Success
 *  - LE_FAULT          Failure
 *  - LE_BUSY           Service is busy
 *  - LE_TIMEOUT        Timeout occurred
 *  - LE_NOT_PERMITTED  GNSS device is not in "ready" state
 *
 * @note If the caller is passing an null pointer to this function, it is a fatal error
 *       and the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetNmeaSentences
(
    le_gnss_NmeaBitMask_t* nmeaMaskPtrPtr
        ///< [OUT] Bit mask for enabled NMEA sentences.
);


#endif // LE_GNSS_INTERFACE_H_INCLUDE_GUARD

