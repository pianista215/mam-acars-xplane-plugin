#ifndef MAM_PROTOCOL_H
#define MAM_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Shared memory configuration
#define MAM_SHARED_MEMORY_NAME  "MAM_XPLANE_BRIDGE_SHM"
#define MAM_MUTEX_NAME          "MAM_XPLANE_BRIDGE_MUTEX"
#define MAM_MAX_DATAREFS        256
#define MAM_PROTOCOL_VERSION    1
#define MAM_DATAREF_PATH_MAX    256
#define MAM_STRING_VALUE_MAX    256

// Result codes
typedef enum {
    MAM_OK = 0,
    MAM_ERROR_NOT_CONNECTED,
    MAM_ERROR_INVALID_ID,
    MAM_ERROR_DATAREF_NOT_FOUND,
    MAM_ERROR_MAX_DATAREFS,
    MAM_ERROR_SHARED_MEMORY,
    MAM_ERROR_MUTEX,
    MAM_ERROR_PLUGIN_NOT_ACTIVE,
    MAM_ERROR_TYPE_MISMATCH
} MamResult;

// Data types for datarefs
typedef enum {
    MAM_TYPE_UNKNOWN = 0,
    MAM_TYPE_INT,
    MAM_TYPE_FLOAT,
    MAM_TYPE_DOUBLE
} MamDataType;

// Command types from client to plugin
typedef enum {
    MAM_CMD_NONE = 0,
    MAM_CMD_REGISTER_DATAREF,
    MAM_CMD_UNREGISTER_DATAREF,
    MAM_CMD_CHECK_DATAREF,
    MAM_CMD_READ_STRING
} MamCommandType;

// Single dataref entry
typedef struct {
    uint32_t id;                        // Unique ID (1-based, 0 = unused)
    uint32_t active;                    // 1 if registered, 0 if slot is free
    char path[MAM_DATAREF_PATH_MAX];    // Dataref path string
    MamDataType type;                   // Data type
    union {
        int32_t i;
        float f;
        double d;
    } value;
} MamDataRefEntry;

// Command structure for client requests
typedef struct {
    MamCommandType type;
    uint32_t requestId;                 // Client-provided request ID
    char path[MAM_DATAREF_PATH_MAX];    // Dataref path for register command
    uint32_t resultId;                  // Result: assigned dataref ID
    MamResult resultCode;               // Result: success or error code
    uint32_t processed;                 // 1 when plugin has processed command
    char resultString[MAM_STRING_VALUE_MAX]; // Result string for READ_STRING
} MamCommandEntry;

// Main shared memory structure
typedef struct {
    uint32_t protocolVersion;           // Protocol version for compatibility
    uint32_t pluginActive;              // 1 when plugin is running
    uint32_t updateCounter;             // Incremented on each update cycle
    uint32_t registeredCount;           // Number of active datarefs
    MamCommandEntry command;            // Single command slot (simple protocol)
    MamDataRefEntry datarefs[MAM_MAX_DATAREFS];
} MamSharedMemory;

#ifdef __cplusplus
}
#endif

#endif // MAM_PROTOCOL_H
