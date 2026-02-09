#include "mam_client.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>

// Global state
static HANDLE g_hMapFile = NULL;
static HANDLE g_hMutex = NULL;
static MamSharedMemory* g_shm = nullptr;

// Internal helpers
static bool Lock(DWORD timeoutMs = 1000);
static void Unlock();
static const MamDataRefEntry* FindEntry(uint32_t id);

MAM_API MamResult MAM_Connect(void)
{
    if (g_shm != nullptr) {
        // Already connected
        return MAM_OK;
    }

    // Open mutex
    g_hMutex = OpenMutexA(SYNCHRONIZE, FALSE, MAM_MUTEX_NAME);
    if (g_hMutex == NULL) {
        return MAM_ERROR_SHARED_MEMORY;
    }

    // Open file mapping
    g_hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MAM_SHARED_MEMORY_NAME);
    if (g_hMapFile == NULL) {
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
        return MAM_ERROR_SHARED_MEMORY;
    }

    // Map view
    g_shm = (MamSharedMemory*)MapViewOfFile(
        g_hMapFile,
        FILE_MAP_ALL_ACCESS,
        0, 0,
        sizeof(MamSharedMemory)
    );

    if (g_shm == NULL) {
        CloseHandle(g_hMapFile);
        CloseHandle(g_hMutex);
        g_hMapFile = NULL;
        g_hMutex = NULL;
        return MAM_ERROR_SHARED_MEMORY;
    }

    // Verify protocol version
    if (g_shm->protocolVersion != MAM_PROTOCOL_VERSION) {
        MAM_Disconnect();
        return MAM_ERROR_SHARED_MEMORY;
    }

    return MAM_OK;
}

MAM_API void MAM_Disconnect(void)
{
    if (g_shm) {
        UnmapViewOfFile(g_shm);
        g_shm = nullptr;
    }

    if (g_hMapFile) {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
    }

    if (g_hMutex) {
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
    }
}

MAM_API int MAM_IsConnected(void)
{
    return (g_shm != nullptr) ? 1 : 0;
}

MAM_API int MAM_IsPluginActive(void)
{
    if (!g_shm) return 0;

    if (!Lock(100)) return 0;
    int active = g_shm->pluginActive;
    Unlock();

    return active;
}

MAM_API int MAM_RegisterDataRef(const char* path)
{
    if (!g_shm) return 0;
    if (!path || path[0] == '\0') return 0;

    if (!Lock(5000)) {  // 5 second timeout for registration
        return 0;
    }

    // Check if plugin is active
    if (!g_shm->pluginActive) {
        Unlock();
        return 0;
    }

    // Wait for any previous command to be processed
    int retries = 100;
    while (!g_shm->command.processed && retries > 0) {
        Unlock();
        Sleep(10);
        if (!Lock(1000)) return 0;
        retries--;
    }

    if (!g_shm->command.processed) {
        Unlock();
        return 0;
    }

    // Set up command
    static uint32_t s_requestId = 1;
    g_shm->command.type = MAM_CMD_REGISTER_DATAREF;
    g_shm->command.requestId = s_requestId++;
    strcpy_s(g_shm->command.path, MAM_DATAREF_PATH_MAX, path);
    g_shm->command.resultId = 0;
    g_shm->command.resultCode = MAM_OK;
    g_shm->command.processed = 0;

    Unlock();

    // Wait for plugin to process command
    retries = 500;  // 5 seconds max
    while (retries > 0) {
        Sleep(10);
        if (!Lock(1000)) return 0;

        if (g_shm->command.processed) {
            uint32_t resultId = g_shm->command.resultId;
            Unlock();
            return (int)resultId;
        }

        Unlock();
        retries--;
    }

    return 0;
}

MAM_API MamResult MAM_UnregisterDataRef(int id)
{
    if (!g_shm) return MAM_ERROR_NOT_CONNECTED;
    if (id <= 0) return MAM_ERROR_INVALID_ID;

    if (!Lock(5000)) {
        return MAM_ERROR_MUTEX;
    }

    // Check if plugin is active
    if (!g_shm->pluginActive) {
        Unlock();
        return MAM_ERROR_PLUGIN_NOT_ACTIVE;
    }

    // Wait for any previous command to be processed
    int retries = 100;
    while (!g_shm->command.processed && retries > 0) {
        Unlock();
        Sleep(10);
        if (!Lock(1000)) return MAM_ERROR_MUTEX;
        retries--;
    }

    if (!g_shm->command.processed) {
        Unlock();
        return MAM_ERROR_MUTEX;
    }

    // Set up command
    g_shm->command.type = MAM_CMD_UNREGISTER_DATAREF;
    g_shm->command.resultId = (uint32_t)id;
    g_shm->command.resultCode = MAM_OK;
    g_shm->command.processed = 0;

    Unlock();

    // Wait for plugin to process command
    retries = 500;
    while (retries > 0) {
        Sleep(10);
        if (!Lock(1000)) return MAM_ERROR_MUTEX;

        if (g_shm->command.processed) {
            MamResult result = g_shm->command.resultCode;
            Unlock();
            return result;
        }

        Unlock();
        retries--;
    }

    return MAM_ERROR_MUTEX;
}

MAM_API int MAM_DataRefExists(const char* path)
{
    if (!g_shm) return 0;
    if (!path || path[0] == '\0') return 0;

    if (!Lock(5000)) {
        return 0;
    }

    if (!g_shm->pluginActive) {
        Unlock();
        return 0;
    }

    // Wait for any previous command to be processed
    int retries = 100;
    while (!g_shm->command.processed && retries > 0) {
        Unlock();
        Sleep(10);
        if (!Lock(1000)) return 0;
        retries--;
    }

    if (!g_shm->command.processed) {
        Unlock();
        return 0;
    }

    // Set up command
    g_shm->command.type = MAM_CMD_CHECK_DATAREF;
    strcpy_s(g_shm->command.path, MAM_DATAREF_PATH_MAX, path);
    g_shm->command.resultCode = MAM_OK;
    g_shm->command.processed = 0;

    Unlock();

    // Wait for plugin to process command
    retries = 500;
    while (retries > 0) {
        Sleep(10);
        if (!Lock(1000)) return 0;

        if (g_shm->command.processed) {
            int exists = (g_shm->command.resultCode == MAM_OK) ? 1 : 0;
            Unlock();
            return exists;
        }

        Unlock();
        retries--;
    }

    return 0;
}

MAM_API MamResult MAM_GetString(const char* path, char* outBuffer, int bufferSize)
{
    if (!g_shm) return MAM_ERROR_NOT_CONNECTED;
    if (!path || path[0] == '\0' || !outBuffer || bufferSize <= 0) return MAM_ERROR_INVALID_ID;

    outBuffer[0] = '\0';

    if (!Lock(5000)) {
        return MAM_ERROR_MUTEX;
    }

    if (!g_shm->pluginActive) {
        Unlock();
        return MAM_ERROR_PLUGIN_NOT_ACTIVE;
    }

    // Wait for any previous command to be processed
    int retries = 100;
    while (!g_shm->command.processed && retries > 0) {
        Unlock();
        Sleep(10);
        if (!Lock(1000)) return MAM_ERROR_MUTEX;
        retries--;
    }

    if (!g_shm->command.processed) {
        Unlock();
        return MAM_ERROR_MUTEX;
    }

    // Set up command
    g_shm->command.type = MAM_CMD_READ_STRING;
    strcpy_s(g_shm->command.path, MAM_DATAREF_PATH_MAX, path);
    g_shm->command.resultString[0] = '\0';
    g_shm->command.resultCode = MAM_OK;
    g_shm->command.processed = 0;

    Unlock();

    // Wait for plugin to process command
    retries = 500;
    while (retries > 0) {
        Sleep(10);
        if (!Lock(1000)) return MAM_ERROR_MUTEX;

        if (g_shm->command.processed) {
            MamResult result = g_shm->command.resultCode;
            if (result == MAM_OK) {
                strcpy_s(outBuffer, bufferSize, g_shm->command.resultString);
            }
            Unlock();
            return result;
        }

        Unlock();
        retries--;
    }

    return MAM_ERROR_MUTEX;
}

MAM_API MamResult MAM_GetDouble(int id, double* outValue)
{
    if (!g_shm) return MAM_ERROR_NOT_CONNECTED;
    if (id <= 0 || !outValue) return MAM_ERROR_INVALID_ID;

    if (!Lock(100)) {
        return MAM_ERROR_MUTEX;
    }

    const MamDataRefEntry* entry = FindEntry((uint32_t)id);
    if (!entry) {
        Unlock();
        return MAM_ERROR_INVALID_ID;
    }

    // Convert based on type
    switch (entry->type) {
        case MAM_TYPE_DOUBLE:
            *outValue = entry->value.d;
            break;
        case MAM_TYPE_FLOAT:
            *outValue = (double)entry->value.f;
            break;
        case MAM_TYPE_INT:
            *outValue = (double)entry->value.i;
            break;
        default:
            Unlock();
            return MAM_ERROR_TYPE_MISMATCH;
    }

    Unlock();
    return MAM_OK;
}

MAM_API MamResult MAM_GetFloat(int id, float* outValue)
{
    if (!g_shm) return MAM_ERROR_NOT_CONNECTED;
    if (id <= 0 || !outValue) return MAM_ERROR_INVALID_ID;

    if (!Lock(100)) {
        return MAM_ERROR_MUTEX;
    }

    const MamDataRefEntry* entry = FindEntry((uint32_t)id);
    if (!entry) {
        Unlock();
        return MAM_ERROR_INVALID_ID;
    }

    // Convert based on type
    switch (entry->type) {
        case MAM_TYPE_FLOAT:
            *outValue = entry->value.f;
            break;
        case MAM_TYPE_DOUBLE:
            *outValue = (float)entry->value.d;
            break;
        case MAM_TYPE_INT:
            *outValue = (float)entry->value.i;
            break;
        default:
            Unlock();
            return MAM_ERROR_TYPE_MISMATCH;
    }

    Unlock();
    return MAM_OK;
}

MAM_API MamResult MAM_GetInt(int id, int* outValue)
{
    if (!g_shm) return MAM_ERROR_NOT_CONNECTED;
    if (id <= 0 || !outValue) return MAM_ERROR_INVALID_ID;

    if (!Lock(100)) {
        return MAM_ERROR_MUTEX;
    }

    const MamDataRefEntry* entry = FindEntry((uint32_t)id);
    if (!entry) {
        Unlock();
        return MAM_ERROR_INVALID_ID;
    }

    // Convert based on type
    switch (entry->type) {
        case MAM_TYPE_INT:
            *outValue = entry->value.i;
            break;
        case MAM_TYPE_FLOAT:
            *outValue = (int)entry->value.f;
            break;
        case MAM_TYPE_DOUBLE:
            *outValue = (int)entry->value.d;
            break;
        default:
            Unlock();
            return MAM_ERROR_TYPE_MISMATCH;
    }

    Unlock();
    return MAM_OK;
}

MAM_API uint32_t MAM_GetUpdateCounter(void)
{
    if (!g_shm) return 0;

    if (!Lock(100)) return 0;
    uint32_t counter = g_shm->updateCounter;
    Unlock();

    return counter;
}

MAM_API uint32_t MAM_GetRegisteredCount(void)
{
    if (!g_shm) return 0;

    if (!Lock(100)) return 0;
    uint32_t count = g_shm->registeredCount;
    Unlock();

    return count;
}

MAM_API const char* MAM_GetErrorString(MamResult result)
{
    switch (result) {
        case MAM_OK:                      return "Success";
        case MAM_ERROR_NOT_CONNECTED:     return "Not connected to plugin";
        case MAM_ERROR_INVALID_ID:        return "Invalid dataref ID";
        case MAM_ERROR_DATAREF_NOT_FOUND: return "Dataref not found";
        case MAM_ERROR_MAX_DATAREFS:      return "Maximum datarefs reached";
        case MAM_ERROR_SHARED_MEMORY:     return "Shared memory error";
        case MAM_ERROR_MUTEX:             return "Mutex error";
        case MAM_ERROR_PLUGIN_NOT_ACTIVE: return "Plugin not active";
        case MAM_ERROR_TYPE_MISMATCH:     return "Type mismatch";
        default:                          return "Unknown error";
    }
}

// Internal helpers

static bool Lock(DWORD timeoutMs)
{
    if (!g_hMutex) return false;
    DWORD result = WaitForSingleObject(g_hMutex, timeoutMs);
    return (result == WAIT_OBJECT_0);
}

static void Unlock()
{
    if (g_hMutex) {
        ReleaseMutex(g_hMutex);
    }
}

static const MamDataRefEntry* FindEntry(uint32_t id)
{
    if (!g_shm) return nullptr;

    for (uint32_t i = 0; i < MAM_MAX_DATAREFS; i++) {
        if (g_shm->datarefs[i].active && g_shm->datarefs[i].id == id) {
            return &g_shm->datarefs[i];
        }
    }

    return nullptr;
}
