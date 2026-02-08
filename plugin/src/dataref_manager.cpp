#include "dataref_manager.h"
#include "XPLMUtilities.h"
#include <cstring>
#include <cstdio>

DataRefManager::DataRefManager()
    : m_nextId(1)
    , m_registeredCount(0)
{
    memset(m_entries, 0, sizeof(m_entries));
}

DataRefManager::~DataRefManager()
{
    m_refs.clear();
}

uint32_t DataRefManager::RegisterDataRef(const char* path)
{
    if (!path || path[0] == '\0') {
        XPLMDebugString("[MAM] RegisterDataRef: empty path\n");
        return 0;
    }

    // Check if already registered
    for (uint32_t i = 0; i < MAM_MAX_DATAREFS; i++) {
        if (m_entries[i].active && strcmp(m_entries[i].path, path) == 0) {
            char msg[512];
            snprintf(msg, sizeof(msg), "[MAM] DataRef already registered: %s (id=%u)\n", path, m_entries[i].id);
            XPLMDebugString(msg);
            return m_entries[i].id;
        }
    }

    // Find the dataref in X-Plane
    XPLMDataRef ref = XPLMFindDataRef(path);
    if (!ref) {
        char msg[512];
        snprintf(msg, sizeof(msg), "[MAM] DataRef not found: %s\n", path);
        XPLMDebugString(msg);
        return 0;
    }

    // Get type
    XPLMDataTypeID typeId = XPLMGetDataRefTypes(ref);
    if (typeId == xplmType_Unknown) {
        char msg[512];
        snprintf(msg, sizeof(msg), "[MAM] DataRef has unknown type: %s\n", path);
        XPLMDebugString(msg);
        return 0;
    }

    // Find free slot
    uint32_t slotIndex = MAM_MAX_DATAREFS;
    for (uint32_t i = 0; i < MAM_MAX_DATAREFS; i++) {
        if (!m_entries[i].active) {
            slotIndex = i;
            break;
        }
    }

    if (slotIndex >= MAM_MAX_DATAREFS) {
        XPLMDebugString("[MAM] Max datarefs reached\n");
        return 0;
    }

    // Assign ID and populate entry
    uint32_t id = m_nextId++;

    MamDataRefEntry& entry = m_entries[slotIndex];
    entry.id = id;
    entry.active = 1;
    strcpy_s(entry.path, MAM_DATAREF_PATH_MAX, path);
    entry.type = XPLMTypeToMamType(typeId);
    entry.value.d = 0.0;

    // Store reference info
    DataRefInfo info;
    info.ref = ref;
    info.typeId = typeId;
    info.entryIndex = slotIndex;
    m_refs[id] = info;

    m_registeredCount++;

    char msg[512];
    snprintf(msg, sizeof(msg), "[MAM] Registered dataref: %s (id=%u, slot=%u, type=%d)\n",
             path, id, slotIndex, entry.type);
    XPLMDebugString(msg);

    // Read initial value
    ReadValue(ref, typeId, entry);

    return id;
}

bool DataRefManager::UnregisterDataRef(uint32_t id)
{
    auto it = m_refs.find(id);
    if (it == m_refs.end()) {
        return false;
    }

    uint32_t slotIndex = it->second.entryIndex;
    m_entries[slotIndex].active = 0;
    m_entries[slotIndex].id = 0;
    m_refs.erase(it);
    m_registeredCount--;

    char msg[128];
    snprintf(msg, sizeof(msg), "[MAM] Unregistered dataref id=%u\n", id);
    XPLMDebugString(msg);

    return true;
}

void DataRefManager::UpdateAllValues()
{
    for (auto& pair : m_refs) {
        DataRefInfo& info = pair.second;
        MamDataRefEntry& entry = m_entries[info.entryIndex];
        ReadValue(info.ref, info.typeId, entry);
    }
}

const MamDataRefEntry* DataRefManager::GetEntry(uint32_t id) const
{
    auto it = m_refs.find(id);
    if (it == m_refs.end()) {
        return nullptr;
    }
    return &m_entries[it->second.entryIndex];
}

MamDataType DataRefManager::XPLMTypeToMamType(XPLMDataTypeID typeId)
{
    // Prefer double > float > int
    if (typeId & xplmType_Double) return MAM_TYPE_DOUBLE;
    if (typeId & xplmType_Float) return MAM_TYPE_FLOAT;
    if (typeId & xplmType_Int) return MAM_TYPE_INT;
    return MAM_TYPE_UNKNOWN;
}

void DataRefManager::ReadValue(XPLMDataRef ref, XPLMDataTypeID typeId, MamDataRefEntry& entry)
{
    // Read based on preferred type (same order as XPLMTypeToMamType)
    if (typeId & xplmType_Double) {
        entry.value.d = XPLMGetDatad(ref);
        entry.type = MAM_TYPE_DOUBLE;
    } else if (typeId & xplmType_Float) {
        entry.value.f = XPLMGetDataf(ref);
        entry.type = MAM_TYPE_FLOAT;
    } else if (typeId & xplmType_Int) {
        entry.value.i = XPLMGetDatai(ref);
        entry.type = MAM_TYPE_INT;
    }
}
