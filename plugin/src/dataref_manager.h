#ifndef DATAREF_MANAGER_H
#define DATAREF_MANAGER_H

#include "XPLMDataAccess.h"
#include "../../shared/mam_protocol.h"
#include <string>
#include <unordered_map>

class DataRefManager {
public:
    DataRefManager();
    ~DataRefManager();

    // Register a dataref by path, returns ID (1-based) or 0 on failure
    uint32_t RegisterDataRef(const char* path);

    // Unregister a dataref by ID
    bool UnregisterDataRef(uint32_t id);

    // Update all registered dataref values
    void UpdateAllValues();

    // Get a dataref entry by ID (for copying to shared memory)
    const MamDataRefEntry* GetEntry(uint32_t id) const;

    // Get all entries for copying to shared memory
    const MamDataRefEntry* GetEntries() const { return m_entries; }
    uint32_t GetRegisteredCount() const { return m_registeredCount; }

private:
    struct DataRefInfo {
        XPLMDataRef ref;
        XPLMDataTypeID typeId;
        uint32_t entryIndex;
    };

    MamDataRefEntry m_entries[MAM_MAX_DATAREFS];
    std::unordered_map<uint32_t, DataRefInfo> m_refs;  // id -> info
    uint32_t m_nextId;
    uint32_t m_registeredCount;

    MamDataType XPLMTypeToMamType(XPLMDataTypeID typeId);
    void ReadValue(XPLMDataRef ref, XPLMDataTypeID typeId, MamDataRefEntry& entry);
};

#endif // DATAREF_MANAGER_H
