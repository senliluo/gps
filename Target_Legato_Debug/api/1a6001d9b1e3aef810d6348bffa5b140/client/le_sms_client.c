/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#include "le_sms_messages.h"
#include "le_sms_interface.h"


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
                "le_sms_ConnectService() not called for current thread");

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
    _ClientDataPool = le_mem_CreatePool("le_sms_ClientData", sizeof(_ClientData_t));

    // Allocate the client thread pool
    _ClientThreadDataPool = le_mem_CreatePool("le_sms_ClientThreadData", sizeof(_ClientThreadData_t));

    // Create the thread-local data key to be used to store a pointer to each thread object.
    LE_ASSERT(pthread_key_create(&_ThreadDataKey, NULL) == 0);

    // Create safe reference map for handler references.
    // The size of the map should be based on the number of handlers defined multiplied by
    // the number of client threads.  Since this number can't be completely determined at
    // build time, just make a reasonable guess.
    _HandlerRefMap = le_ref_CreateMap("le_sms_ClientHandlers", 5);
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
void le_sms_ConnectService
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
le_result_t le_sms_TryConnectService
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
void le_sms_DisconnectService
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
static void _Handle_le_sms_AddRxMessageHandler
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
    le_sms_RxMessageHandlerFunc_t _handlerRef_le_sms_AddRxMessageHandler = (le_sms_RxMessageHandlerFunc_t)_clientDataPtr->handlerPtr;
    void* contextPtr = _clientDataPtr->contextPtr;

    // Unpack the remaining parameters
    le_sms_MsgRef_t msgRef;
    _msgBufPtr = UnpackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );


    // Call the registered handler
    if ( _handlerRef_le_sms_AddRxMessageHandler != NULL )
    {
        _handlerRef_le_sms_AddRxMessageHandler( msgRef, contextPtr );
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
 * Add handler function for EVENT 'le_sms_RxMessage'
 *
 * This event provides information on new received messages.
 */
//--------------------------------------------------------------------------------------------------
le_sms_RxMessageHandlerRef_t le_sms_AddRxMessageHandler
(
    le_sms_RxMessageHandlerFunc_t handlerPtr,
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

    le_sms_RxMessageHandlerRef_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_AddRxMessageHandler;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
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
 * Remove handler function for EVENT 'le_sms_RxMessage'
 */
//--------------------------------------------------------------------------------------------------
void le_sms_RemoveRxMessageHandler
(
    le_sms_RxMessageHandlerRef_t addHandlerRef
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
    _msgPtr->id = _MSGID_le_sms_RemoveRxMessageHandler;
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
    addHandlerRef = (le_sms_RxMessageHandlerRef_t)clientDataPtr->handlerRef;
    le_mem_Release(clientDataPtr);
    _msgBufPtr = PackData( _msgBufPtr, &addHandlerRef, sizeof(le_sms_RxMessageHandlerRef_t) );

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


// This function parses the message buffer received from the server, and then calls the user
// registered handler, which is stored in a client data object.
static void _Handle_le_sms_AddFullStorageEventHandler
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
    le_sms_FullStorageHandlerFunc_t _handlerRef_le_sms_AddFullStorageEventHandler = (le_sms_FullStorageHandlerFunc_t)_clientDataPtr->handlerPtr;
    void* contextPtr = _clientDataPtr->contextPtr;

    // Unpack the remaining parameters
    le_sms_Storage_t storage;
    _msgBufPtr = UnpackData( _msgBufPtr, &storage, sizeof(le_sms_Storage_t) );


    // Call the registered handler
    if ( _handlerRef_le_sms_AddFullStorageEventHandler != NULL )
    {
        _handlerRef_le_sms_AddFullStorageEventHandler( storage, contextPtr );
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
 * Add handler function for EVENT 'le_sms_FullStorageEvent'
 *
 * This event provides information on full storage indication.
 */
//--------------------------------------------------------------------------------------------------
le_sms_FullStorageEventHandlerRef_t le_sms_AddFullStorageEventHandler
(
    le_sms_FullStorageHandlerFunc_t handlerPtr,
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

    le_sms_FullStorageEventHandlerRef_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_AddFullStorageEventHandler;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
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
 * Remove handler function for EVENT 'le_sms_FullStorageEvent'
 */
//--------------------------------------------------------------------------------------------------
void le_sms_RemoveFullStorageEventHandler
(
    le_sms_FullStorageEventHandlerRef_t addHandlerRef
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
    _msgPtr->id = _MSGID_le_sms_RemoveFullStorageEventHandler;
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
    addHandlerRef = (le_sms_FullStorageEventHandlerRef_t)clientDataPtr->handlerRef;
    le_mem_Release(clientDataPtr);
    _msgBufPtr = PackData( _msgBufPtr, &addHandlerRef, sizeof(le_sms_FullStorageEventHandlerRef_t) );

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
 * Create an SMS Message data structure.
 *
 * @return Reference to the new Message object.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_Create
(
    void
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_sms_MsgRef_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_Create;
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


//--------------------------------------------------------------------------------------------------
/**
 * Set the timeout to send a SMS Message.
 *
 * @return
 * - LE_FAULT         Message is not in UNSENT state or is Read-Only.
 * - LE_OK            Function succeeded.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetTimeout
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    uint32_t timeout
        ///< [IN] Timeout in seconds.
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
    _msgPtr->id = _MSGID_le_sms_SetTimeout;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );
    _msgBufPtr = PackData( _msgBufPtr, &timeout, sizeof(uint32_t) );

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
 * Set the Telephone destination number.
 *
 * Telephone number is defined in ITU-T recommendations E.164/E.163.
 * E.164 numbers can have a maximum of fifteen digits and are usually written with a '+' prefix.
 *
 * @return LE_NOT_PERMITTED Message is Read-Only.
 * @return LE_BAD_PARAMETER Telephone destination number length is equal to zero.
 * @return LE_OK            Function succeeded.
 *
 * @note If telephone destination number is too long is too long (max LE_MDMDEFS_PHONE_NUM_MAX_LEN
 *       digits), it is a fatal error, the function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetDestination
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    const char* dest
        ///< [IN] Telephone number string.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate
    if ( strlen(dest) > 17 ) LE_FATAL("strlen(dest) > 17");


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_SetDestination;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );
    _msgBufPtr = PackString( _msgBufPtr, dest );

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
 * This function must be called to set the Text Message content.
 *
 * @return LE_NOT_PERMITTED Message is Read-Only.
 * @return LE_BAD_PARAMETER Text message length is equal to zero.
 * @return LE_OK            Function succeeded.
 *
 * @note Text Message is encoded in ASCII format (ISO8859-15) and characters have to exist in
 *  the GSM 23.038 7 bit alphabet.
 *
 * @note If message is too long (max LE_SMS_TEXT_MAX_LEN digits), it is a fatal error, the
 *       function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetText
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    const char* text
        ///< [IN] SMS text.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate
    if ( strlen(text) > 160 ) LE_FATAL("strlen(text) > 160");


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_SetText;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );
    _msgBufPtr = PackString( _msgBufPtr, text );

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
 * Set the binary message content.
 *
 * @return LE_NOT_PERMITTED Message is Read-Only.
 * @return LE_BAD_PARAMETER Length of the data is equal to zero.
 * @return LE_OK            Function succeeded.
 *
 * @note If length of the data is too long (max LE_SMS_BINARY_MAX_BYTES bytes), it is a fatal
 *       error, the function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetBinary
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    const uint8_t* binPtr,
        ///< [IN] Binary data.

    size_t binNumElements
        ///< [IN]
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate
    if ( binNumElements > 140 ) LE_FATAL("binNumElements > 140");


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_SetBinary;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );
    _msgBufPtr = PackData( _msgBufPtr, &binNumElements, sizeof(size_t) );
    _msgBufPtr = PackData( _msgBufPtr, binPtr, binNumElements*sizeof(uint8_t) );

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
 * Set the UCS2 message content (16-bit format).
 *
 * @return
 *  - LE_NOT_PERMITTED Message is Read-Only.
 *  - LE_BAD_PARAMETER Length of the data is equal to zero.
 *  - LE_OK            Function succeeded.
 *
 * @note If length of the data is too long (max LE_SMS_UCS2_MAX_CHARS), it is a fatal
 *       error, the function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetUCS2
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    const uint16_t* ucs2Ptr,
        ///< [IN] UCS2 message.

    size_t ucs2NumElements
        ///< [IN]
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate
    if ( ucs2NumElements > 70 ) LE_FATAL("ucs2NumElements > 70");


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_SetUCS2;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );
    _msgBufPtr = PackData( _msgBufPtr, &ucs2NumElements, sizeof(size_t) );
    _msgBufPtr = PackData( _msgBufPtr, ucs2Ptr, ucs2NumElements*sizeof(uint16_t) );

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
 * Set the PDU message content.
 *
 * @return LE_NOT_PERMITTED Message is Read-Only.
 * @return LE_BAD_PARAMETER Length of the data is equal to zero.
 * @return LE_OK            Function succeeded.
 *
 * @note If length of the data is too long (max LE_SMS_PDU_MAX_BYTES bytes), it is a fatal error,
 *       the function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetPDU
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    const uint8_t* pduPtr,
        ///< [IN] PDU message.

    size_t pduNumElements
        ///< [IN]
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate
    if ( pduNumElements > 176 ) LE_FATAL("pduNumElements > 176");


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_SetPDU;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );
    _msgBufPtr = PackData( _msgBufPtr, &pduNumElements, sizeof(size_t) );
    _msgBufPtr = PackData( _msgBufPtr, pduPtr, pduNumElements*sizeof(uint8_t) );

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
 * Send an SMS message.
 *
 * Verifies first if the parameters are valid, then it checks the modem state can support
 * message sending.
 *
 * @return LE_FORMAT_ERROR  Message content is invalid.
 * @return LE_FAULT         Function failed to send the message.
 * @return LE_OK            Function succeeded.
 * @return LE_TIMEOUT       Timeout before the complete sending.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_Send
(
    le_sms_MsgRef_t msgRef
        ///< [IN] Reference to the message object.
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
    _msgPtr->id = _MSGID_le_sms_Send;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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


// This function parses the message buffer received from the server, and then calls the user
// registered handler, which is stored in a client data object.
static void _Handle_le_sms_SendAsync
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
    le_sms_CallbackResultFunc_t _handlerRef_le_sms_SendAsync = (le_sms_CallbackResultFunc_t)_clientDataPtr->handlerPtr;
    void* contextPtr = _clientDataPtr->contextPtr;

    // Unpack the remaining parameters
    le_sms_MsgRef_t msgRef;
    _msgBufPtr = UnpackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

    le_sms_Status_t status;
    _msgBufPtr = UnpackData( _msgBufPtr, &status, sizeof(le_sms_Status_t) );


    // Call the registered handler
    if ( _handlerRef_le_sms_SendAsync != NULL )
    {
        _handlerRef_le_sms_SendAsync( msgRef, status, contextPtr );
    }
    else
    {
        LE_FATAL("Error in client data: no registered handler");
    }

    // The registered handler has been called, so no longer need the client data.
    // Explicitly set handlerPtr to NULL, so that we can catch if this function gets
    // accidently called again.
    _clientDataPtr->handlerPtr = NULL;
    le_mem_Release(_clientDataPtr);

    // Release the message, now that we are finished with it.
    le_msg_ReleaseMsg(_msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Send an asynchronous SMS message.
 *
 * Verifies first if the parameters are valid, then it checks the modem state can support
 * message sending.
 *
 * @return LE_FORMAT_ERROR  Message content is invalid.
 * @return LE_FAULT         Function failed to send the message.
 * @return LE_OK            Function succeeded.
 * @return LE_TIMEOUT       Timeout before the complete sending.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SendAsync
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    le_sms_CallbackResultFunc_t handlerPtr,
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

    le_result_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_SendAsync;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );
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


    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the error code when a 3GPP2 message sending has Failed.
 *
 * @return The error code
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 *
 * @note It is only applicable for 3GPP2 message sending failure, otherwise
 *       LE_SMS_ERROR_3GPP2_MAX is returned.
 */
//--------------------------------------------------------------------------------------------------
le_sms_ErrorCode3GPP2_t le_sms_Get3GPP2ErrorCode
(
    le_sms_MsgRef_t msgRef
        ///< [IN] Reference to the message object.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_sms_ErrorCode3GPP2_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_Get3GPP2ErrorCode;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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
 * Get the Radio Protocol and the Transfer Protocol error code when a 3GPP message sending has
 * failed.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 *
 * @note It is only applicable for 3GPP message sending failure, otherwise
 *       LE_SMS_ERROR_3GPP_MAX is returned.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_GetErrorCode
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    le_sms_ErrorCode_t* rpCausePtr,
        ///< [OUT] Radio Protocol cause code.

    le_sms_ErrorCode_t* tpCausePtr
        ///< [OUT] Transfer Protocol cause code.
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
    _msgPtr->id = _MSGID_le_sms_GetErrorCode;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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
    _msgBufPtr = UnpackData( _msgBufPtr, rpCausePtr, sizeof(le_sms_ErrorCode_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, tpCausePtr, sizeof(le_sms_ErrorCode_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);

}


//--------------------------------------------------------------------------------------------------
/**
 * Called to get the platform specific error code.
 *
 * @return The platform specific error code.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_sms_GetPlatformSpecificErrorCode
(
    le_sms_MsgRef_t msgRef
        ///< [IN] Reference to the message object.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    int32_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_GetPlatformSpecificErrorCode;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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


// This function parses the message buffer received from the server, and then calls the user
// registered handler, which is stored in a client data object.
static void _Handle_le_sms_SendText
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
    le_sms_CallbackResultFunc_t _handlerRef_le_sms_SendText = (le_sms_CallbackResultFunc_t)_clientDataPtr->handlerPtr;
    void* contextPtr = _clientDataPtr->contextPtr;

    // Unpack the remaining parameters
    le_sms_MsgRef_t msgRef;
    _msgBufPtr = UnpackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

    le_sms_Status_t status;
    _msgBufPtr = UnpackData( _msgBufPtr, &status, sizeof(le_sms_Status_t) );


    // Call the registered handler
    if ( _handlerRef_le_sms_SendText != NULL )
    {
        _handlerRef_le_sms_SendText( msgRef, status, contextPtr );
    }
    else
    {
        LE_FATAL("Error in client data: no registered handler");
    }

    // The registered handler has been called, so no longer need the client data.
    // Explicitly set handlerPtr to NULL, so that we can catch if this function gets
    // accidently called again.
    _clientDataPtr->handlerPtr = NULL;
    le_mem_Release(_clientDataPtr);

    // Release the message, now that we are finished with it.
    le_msg_ReleaseMsg(_msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Create and asynchronously send a text message.
 *
 * @return
 *  - le_sms_Msg Reference to the new Message object pooled.
 *  - NULL Not possible to pool a new message.
 *
 * @note If telephone destination number is too long is too long (max LE_MDMDEFS_PHONE_NUM_MAX_LEN
 *       digits), it is a fatal error, the function will not return.
 * @note If message is too long (max LE_SMS_TEXT_MAX_LEN digits), it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_SendText
(
    const char* destStr,
        ///< [IN] Telephone number string.

    const char* textStr,
        ///< [IN] SMS text.

    le_sms_CallbackResultFunc_t handlerPtr,
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

    le_sms_MsgRef_t _result;

    // Range check values, if appropriate
    if ( strlen(destStr) > 17 ) LE_FATAL("strlen(destStr) > 17");
    if ( strlen(textStr) > 160 ) LE_FATAL("strlen(textStr) > 160");


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_SendText;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackString( _msgBufPtr, destStr );
    _msgBufPtr = PackString( _msgBufPtr, textStr );
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


    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


// This function parses the message buffer received from the server, and then calls the user
// registered handler, which is stored in a client data object.
static void _Handle_le_sms_SendPdu
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
    le_sms_CallbackResultFunc_t _handlerRef_le_sms_SendPdu = (le_sms_CallbackResultFunc_t)_clientDataPtr->handlerPtr;
    void* contextPtr = _clientDataPtr->contextPtr;

    // Unpack the remaining parameters
    le_sms_MsgRef_t msgRef;
    _msgBufPtr = UnpackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

    le_sms_Status_t status;
    _msgBufPtr = UnpackData( _msgBufPtr, &status, sizeof(le_sms_Status_t) );


    // Call the registered handler
    if ( _handlerRef_le_sms_SendPdu != NULL )
    {
        _handlerRef_le_sms_SendPdu( msgRef, status, contextPtr );
    }
    else
    {
        LE_FATAL("Error in client data: no registered handler");
    }

    // The registered handler has been called, so no longer need the client data.
    // Explicitly set handlerPtr to NULL, so that we can catch if this function gets
    // accidently called again.
    _clientDataPtr->handlerPtr = NULL;
    le_mem_Release(_clientDataPtr);

    // Release the message, now that we are finished with it.
    le_msg_ReleaseMsg(_msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Create and asynchronously send a PDU message.
 *
 * @return
 *  - le_sms_Msg Reference to the new Message object pooled.
 *  - NULL Not possible to pool a new message.
 *
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_SendPdu
(
    const uint8_t* pduPtr,
        ///< [IN] PDU message.

    size_t pduNumElements,
        ///< [IN]

    le_sms_CallbackResultFunc_t handlerPtr,
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

    le_sms_MsgRef_t _result;

    // Range check values, if appropriate
    if ( pduNumElements > 176 ) LE_FATAL("pduNumElements > 176");


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_SendPdu;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &pduNumElements, sizeof(size_t) );
    _msgBufPtr = PackData( _msgBufPtr, pduPtr, pduNumElements*sizeof(uint8_t) );
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


    // Unpack any "out" parameters


    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a Message data structure.
 *
 * It deletes the Message data structure and all the allocated memory is freed. If several
 * users own the Message object (e.g., several handler functions registered for
 * SMS message reception), the Message object will only be deleted if one User
 * owns the Message object.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_Delete
(
    le_sms_MsgRef_t msgRef
        ///< [IN] Reference to the message object.
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
    _msgPtr->id = _MSGID_le_sms_Delete;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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
 * Get the message format.
 *
 * @return Message format.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 *
 * @note For incoming SMS, format returned by le_sms_GetFormat is never LE_SMS_FORMAT_PDU.
 */
//--------------------------------------------------------------------------------------------------
le_sms_Format_t le_sms_GetFormat
(
    le_sms_MsgRef_t msgRef
        ///< [IN] Reference to the message object.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_sms_Format_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_GetFormat;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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
 * Get the message type.
 *
 * @return Message type.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_Type_t le_sms_GetType
(
    le_sms_MsgRef_t msgRef
        ///< [IN] Reference to the message object.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_sms_Type_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_GetType;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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
 * Get the Cell Broadcast Message Identifier.
 *
 * @return
 * - LE_FAULT       Message is not a cell broadcast type.
 * - LE_OK          Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetCellBroadcastId
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    uint16_t* messageIdPtr
        ///< [OUT] Cell Broadcast Message Identifier.
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
    _msgPtr->id = _MSGID_le_sms_GetCellBroadcastId;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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
    _msgBufPtr = UnpackData( _msgBufPtr, messageIdPtr, sizeof(uint16_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Cell Broadcast Message Serial Number.
 *
 * @return
 * - LE_FAULT       Message is not a cell broadcast type.
 * - LE_OK          Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetCellBroadcastSerialNumber
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    uint16_t* serialNumberPtr
        ///< [OUT] Cell Broadcast Serial Number.
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
    _msgPtr->id = _MSGID_le_sms_GetCellBroadcastSerialNumber;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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
    _msgBufPtr = UnpackData( _msgBufPtr, serialNumberPtr, sizeof(uint16_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Sender Telephone number.
 *
 * Output parameter is updated with the Telephone number. If the Telephone number string exceeds
 * the value of 'len' parameter,  LE_OVERFLOW error code is returned and 'tel' is filled until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'tel'.
 *
 * @return LE_NOT_PERMITTED Message is not a received message.
 * @return LE_OVERFLOW      Telephone number length exceed the maximum length.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetSenderTel
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    char* tel,
        ///< [OUT] Telephone number string.

    size_t telNumElements
        ///< [IN]
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
    _msgPtr->id = _MSGID_le_sms_GetSenderTel;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );
    _msgBufPtr = PackData( _msgBufPtr, &telNumElements, sizeof(size_t) );

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
    _msgBufPtr = UnpackString( _msgBufPtr, tel, telNumElements*sizeof(char) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Service Center Time Stamp string.
 *
 * Output parameter is updated with the Time Stamp string. If the Time Stamp string exceeds the
 * value of 'len' parameter, a LE_OVERFLOW error code is returned and 'timestamp' is filled until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'timestamp'.
 *
 * @return LE_NOT_PERMITTED Message is not a received message.
 * @return LE_OVERFLOW      Timestamp number length exceed the maximum length.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTimeStamp
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    char* timestamp,
        ///< [OUT] Message time stamp (in text mode).
        ///<            string format: "yy/MM/dd,hh:mm:ss+/-zz"
        ///<            (Year/Month/Day,Hour:Min:Seconds+/-TimeZone)

    size_t timestampNumElements
        ///< [IN]
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
    _msgPtr->id = _MSGID_le_sms_GetTimeStamp;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );
    _msgBufPtr = PackData( _msgBufPtr, &timestampNumElements, sizeof(size_t) );

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
    _msgBufPtr = UnpackString( _msgBufPtr, timestamp, timestampNumElements*sizeof(char) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the message Length value.
 *
 * @return Number of characters for text and UCS2 messages, or the length of the data in bytes for
 *  raw binary messages.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
size_t le_sms_GetUserdataLen
(
    le_sms_MsgRef_t msgRef
        ///< [IN] Reference to the message object.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    size_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_GetUserdataLen;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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
 * Get the text Message.
 *
 * Output parameter is updated with the text string encoded in ASCII format. If the text string
 * exceeds the value of 'len' parameter, LE_OVERFLOW error code is returned and 'text' is filled
 * until 'len-1' characters and a null-character is implicitly appended at the end of 'text'.
 *
 * @return LE_OVERFLOW      Message length exceed the maximum length.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetText
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    char* text,
        ///< [OUT] SMS text.

    size_t textNumElements
        ///< [IN]
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
    _msgPtr->id = _MSGID_le_sms_GetText;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );
    _msgBufPtr = PackData( _msgBufPtr, &textNumElements, sizeof(size_t) );

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
    _msgBufPtr = UnpackString( _msgBufPtr, text, textNumElements*sizeof(char) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the binary Message.
 *
 * Output parameters are updated with the binary message content and the length of the raw
 * binary message in bytes. If the binary data exceed the value of 'len' input parameter, a
 * LE_OVERFLOW error code is returned and 'raw' is filled until 'len' bytes.
 *
 * @return LE_FORMAT_ERROR  Message is not in binary format
 * @return LE_OVERFLOW      Message length exceed the maximum length.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetBinary
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    uint8_t* binPtr,
        ///< [OUT] Binary message.

    size_t* binNumElementsPtr
        ///< [INOUT]
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
    _msgPtr->id = _MSGID_le_sms_GetBinary;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );
    _msgBufPtr = PackData( _msgBufPtr, binNumElementsPtr, sizeof(size_t) );

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
    _msgBufPtr = UnpackData( _msgBufPtr, binNumElementsPtr, sizeof(size_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, binPtr, *binNumElementsPtr*sizeof(uint8_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the UCS2 Message (16-bit format).
 *
 * Output parameters are updated with the UCS2 message content and the number of characters. If
 * the UCS2 data exceed the value of the length input parameter, a LE_OVERFLOW error
 * code is returned and 'ucs2Ptr' is filled until of the number of chars specified.
 *
 * @return
 *  - LE_FORMAT_ERROR  Message is not in binary format
 *  - LE_OVERFLOW      Message length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetUCS2
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    uint16_t* ucs2Ptr,
        ///< [OUT] UCS2 message.

    size_t* ucs2NumElementsPtr
        ///< [INOUT]
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
    _msgPtr->id = _MSGID_le_sms_GetUCS2;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );
    _msgBufPtr = PackData( _msgBufPtr, ucs2NumElementsPtr, sizeof(size_t) );

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
    _msgBufPtr = UnpackData( _msgBufPtr, ucs2NumElementsPtr, sizeof(size_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, ucs2Ptr, *ucs2NumElementsPtr*sizeof(uint16_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the PDU message.
 *
 * Output parameters are updated with the PDU message content and the length of the PDU message
 * in bytes. If the PDU data exceed the value of 'len' input parameter, a LE_OVERFLOW error code is
 * returned and 'pdu' is filled until 'len' bytes.
 *
 * @return LE_FORMAT_ERROR  Unable to encode the message in PDU (only for outgoing messages).
 * @return LE_OVERFLOW      Message length exceed the maximum length.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetPDU
(
    le_sms_MsgRef_t msgRef,
        ///< [IN] Reference to the message object.

    uint8_t* pduPtr,
        ///< [OUT] PDU message.

    size_t* pduNumElementsPtr
        ///< [INOUT]
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
    _msgPtr->id = _MSGID_le_sms_GetPDU;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );
    _msgBufPtr = PackData( _msgBufPtr, pduNumElementsPtr, sizeof(size_t) );

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
    _msgBufPtr = UnpackData( _msgBufPtr, pduNumElementsPtr, sizeof(size_t) );
    _msgBufPtr = UnpackData( _msgBufPtr, pduPtr, *pduNumElementsPtr*sizeof(uint8_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the message Length value.
 *
 * @return Length of the data in bytes of the PDU message.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
size_t le_sms_GetPDULen
(
    le_sms_MsgRef_t msgRef
        ///< [IN] Reference to the message object.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    size_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_GetPDULen;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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
 * Delete an SMS message from the storage area.
 *
 * Verifies first if the parameter is valid, then it checks the modem state can support
 * message deleting.
 *
 * @return LE_FAULT         Function failed to perform the deletion.
 * @return LE_NO_MEMORY     Message storage is not available.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_DeleteFromStorage
(
    le_sms_MsgRef_t msgRef
        ///< [IN] Reference to the message object.
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
    _msgPtr->id = _MSGID_le_sms_DeleteFromStorage;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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
 * Create an object's reference of the list of received messages
 * saved in the SMS message storage area.
 *
 * @return
 *      Reference to the List object. Null pointer if no messages have been retrieved.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgListRef_t le_sms_CreateRxMsgList
(
    void
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_sms_MsgListRef_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_CreateRxMsgList;
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


//--------------------------------------------------------------------------------------------------
/**
 * Delete the list of the Messages retrieved from the message
 * storage.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_DeleteList
(
    le_sms_MsgListRef_t msgListRef
        ///< [IN] Messages list.
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
    _msgPtr->id = _MSGID_le_sms_DeleteList;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgListRef, sizeof(le_sms_MsgListRef_t) );

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
 * Get the first Message object reference in the list of messages
 * retrieved with le_sms_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return Msg  Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_GetFirst
(
    le_sms_MsgListRef_t msgListRef
        ///< [IN] Messages list.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_sms_MsgRef_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_GetFirst;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgListRef, sizeof(le_sms_MsgListRef_t) );

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
 * Get the next Message object reference in the list of messages
 * retrieved with le_sms_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return Msg  Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_GetNext
(
    le_sms_MsgListRef_t msgListRef
        ///< [IN] Messages list.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_sms_MsgRef_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_GetNext;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgListRef, sizeof(le_sms_MsgListRef_t) );

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
 * Read the Message status (Received Read, Received Unread, Stored
 * Sent, Stored Unsent).
 *
 * @return Status of the message.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_Status_t le_sms_GetStatus
(
    le_sms_MsgRef_t msgRef
        ///< [IN] Messages list.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_sms_Status_t _result;

    // Range check values, if appropriate


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_GetStatus;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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
 * Mark a message as 'read'.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_MarkRead
(
    le_sms_MsgRef_t msgRef
        ///< [IN] Messages list.
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
    _msgPtr->id = _MSGID_le_sms_MarkRead;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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
 * Mark a message as 'unread'.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_MarkUnread
(
    le_sms_MsgRef_t msgRef
        ///< [IN] Messages list.
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
    _msgPtr->id = _MSGID_le_sms_MarkUnread;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &msgRef, sizeof(le_sms_MsgRef_t) );

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
 * Get the SMS center address.
 *
 * Output parameter is updated with the SMS Service center address. If the Telephone number string exceeds
 * the value of 'len' parameter,  LE_OVERFLOW error code is returned and 'tel' is filled until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'tel'.
 *
 * @return
 *  - LE_FAULT         Service is not available.
 *  - LE_OVERFLOW      Telephone number length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetSmsCenterAddress
(
    char* tel,
        ///< [OUT] SMS center address number string.

    size_t telNumElements
        ///< [IN]
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
    _msgPtr->id = _MSGID_le_sms_GetSmsCenterAddress;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &telNumElements, sizeof(size_t) );

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
    _msgBufPtr = UnpackString( _msgBufPtr, tel, telNumElements*sizeof(char) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the SMS center address.
 *
 * SMS center address number is defined in ITU-T recommendations E.164/E.163.
 * E.164 numbers can have a maximum of fifteen digits and are usually written with a '+' prefix.
 *
 * @return
 *  - LE_FAULT         Service is not available..
 *  - LE_OK            Function succeeded.
 *
 * @note If the SMS center address number is too long (max LE_MDMDEFS_PHONE_NUM_MAX_LEN digits), it
 *       is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetSmsCenterAddress
(
    const char* tel
        ///< [IN] SMS center address number string.
)
{
    le_msg_MessageRef_t _msgRef;
    le_msg_MessageRef_t _responseMsgRef;
    _Message_t* _msgPtr;

    // Will not be used if no data is sent/received from server.
    __attribute__((unused)) uint8_t* _msgBufPtr;

    le_result_t _result;

    // Range check values, if appropriate
    if ( strlen(tel) > 17 ) LE_FATAL("strlen(tel) > 17");


    // Create a new message object and get the message buffer
    _msgRef = le_msg_CreateMsg(GetCurrentSessionRef());
    _msgPtr = le_msg_GetPayloadPtr(_msgRef);
    _msgPtr->id = _MSGID_le_sms_SetSmsCenterAddress;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackString( _msgBufPtr, tel );

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
 * Set the preferred SMS storage for incoming messages.
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetPreferredStorage
(
    le_sms_Storage_t prefStorage
        ///< [IN] storage parameter.
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
    _msgPtr->id = _MSGID_le_sms_SetPreferredStorage;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &prefStorage, sizeof(le_sms_Storage_t) );

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
 * Get the preferred SMS storage for incoming messages.
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetPreferredStorage
(
    le_sms_Storage_t* prefStoragePtr
        ///< [OUT] storage parameter.
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
    _msgPtr->id = _MSGID_le_sms_GetPreferredStorage;
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
    _msgBufPtr = UnpackData( _msgBufPtr, prefStoragePtr, sizeof(le_sms_Storage_t) );

    // Release the message object, now that all results/output has been copied.
    le_msg_ReleaseMsg(_responseMsgRef);


    return _result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Activate Cell Broadcast message notification
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_ActivateCellBroadcast
(
    void
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
    _msgPtr->id = _MSGID_le_sms_ActivateCellBroadcast;
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


//--------------------------------------------------------------------------------------------------
/**
 * Deactivate Cell Broadcast message notification
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_DeactivateCellBroadcast
(
    void
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
    _msgPtr->id = _MSGID_le_sms_DeactivateCellBroadcast;
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


//--------------------------------------------------------------------------------------------------
/**
 * Activate CDMA Cell Broadcast message notification
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_ActivateCdmaCellBroadcast
(
    void
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
    _msgPtr->id = _MSGID_le_sms_ActivateCdmaCellBroadcast;
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


//--------------------------------------------------------------------------------------------------
/**
 * Deactivate CDMA Cell Broadcast message notification
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_DeactivateCdmaCellBroadcast
(
    void
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
    _msgPtr->id = _MSGID_le_sms_DeactivateCdmaCellBroadcast;
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


//--------------------------------------------------------------------------------------------------
/**
 * Add Cell Broadcast message Identifiers range.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_AddCellBroadcastIds
(
    uint16_t fromId,
        ///< [IN] Starting point of the range of cell broadcast message identifier.

    uint16_t toId
        ///< [IN] Ending point of the range of cell broadcast message identifier.
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
    _msgPtr->id = _MSGID_le_sms_AddCellBroadcastIds;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &fromId, sizeof(uint16_t) );
    _msgBufPtr = PackData( _msgBufPtr, &toId, sizeof(uint16_t) );

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
 * Remove Cell Broadcast message Identifiers range.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_RemoveCellBroadcastIds
(
    uint16_t fromId,
        ///< [IN] Starting point of the range of cell broadcast message identifier.

    uint16_t toId
        ///< [IN] Ending point of the range of cell broadcast message identifier.
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
    _msgPtr->id = _MSGID_le_sms_RemoveCellBroadcastIds;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &fromId, sizeof(uint16_t) );
    _msgBufPtr = PackData( _msgBufPtr, &toId, sizeof(uint16_t) );

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
 * Clear Cell Broadcast message Identifiers.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_ClearCellBroadcastIds
(
    void
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
    _msgPtr->id = _MSGID_le_sms_ClearCellBroadcastIds;
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


//--------------------------------------------------------------------------------------------------
/**
 * Add CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_BAD_PARAMETER Parameter is invalid.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_AddCdmaCellBroadcastServices
(
    le_sms_CdmaServiceCat_t serviceCat,
        ///< [IN] Service category assignment. Reference to 3GPP2 C.R1001-D
        ///<      v1.0 Section 9.3.1 Standard Service Category Assignments.

    le_sms_Languages_t language
        ///< [IN] Language Indicator. Reference to
        ///<      3GPP2 C.R1001-D v1.0 Section 9.2.1 Language Indicator
        ///<      Value Assignments
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
    _msgPtr->id = _MSGID_le_sms_AddCdmaCellBroadcastServices;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &serviceCat, sizeof(le_sms_CdmaServiceCat_t) );
    _msgBufPtr = PackData( _msgBufPtr, &language, sizeof(le_sms_Languages_t) );

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
 * Remove CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_BAD_PARAMETER Parameter is invalid.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_RemoveCdmaCellBroadcastServices
(
    le_sms_CdmaServiceCat_t serviceCat,
        ///< [IN] Service category assignment. Reference to 3GPP2 C.R1001-D
        ///<      v1.0 Section 9.3.1 Standard Service Category Assignments.

    le_sms_Languages_t language
        ///< [IN] Language Indicator. Reference to
        ///<      3GPP2 C.R1001-D v1.0 Section 9.2.1 Language Indicator
        ///<      Value Assignments
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
    _msgPtr->id = _MSGID_le_sms_RemoveCdmaCellBroadcastServices;
    _msgBufPtr = _msgPtr->buffer;

    // Pack the input parameters
    _msgBufPtr = PackData( _msgBufPtr, &serviceCat, sizeof(le_sms_CdmaServiceCat_t) );
    _msgBufPtr = PackData( _msgBufPtr, &language, sizeof(le_sms_Languages_t) );

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
 * Clear CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_ClearCdmaCellBroadcastServices
(
    void
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
    _msgPtr->id = _MSGID_le_sms_ClearCdmaCellBroadcastServices;
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
        case _MSGID_le_sms_AddRxMessageHandler :
            le_event_QueueFunctionToThread(callersThreadRef, _Handle_le_sms_AddRxMessageHandler, msgRef, clientDataPtr);
            break;

        case _MSGID_le_sms_AddFullStorageEventHandler :
            le_event_QueueFunctionToThread(callersThreadRef, _Handle_le_sms_AddFullStorageEventHandler, msgRef, clientDataPtr);
            break;

        case _MSGID_le_sms_SendAsync :
            le_event_QueueFunctionToThread(callersThreadRef, _Handle_le_sms_SendAsync, msgRef, clientDataPtr);
            break;

        case _MSGID_le_sms_SendText :
            le_event_QueueFunctionToThread(callersThreadRef, _Handle_le_sms_SendText, msgRef, clientDataPtr);
            break;

        case _MSGID_le_sms_SendPdu :
            le_event_QueueFunctionToThread(callersThreadRef, _Handle_le_sms_SendPdu, msgRef, clientDataPtr);
            break;

        default: LE_ERROR("Unknowm msg id = %i for client thread = %p", msgPtr->id, callersThreadRef);
    }
}

