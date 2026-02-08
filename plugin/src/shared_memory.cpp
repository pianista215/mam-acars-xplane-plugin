#include "shared_memory.h"
#include "XPLMUtilities.h"
#include <cstring>
#include <cstdio>

SharedMemory::SharedMemory()
    : m_hMapFile(NULL)
    , m_hMutex(NULL)
    , m_shm(nullptr)
{
}

SharedMemory::~SharedMemory()
{
    Close();
}

bool SharedMemory::Create()
{
    // Create mutex first
    m_hMutex = CreateMutexA(NULL, FALSE, MAM_MUTEX_NAME);
    if (m_hMutex == NULL) {
        char msg[128];
        snprintf(msg, sizeof(msg), "[MAM] Failed to create mutex, error=%lu\n", GetLastError());
        XPLMDebugString(msg);
        return false;
    }

    // Create file mapping
    DWORD size = sizeof(MamSharedMemory);
    m_hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,   // Use paging file
        NULL,                   // Default security
        PAGE_READWRITE,         // Read/write access
        0,                      // High-order DWORD of size
        size,                   // Low-order DWORD of size
        MAM_SHARED_MEMORY_NAME  // Name
    );

    if (m_hMapFile == NULL) {
        char msg[128];
        snprintf(msg, sizeof(msg), "[MAM] Failed to create file mapping, error=%lu\n", GetLastError());
        XPLMDebugString(msg);
        CloseHandle(m_hMutex);
        m_hMutex = NULL;
        return false;
    }

    // Map view
    m_shm = (MamSharedMemory*)MapViewOfFile(
        m_hMapFile,
        FILE_MAP_ALL_ACCESS,
        0, 0, size
    );

    if (m_shm == NULL) {
        char msg[128];
        snprintf(msg, sizeof(msg), "[MAM] Failed to map view, error=%lu\n", GetLastError());
        XPLMDebugString(msg);
        CloseHandle(m_hMapFile);
        CloseHandle(m_hMutex);
        m_hMapFile = NULL;
        m_hMutex = NULL;
        return false;
    }

    // Initialize shared memory
    if (Lock()) {
        memset(m_shm, 0, sizeof(MamSharedMemory));
        m_shm->protocolVersion = MAM_PROTOCOL_VERSION;
        m_shm->pluginActive = 1;
        m_shm->updateCounter = 0;
        m_shm->registeredCount = 0;
        m_shm->command.type = MAM_CMD_NONE;
        m_shm->command.processed = 1;
        Unlock();
    }

    XPLMDebugString("[MAM] Shared memory created successfully\n");
    return true;
}

void SharedMemory::Close()
{
    if (m_shm) {
        // Mark plugin as inactive before closing
        if (Lock(100)) {
            m_shm->pluginActive = 0;
            Unlock();
        }
        UnmapViewOfFile(m_shm);
        m_shm = nullptr;
    }

    if (m_hMapFile) {
        CloseHandle(m_hMapFile);
        m_hMapFile = NULL;
    }

    if (m_hMutex) {
        CloseHandle(m_hMutex);
        m_hMutex = NULL;
    }

    XPLMDebugString("[MAM] Shared memory closed\n");
}

bool SharedMemory::Lock(DWORD timeoutMs)
{
    if (!m_hMutex) return false;

    DWORD result = WaitForSingleObject(m_hMutex, timeoutMs);
    return (result == WAIT_OBJECT_0);
}

void SharedMemory::Unlock()
{
    if (m_hMutex) {
        ReleaseMutex(m_hMutex);
    }
}
