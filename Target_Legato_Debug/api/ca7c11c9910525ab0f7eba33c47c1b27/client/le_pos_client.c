/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#include "le_pos_messages.h"
#include "le_pos_interface.h"


//--------------------------------------------------------------------------------------------------
// Generic Pack/Unpack Functions
//--------------------------------------------------------------------------------------------------

// todo: These functions could be moved to a separate library, to reduce overall code size and RAM
//       usage because they are common to each client and server.  However, they would then likely
//       need to be more generic, and provide better parameter checking and return results.  With
//       the way they are now, they can be customized to the specific needs of the generated code,
//       so for now, they will be kept with the generated code.  This may need revisiting later.

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* PackData(void* msgBufPtr, const void* dataPtr, size_t dataSize)
{
    // todo: should check for buffer overflow, but not sure what to do if it happens
    //       i.e. is it a fatal error, or just return a result

    memcpy( msgBufPtr, dataPtr, dataSize );
    return ( msgBufPtr + dataSize );
}

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* UnpackData(void* msgBufPtr, void* dataPtr, size_t dataSize)
{
    memcpy( dataPtr, msgBufPtr, dataSize );
    return ( msgBufPtr + dataSize );
}

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* PackString(void* msgBufPtr, const char* dataStr)
{
    // todo: should check for buffer overflow, but not sure what to do if it happens
    //       i.e. is it a fatal error, or just return a result

    // Get the sizes
    uint32_t strSize = strlen(dataStr);
    const uint32_t sizeOfStrSize = sizeof(strSize);

    // Always pack the string size first, and then the string itself
    memcpy( msgBufPtr, &strSize, sizeOfStrSize );
    msgBufPtr += sizeOfStrSize;
    memcpy( msgBufPtr, dataStr, strSize );

    // Return pointer to next free byte; msgBufPtr was adjusted above for string size value.
    return ( msgBufPtr + strSize );
}

// Unused attribute is needed because this function may not always get used
__attribute__((unused)) static void* UnpackString(void* msgBufPtr, char* dataStr, size_t dataSize)
{
    // todo: should check for buffer overflow, but not sure what to do if it happens
    //       i.e. is it a fatal error, or just return a result

    uint32_t strSize;
    const uint32_t sizeOfStrSize = sizeof(strSize);

    // Get the string size first, and then the actual string
    memcpy( &strSize, msgBufPtr, sizeOfStrSize );
    msgBufPtr += sizeOfStrSize;

    // Copy the string, and make sure it is null-terminated
    memcpy( dataStr, msgBufPtr, strSize );
    dataStr[strSize] = 0;

    // Return pointer to next free byte; msgBufPtr was adjusted above for string size value.
    return ( msgBufPtr + strSize );
}


//--------------------------------------------------------------------------------------------------
// Generic Client Types, Variables and Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Client Data Objects
 *
 * This object is used for each registered handler.  This is needed since we are not using
 * events, but are instead queueing functions directly with the event loop.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_event_HandlerFunc_t handlerPtr;          ///< Registered handler function
    void*                  contextPtr;          ///< ContextPtr registered with handler
    le_event_HandlerRef_t  handlerRef;          ///< HandlerRef for the registered handler
    le_thread_Ref_t        callersThreadRef;    ///< Caller's thread.
}
_ClientData_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for client data objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t _ClientDataPool;


//--------------------------------------------------------------------------------------------------
/**
 * Client Thread Objects
 *
 * This object is used to contain thread specific data for each IPC client.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t sessionRef;     ///< Client Session Reference
    int                 clientCount;    ///< Number of clients sharing this thread
}
_ClientThreadData_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for client thread objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t _ClientThreadDataPool;


//--------------------------------------------------------------------------------------------------
/**
 * Key under which the pointer to the Thread Object (_ClientThreadData_t) will be kept in
 * thread-local storage.  This allows a thread to quickly get a pointer to its own Thread Object.
 */
//--------------------------------------------------------------------------------------------------
static pthread_key_t _ThreadDataKey;


//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for use with Add/Remove handler references
 *
 * @warning Use _Mutex, defined below, to protect accesses to this data.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t _HandlerRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Mutex and associated macros for use with the above HandlerRefMap.
 *
 * Unused attribute is needed because this variable may not always get used.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static pthread_mutex_t _Mutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.

/// Locks the mutex.
#define _LOCK    LE_ASSERT(pthread_mutex_lock(&_Mutex) == 0);

/// Unlocks the mutex.
#define _UNLOCK  LE_ASSERT(pthread_mutex_unlock(&_Mutex) == 0);


//--------------------------------------------------------------------------------------------------
/**
 * This global flag is shared by all client threads, and is used to indicate whether the common
 * data has been initialized.
 *
 * @warning Use InitMutex, defined below, to protect accesses to this data.
 */
//--------------------------------------------------------------------------------------------------
static bool CommonDataInitialized = false;


//--------------------------------------------------------------------------------------------------
/**
 * Mutex and associated macros for use with the above CommonDataInitialized.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t InitMutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.

/// Locks the mutex.
#define LOCK_INIT    LE_ASSERT(pthread_mutex_lock(&InitMutex) == 0);

/// Unlocks the mutex.
#define UNLOCK_INIT  LE_ASSERT(pthread_mutex_unlock(&InitMutex) == 0);


//--------------------------------------------------------------------------------------------------
/**
 * Forward declaration needed by InitClientForThread
 */
//--------------------------------------------------------------------------------------------------
static void ClientIndicationRecvHandler
(
    le_msg_MessageRef_t  msgRef,
    void*                contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize thread specific data, and connect to the service for the current thread.
 *
 * @return
 *  - LE_OK if the client connected successfully to the service.
 *  - LE_UNAVAILABLE if the server is not currently offering the service to which the client is bound.
 *  - LE_NOT_PERMITTED if the client interface is not bound to any service (doesn't have a binding).
 *  - LE_COMM_ERROR if the Service Directory cannot be reached.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t InitClientForThread
(
    bool isBlocking
)
{
    // Open a session.
    le_msg_ProtocolRef_t protocolRef;
    le_msg_SessionRef_t sessionRef;

    protocolRef = le_msg_GetProtocolRef(PROTOCOL_ID_STR, sizeof(_Message_t));
    sessionRef = le_msg_CreateSession(protocolRef, SERVICE_INSTANCE_NAME);
    le_msg_SetSessionRecvHandler(sessionRef, ClientIndicationRecvHandler, NULL);

    if ( isBlocking )
    {
        le_msg_OpenSessionSync(sessionRef);
    }
    else
    {
        le_result_t result;

        result = le_msg_TryOpenSessionSync(sessionRef);
        if ( result != LE_OK )
        {
            LE_DEBUG("Could not connect to '%s' service", SERVICE_INSTANCE_NAME);

            le_msg_DeleteSession(sessionRef);

            switch (result)
            {
                case LE_UNAVAILABLE:
                    LE_DEBUG("Service not offered");
                    break;

                case LE_NOT_PERMITTED:
                    LE_DEBUG("Missing binding");
                    break;

                case LE_COMM_ERROR:
                    LE_DEBUG("Can't reach ServiceDirectory");
                    break;

                default:
                    LE_CRIT("le_msg_TryOpenSessionSync() returned unexpected result code %d (%s)",
                            result,
                            LE_RESULT_TXT(result));
                    break;
            }

            return result;
        }
    }

    // Store the client sessionRef in thread-local storage, since each thread requires
    // its own sessionRef.
    _ClientThreadData_t* clientThreadPtr = le_mem_ForceAlloc(_ClientThreadDataPool);
    clientThreadPtr->sessionRef = sessionRef;
    if (pthread_setspecific(_ThreadDataKey, clientThreadPtr) != 0)
    {
        LE_FATAL("pthread_setspecific() failed!");
    }

    // This is the first client for the current thread
    clientThreadPtr->clientCount = 1;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a pointer to the client thread data for the current thread.
 *
 * If the current thread does not have client data, then NULL is returned
 */
//--------------------------------------------------------------------------------------------------
static _ClientThreadData_t* GetClientThreadDataPtr
(
    void
)
{
    return pthread_getspecific(_ThreadDataKey);
}


//--------------------------------------------------------------------------------------------------
/**
 * Return the sessionRef for the current thread.
 *
 * If the current thread does not have a session ref, then this is a fatal error.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static le_msg_SessionRef_t GetCurrentSessionRef
(
    void
)
{
    _ClientThreadData_t* clientThreadPtr = GetClientThreadDataPtr();

    // If the thread specific data is NULL, then the session ref has not been created.
    LE_FATAL_IF(clientThreadPtr==NULL,
                "le_pos_ConnectService() not called for current thread");

    return clientThreadPtr->sessionRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Init data that is common across all threads.
 */
//--------------------------------------------------------------------------------------------------
static void InitCommonData(void)
{
    // Allocate the client data pool
    _ClientDataPool = le_mem_CreatePool("le_pos_ClientData", sizeof(_ClientData_t));

    // Allocate the client thread pool
    _ClientThreadDataPool = le_mem_CreatePool("le_pos_ClientThreadData", sizeof(_ClientThreadData_t));

    // Create the thread-local data key to be used to store a pointer to each thread object.
    LE_ASSERT(pthread_key_create(&_ThreadDataKey, NULL) == 0);

    // Create safe reference map for handler references.
    // The size of the map should be based on the number of handlers defined multiplied by
    // the number of client threads.  Since this number can't be completely determined at
    // build time, just make a reasonable guess.
    _HandlerRefMap = le_ref_CreateMap("le_pos_ClientHandlers", 5);
}


//--------------------------------------------------------------------------------------------------
/**
 * Connect to the service, using either blocking or non-blocking calls.
 *
 * This function implements the details of the public ConnectService functions.
 *
 * @return
 *  - LE_OK if the client connected successfully to the service.
 *  - LE_UNAVAILABLE if the server is not currently offering the service to which the client is bound.
 *  - LE_NOT_PERMITTED if the client interface is not bound to any service (doesn't have a binding).
 *  - LE_COMM_ERROR if the Service Directory cannot be reached.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DoConnectService
(
    bool isBlocking
)
{
    // If this is the first time the function is called, init the client common data.
    LOCK_INIT
    if ( ! CommonDataInitialized )
    {
        InitCommonData();
        CommonDataInitialized = true;
    }
    UNLOCK_INIT

    _ClientThreadData_t* clientThreadPtr = GetClientThreadDataPtr();

    // If the thread specific data is NULL, then there is no current client session.
    if (clientThreadPtr == NULL)
    {
        le_result_t result;

        result = InitClientForThread(isBlocking);
        if ( result != LE_OK )
        {
            // Note that the blocking call will always return LE_OK
            return result;
        }

        LE_DEBUG("======= Starting client for '%s' service ========", SERVICE_INSTANCE_NAME);
    }
    else
    {
        // Keep track of the number of clients for the current thread.  There is only one
        // connection per thread, and it is shared by all clients.
        clientThreadPtr->clientCount++;
        LE_DEBUG("======= Starting another client for '%s' service ========", SERVICE_INSTANCE_NAME);
    }

    return LE_OK;
}


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
void le_pos_ConnectService
(
    void
)
{
    // Conect to the service; block until connected.
    DoConnectService(true);
}

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
le_result_t le_pos_TryConnectService
(
    void
)
{
    // Conect to the service; return with an error if not connected.
    return DoConnectService(false);
}

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
void le_pos_DisconnectService
(
    void
)
{
    _ClientThreadData_t* clientThreadPtr = GetClientThreadDataPtr();

    // If the thread specific data is NULL, then there is no current client session.
    if (clientThreadPtr == NULL)
    {
        LE_CRIT("Trying to stop non-existent client session for '%s' service",
                SERVICE_INSTANCE_NAME);
    }
    else
    {
        // This is the last client for this thread, so close the session.
        if ( clientThreadPtr->clientCount == 1 )
        {
            le_msg_DeleteSession( clientThreadPtr->sessionRef );

            // Need to delete the thread specific data, since it is no longer valid.  If a new
            // client session is started, new thread specific data will be allocated.
            le_mem_Release(clientThreadPtr);
            if (pthread_setspecific(_ThreadDataKey, NULL) != 0)
            {
                LE_FATAL("pthread_setspecific() failed!");
            }

            LE_DEBUG("======= Stopping client for '%s' service ========", SERVICE_INSTANCE_NAME);
        }
        else
        {
            // There is one less client sharing this thread's connection.
            clientThreadPtr->clientCount--;

            LE_DEBUG("======= Stopping another client for '%s' service ========",
                     SERVICE_INSTANCE_NAME);
        }
    }
}


//--------------------------------------------------------------------------------------------------
// Client Specific Client Code
//--------------------------------------------------------------------------------------------------


// This function parses the message buffer received from the server, and then calls the user
// registered handler, which is stored in a client data object.
static void _Handle_le_pos_AddMovementHandler
(
    void* _reportPtr,
    void* _dataPtr
)
{
    le_msg_MessageRef_t _msgRef = _reportPtr;
    _Message_t* _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    uint8_t* _msgBufPtr = _msgPtr->buffer;

    // The clientContextPtr always exists and is always first. It is a safe reference to the client
    // data object, but we already get the pointer to the client data object through the _dataPtr
    // parameter, so we don't need to do anything with clientContextPtr, other than unpacking it.
    void* _clientContextPtr;
    _msgBufPtr = UnpackData( _msgBufPtr, &_clientContextPtr, sizeof(void*) );

    // The client data pointer is passed in as a parameter, since the lookup in the safe ref map
    // and check for NULL has already been done when this function is queued.
    _ClientData_t* _clientDataPtr = _dataPtr;

    // Pull out additional data from the client data pointer
    le_pos_MovementHandlerFunc_t _handlerRef_le_pos_AddMovementHandler = (le_pos_MovementHandlerFunc_t)_clientDataPtr->handlerPtr;
    void* contextPtr = _clientDataPtr->contextPtr;

    // Unpack the remaining parameters
    le_pos_SampleRef_t positionSampleRef;
    _msgBufPtr = UnpackData( _msgBufPtr, &positionSampleRef, sizeof(le_pos_SampleRef_t) );


    // Call the registered handler
    if ( _handlerRef_le_pos_AddMovementHandler != NULL )
    {
        _handlerRef_le_pos_AddMovementHandler( positionSampleRef, contextPtr );
    }
    else
    {
        LE_FATAL("Error in client data: no registered handler");
    }


    // Release the message, now that we are finished with it.
    le_msg_ReleaseMsg(_msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_pos_Movement'
 *
 * This event provides information on movement changes.
 */
//--------------------------------------------------------------------------------------------------
le_pos_MovementHandlerRef_t le_pos_AddMovementHandler
(
    uint32_t horizontalMagnitude,
        ///< [IN] Horizontal magnitude in meters.
        ///<            0 means that I don't care about
        ///<           changes in the latitude and longitude.

    uint32_t verticalMagnitude,
        ///< [IN] Vertical magnitude in meters.
        ///<            0 means that I don't care about
        ///<            changes in the altitude.

    le_pos_MovementHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_pos_MovementHandlerRef_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_AddMovementHandler;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &horizontalMagnitude, sizeof(uint32_t) );
    _msgBufPtr = PackData( _msgBufPtr, &verticalMagnitude, sizeof(uint32_t) );
    // The handlerPtr and contextPtr input parameters are stored in the client data object, and it is
    // a safe reference to this object that is passed down as the context pointer.  The handlerPtr is
    // not passed down.
    // Create a new client data object and fill it in
    _ClientData_t* _clientDataPtr = le_mem_ForceAlloc(_ClientDataPool);
    _clientDataPtr->handlerPtr = (le_event_HandlerFunc_t)handlerPtr;
    _clientDataPtr->contextPtr = contextPtr;
    _clientDataPtr->callersThreadRef = le_thread_GetCurrent();
    // Create a safeRef to be passed down as the contextPtr
    _LOCK
    contextPtr = le_ref_CreateRef(_HandlerRefMap, _clientDataPtr);
    _UNLOCK
    _msgBufPtr = PackData( _msgBufPtr, &contextPtr, sizeof(void*) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );

    // Put the handler reference result into the client data object, and
    // then return a safe reference to the client data object as the reference;
    // this safe reference is contained in the contextPtr, which was assigned
    // when the client data object was created.
    _clientDataPtr->handlerRef = (le_event_HandlerRef_t)_result;
    _result = contextPtr;

    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_pos_Movement'
 */
//--------------------------------------------------------------------------------------------------
void le_pos_RemoveMovementHandler
(
    le_pos_MovementHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;



    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_RemoveMovementHandler;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    // The passed in handlerRef is a safe reference for the client data object.  Need to get the
    // real handlerRef from the client data object and then delete both the safe reference and
    // the object since they are no longer needed.
    _LOCK
    _ClientData_t* clientDataPtr = le_ref_Lookup(_HandlerRefMap, addHandlerRef);
    LE_FATAL_IF(clientDataPtr==NULL, "Invalid reference");
    le_ref_DeleteRef(_HandlerRefMap, addHandlerRef);
    _UNLOCK
    addHandlerRef = (le_pos_MovementHandlerRef_t)clientDataPtr->handlerRef;
    le_mem_Release(clientDataPtr);
    _msgBufPtr = PackData( _msgBufPtr, &addHandlerRef, sizeof(le_pos_MovementHandlerRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;


    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);

}


//--------------------------------------------------------------------------------------------------
/**
 * Get the 2D location's data (Latitude, Longitude, Horizontal
 * accuracy).
 *
 * @return LE_FAULT         Function failed to get the 2D location's data
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note latitudePtr, longitudePtr, hAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_Get2DLocation
(
    int32_t* latitudePtr,
        ///< [OUT] WGS84 Latitude in degrees, positive North [resolution 1e-6].

    int32_t* longitudePtr,
        ///< [OUT] WGS84 Longitude in degrees, positive East [resolution 1e-6].

    int32_t* hAccuracyPtr
        ///< [OUT] Horizontal position's accuracy in meters.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_Get2DLocation;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters


    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, latitudePtr, sizeof(int32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, longitudePtr, sizeof(int32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, hAccuracyPtr, sizeof(int32_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the 3D location's data (Latitude, Longitude, Altitude,
 * Horizontal accuracy, Vertical accuracy).
 *
 * @return LE_FAULT         Function failed to get the 3D location's data
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note latitudePtr, longitudePtr,hAccuracyPtr, altitudePtr, vAccuracyPtr can be set to NULL
 *       if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_Get3DLocation
(
    int32_t* latitudePtr,
        ///< [OUT] WGS84 Latitude in degrees, positive North [resolution 1e-6].

    int32_t* longitudePtr,
        ///< [OUT] WGS84 Longitude in degrees, positive East [resolution 1e-6].

    int32_t* hAccuracyPtr,
        ///< [OUT] Horizontal position's accuracy in meters.

    int32_t* altitudePtr,
        ///< [OUT] Altitude in meters, above Mean Sea Level.

    int32_t* vAccuracyPtr
        ///< [OUT] Vertical position's accuracy in meters.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_Get3DLocation;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters


    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, latitudePtr, sizeof(int32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, longitudePtr, sizeof(int32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, hAccuracyPtr, sizeof(int32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, altitudePtr, sizeof(int32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, vAccuracyPtr, sizeof(int32_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the time of the last updated location
 *
 * @return LE_FAULT         Function failed to get the time.
 * @return LE_OUT_OF_RANGE  The retrieved time is invalid (all fields are set to 0).
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetTime
(
    uint16_t* hoursPtr,
        ///< [OUT] UTC Hours into the day [range 0..23].

    uint16_t* minutesPtr,
        ///< [OUT] UTC Minutes into the hour [range 0..59].

    uint16_t* secondsPtr,
        ///< [OUT] UTC Seconds into the minute [range 0..59].

    uint16_t* millisecondsPtr
        ///< [OUT] UTC Milliseconds into the second [range 0..999].
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_GetTime;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters


    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, hoursPtr, sizeof(uint16_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, minutesPtr, sizeof(uint16_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, secondsPtr, sizeof(uint16_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, millisecondsPtr, sizeof(uint16_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the date of the last updated location
 *
 * @return LE_FAULT         Function failed to get the date.
 * @return LE_OUT_OF_RANGE  The retrieved date is invalid (all fields are set to 0).
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetDate
(
    uint16_t* yearPtr,
        ///< [OUT] UTC Year A.D. [e.g. 2014].

    uint16_t* monthPtr,
        ///< [OUT] UTC Month into the year [range 1...12].

    uint16_t* dayPtr
        ///< [OUT] UTC Days into the month [range 1...31].
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_GetDate;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters


    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, yearPtr, sizeof(uint16_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, monthPtr, sizeof(uint16_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, dayPtr, sizeof(uint16_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the motion's data (Horizontal Speed, Horizontal Speed's
 * accuracy, Vertical Speed, Vertical Speed's accuracy).
 *
 * @return LE_FAULT         Function failed to get the motion's data.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX,
 *                          UINT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note hSpeedPtr, hSpeedAccuracyPtr, vSpeedPtr, vSpeedAccuracyPtr can be set to NULL if not
 *       needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetMotion
(
    uint32_t* hSpeedPtr,
        ///< [OUT] Horizontal Speed in m/sec.

    uint32_t* hSpeedAccuracyPtr,
        ///< [OUT] Horizontal Speed's accuracy in m/sec.

    int32_t* vSpeedPtr,
        ///< [OUT] Vertical Speed in m/sec, positive up.

    int32_t* vSpeedAccuracyPtr
        ///< [OUT] Vertical Speed's accuracy in m/sec.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_GetMotion;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters


    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, hSpeedPtr, sizeof(uint32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, hSpeedAccuracyPtr, sizeof(uint32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, vSpeedPtr, sizeof(int32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, vSpeedAccuracyPtr, sizeof(int32_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the heading indication.
 *
 * @return LE_FAULT         Function failed to get the heading indication.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to UINT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note Heading is given in degrees.
 *       Heading ranges from 0 to 359 degrees, where 0 is True North.
 *
 * @note headingPtr, headingAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetHeading
(
    uint32_t* headingPtr,
        ///< [OUT] Heading in degrees.
        ///<       Range: 0 to 359, where 0 is True North.

    uint32_t* headingAccuracyPtr
        ///< [OUT] Heading's accuracy in degrees.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_GetHeading;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters


    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, headingPtr, sizeof(uint32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, headingAccuracyPtr, sizeof(uint32_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the direction indication. Direction of movement is the direction that the vehicle or person
 * is actually moving.
 *
 * @return LE_FAULT         Function failed to get the direction indication.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to UINT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note Direction is given in degrees.
 *       Direction ranges from 0 to 359 degrees, where 0 is True North.
 *
 * @note directionPtr, directionAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetDirection
(
    uint32_t* directionPtr,
        ///< [OUT] Direction indication in degrees.
        ///<       Range: 0 to 359, where 0 is True North.

    uint32_t* directionAccuracyPtr
        ///< [OUT] Direction's accuracy estimate in degrees.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_GetDirection;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters


    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, directionPtr, sizeof(uint32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, directionAccuracyPtr, sizeof(uint32_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position fix state
 *
 * @return LE_FAULT         Function failed to get the position fix state.
 * @return LE_OK            Function succeeded.
 *
 * @note In case the function fails to get the position fix state, a fatal error occurs,
 *       the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetFixState
(
    le_pos_FixState_t* statePtr
        ///< [OUT] Position fix state.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_GetFixState;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters


    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, statePtr, sizeof(le_pos_FixState_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's 2D location (latitude, longitude,
 * horizontal accuracy).
 *
 * @return LE_FAULT         Function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note latitudePtr, longitudePtr, horizontalAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_Get2DLocation
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* latitudePtr,
        ///< [OUT] WGS84 Latitude in degrees, positive North [resolution 1e-6].

    int32_t* longitudePtr,
        ///< [OUT] WGS84 Longitude in degrees, positive East [resolution 1e-6].

    int32_t* horizontalAccuracyPtr
        ///< [OUT] Horizontal position's accuracy in meters.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_sample_Get2DLocation;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &positionSampleRef, sizeof(le_pos_SampleRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, latitudePtr, sizeof(int32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, longitudePtr, sizeof(int32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, horizontalAccuracyPtr, sizeof(int32_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's time.
 *
 * @return LE_FAULT         Function failed to get the time.
 * @return LE_OUT_OF_RANGE  The retrieved time is invalid (all fields are set to 0).
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetTime
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint16_t* hoursPtr,
        ///< [OUT] UTC Hours into the day [range 0..23].

    uint16_t* minutesPtr,
        ///< [OUT] UTC Minutes into the hour [range 0..59].

    uint16_t* secondsPtr,
        ///< [OUT] UTC Seconds into the minute [range 0..59].

    uint16_t* millisecondsPtr
        ///< [OUT] UTC Milliseconds into the second [range 0..999].
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_sample_GetTime;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &positionSampleRef, sizeof(le_pos_SampleRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, hoursPtr, sizeof(uint16_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, minutesPtr, sizeof(uint16_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, secondsPtr, sizeof(uint16_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, millisecondsPtr, sizeof(uint16_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's date.
 *
 * @return LE_FAULT         Function failed to get the date.
 * @return LE_OUT_OF_RANGE  The retrieved date is invalid (all fields are set to 0).
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetDate
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint16_t* yearPtr,
        ///< [OUT] UTC Year A.D. [e.g. 2014].

    uint16_t* monthPtr,
        ///< [OUT] UTC Month into the year [range 1...12].

    uint16_t* dayPtr
        ///< [OUT] UTC Days into the month [range 1...31].
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_sample_GetDate;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &positionSampleRef, sizeof(le_pos_SampleRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, yearPtr, sizeof(uint16_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, monthPtr, sizeof(uint16_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, dayPtr, sizeof(uint16_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's altitude.
 *
 * @return LE_FAULT         Function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note altitudePtr, altitudeAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetAltitude
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* altitudePtr,
        ///< [OUT] Altitude in meters, above Mean Sea Level.

    int32_t* altitudeAccuracyPtr
        ///< [OUT] Vertical position's accuracy in meters.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_sample_GetAltitude;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &positionSampleRef, sizeof(le_pos_SampleRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, altitudePtr, sizeof(int32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, altitudeAccuracyPtr, sizeof(int32_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's horizontal speed.
 *
 * @return LE_FAULT         Function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX,
 *                          UINT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note hSpeedPtr, hSpeedAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetHorizontalSpeed
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* hSpeedPtr,
        ///< [OUT] The Horizontal Speed in m/sec.

    uint32_t* hSpeedAccuracyPtr
        ///< [OUT] The Horizontal Speed's accuracy in m/sec.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_sample_GetHorizontalSpeed;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &positionSampleRef, sizeof(le_pos_SampleRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, hSpeedPtr, sizeof(uint32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, hSpeedAccuracyPtr, sizeof(uint32_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's vertical speed.
 *
 * @return LE_FAULT         The function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX).
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note vSpeedPtr, vSpeedAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetVerticalSpeed
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* vspeedPtr,
        ///< [OUT] The Vertical Speed in m/sec, positive up.

    int32_t* vspeedAccuracyPtr
        ///< [OUT] The Vertical Speed's accuracy in m/sec.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_sample_GetVerticalSpeed;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &positionSampleRef, sizeof(le_pos_SampleRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, vspeedPtr, sizeof(int32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, vspeedAccuracyPtr, sizeof(int32_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's heading. Heading is the direction that the vehicle or person is facing.
 *
 * @return LE_FAULT         Function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to UINT32_MAX).
 * @return LE_OK            TFunction succeeded.
 *
 * @note Heading is given in degrees.
 *       Heading ranges from 0 to 359 degrees, where 0 is True North.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note headingPtr, headingAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetHeading
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* headingPtr,
        ///< [OUT] Heading in degrees.
        ///<       Range: 0 to 359, where 0 is True North.

    uint32_t* headingAccuracyPtr
        ///< [OUT] Heading's accuracy estimate in degrees.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_sample_GetHeading;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &positionSampleRef, sizeof(le_pos_SampleRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, headingPtr, sizeof(uint32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, headingAccuracyPtr, sizeof(uint32_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's direction. Direction of movement is the direction that the vehicle or
 * person is actually moving.
 *
 * @return LE_FAULT         Function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to UINT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note Direction is given in degrees.
 *       Direction ranges from 0 to 359 degrees, where 0 is True North.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note directionPtr, directionAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetDirection
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* directionPtr,
        ///< [OUT] Direction indication in degrees.
        ///<       Range: 0 to 359, where 0 is True North.

    uint32_t* directionAccuracyPtr
        ///< [OUT] Direction's accuracy estimate in degrees.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_sample_GetDirection;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &positionSampleRef, sizeof(le_pos_SampleRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, directionPtr, sizeof(uint32_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, directionAccuracyPtr, sizeof(uint32_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's fix state.
 *
 * @return LE_FAULT         Function failed to get the position sample's fix state.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetFixState
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    le_pos_FixState_t* statePtr
        ///< [OUT] Position fix state.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_sample_GetFixState;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &positionSampleRef, sizeof(le_pos_SampleRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters
    _msgBufPtr = UnpackData( _msgBufPtr, statePtr, sizeof(le_pos_FixState_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Release the position sample.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_pos_sample_Release
(
    le_pos_SampleRef_t positionSampleRef
        ///< [IN] Position sample's reference.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;



    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_sample_Release;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &positionSampleRef, sizeof(le_pos_SampleRef_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;


    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);

}


//--------------------------------------------------------------------------------------------------
/**
 * Set the acquisition rate.
 *
 * @return
 *    LE_OUT_OF_RANGE    Invalid acquisition rate.
 *    LE_OK              The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_SetAcquisitionRate
(
    uint32_t acquisitionRate
        ///< [IN] Acquisition rate in milliseconds.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_SetAcquisitionRate;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &acquisitionRate, sizeof(uint32_t) );

    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the acquisition rate.
 *
 * @return
 *    Acquisition rate in milliseconds.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_pos_GetAcquisitionRate
(
    void
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    uint32_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_pos_GetAcquisitionRate;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters


    // Send a request to the server and get the response.
    LE_DEBUG("Sending message to server and waiting for response : %ti bytes sent",
             _msgBufPtr-_msgPtr->buffer);
    _responseMsgRef = le_msg_RequestSyncResponse(_msgRef);
    // It is a serious error if we don't get a valid response from the server
    LE_FATAL_IF(_responseMsgRef == NULL, "Valid response was not received from server");

    // Process the result and/or output parameters, if there are any.
    _msgPtr = le_msg_GetPayloadPtr(_responseMsgRef);
    _msgBufPtr = _msgPtr->buffer;

    // Unpack the result first
    _msgBufPtr = UnpackData( _msgBufPtr, &_result, sizeof(_result) );


    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


static void ClientIndicationRecvHandler
(
    le_msg_MessageRef_t  msgRef,
    void*                contextPtr
)
{
    // Get the message payload
    _Message_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
    uint8_t* _msgBufPtr = msgPtr->buffer;

    // Have to partially unpack the received message in order to know which thread
    // the queued function should actually go to.
    void* clientContextPtr;
    _msgBufPtr = UnpackData( _msgBufPtr, &clientContextPtr, sizeof(void*) );

    // The clientContextPtr is a safe reference for the client data object.  If the client data
    // pointer is NULL, this means the handler was removed before the event was reported to the
    // client. This is valid, and the event will be dropped.
    _LOCK
    _ClientData_t* clientDataPtr = le_ref_Lookup(_HandlerRefMap, clientContextPtr);
    _UNLOCK
    if ( clientDataPtr == NULL )
    {
        LE_DEBUG("Ignore reported event after handler removed");
        return;
    }

    // Pull out the callers thread
    le_thread_Ref_t callersThreadRef = clientDataPtr->callersThreadRef;

    // Trigger the appropriate event
    switch (msgPtr->id)
    {
        case _MSGID_le_pos_AddMovementHandler :
            le_event_QueueFunctionToThread(callersThreadRef, _Handle_le_pos_AddMovementHandler, msgRef, clientDataPtr);
            break;

        default: LE_ERROR("Unknowm msg id = %i for client thread = %p", msgPtr->id, callersThreadRef);
    }
}

