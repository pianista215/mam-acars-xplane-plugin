#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include "../../shared/mam_protocol.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class SharedMemory {
public:
    SharedMemory();
    ~SharedMemory();

    // Create shared memory (plugin side)
    bool Create();

    // Close shared memory
    void Close();

    // Check if valid
    bool IsValid() const { return m_shm != nullptr; }

    // Get pointer to shared memory (caller must lock/unlock)
    MamSharedMemory* GetData() { return m_shm; }

    // Lock/unlock for thread-safe access
    bool Lock(DWORD timeoutMs = 1000);
    void Unlock();

private:
    HANDLE m_hMapFile;
    HANDLE m_hMutex;
    MamSharedMemory* m_shm;
};

#endif // SHARED_MEMORY_H
