#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "dataref_manager.h"
#include "shared_memory.h"
#include <cstring>
#include <cstdio>

// Global instances
static DataRefManager* g_dataRefManager = nullptr;
static SharedMemory* g_sharedMemory = nullptr;
static XPLMFlightLoopID g_flightLoopId = nullptr;

// Forward declarations
static float FlightLoopCallback(float inElapsedSinceLastCall,
                                float inElapsedTimeSinceLastFlightLoop,
                                int inCounter,
                                void* inRefcon);
static void ProcessCommands();
static void UpdateSharedMemory();

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy_s(outName, 256, "MAM ACARS Bridge");
    strcpy_s(outSig, 256, "com.mam.acarsbridge");
    strcpy_s(outDesc, 256, "Exposes datarefs via shared memory for ACARS");

    XPLMDebugString("[MAM] ====================================\n");
    XPLMDebugString("[MAM] MAM ACARS Bridge Plugin Starting\n");
    XPLMDebugString("[MAM] ====================================\n");

    // Create managers
    g_dataRefManager = new DataRefManager();
    g_sharedMemory = new SharedMemory();

    // Create shared memory
    if (!g_sharedMemory->Create()) {
        XPLMDebugString("[MAM] ERROR: Failed to create shared memory!\n");
        delete g_sharedMemory;
        delete g_dataRefManager;
        g_sharedMemory = nullptr;
        g_dataRefManager = nullptr;
        return 0;
    }

    // Register flight loop
    XPLMCreateFlightLoop_t flightLoopParams;
    flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
    flightLoopParams.phase = xplm_FlightLoop_Phase_AfterFlightModel;
    flightLoopParams.callbackFunc = FlightLoopCallback;
    flightLoopParams.refcon = nullptr;

    g_flightLoopId = XPLMCreateFlightLoop(&flightLoopParams);
    if (!g_flightLoopId) {
        XPLMDebugString("[MAM] ERROR: Failed to create flight loop!\n");
        delete g_sharedMemory;
        delete g_dataRefManager;
        g_sharedMemory = nullptr;
        g_dataRefManager = nullptr;
        return 0;
    }

    XPLMDebugString("[MAM] Plugin started successfully\n");
    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    XPLMDebugString("[MAM] Plugin stopping...\n");

    if (g_flightLoopId) {
        XPLMDestroyFlightLoop(g_flightLoopId);
        g_flightLoopId = nullptr;
    }

    if (g_sharedMemory) {
        delete g_sharedMemory;
        g_sharedMemory = nullptr;
    }

    if (g_dataRefManager) {
        delete g_dataRefManager;
        g_dataRefManager = nullptr;
    }

    XPLMDebugString("[MAM] Plugin stopped\n");
}

PLUGIN_API int XPluginEnable(void)
{
    XPLMDebugString("[MAM] Plugin enabled\n");

    // Schedule flight loop to run every 100ms (10 Hz)
    if (g_flightLoopId) {
        XPLMScheduleFlightLoop(g_flightLoopId, 0.1f, 1);
    }

    // Mark plugin as active in shared memory
    if (g_sharedMemory && g_sharedMemory->IsValid()) {
        if (g_sharedMemory->Lock()) {
            g_sharedMemory->GetData()->pluginActive = 1;
            g_sharedMemory->Unlock();
        }
    }

    return 1;
}

PLUGIN_API void XPluginDisable(void)
{
    XPLMDebugString("[MAM] Plugin disabled\n");

    // Stop flight loop
    if (g_flightLoopId) {
        XPLMScheduleFlightLoop(g_flightLoopId, 0, 0);
    }

    // Mark plugin as inactive in shared memory
    if (g_sharedMemory && g_sharedMemory->IsValid()) {
        if (g_sharedMemory->Lock()) {
            g_sharedMemory->GetData()->pluginActive = 0;
            g_sharedMemory->Unlock();
        }
    }
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam)
{
    // Handle system messages if needed
    (void)inFromWho;
    (void)inMessage;
    (void)inParam;
}

static float FlightLoopCallback(float inElapsedSinceLastCall,
                                float inElapsedTimeSinceLastFlightLoop,
                                int inCounter,
                                void* inRefcon)
{
    (void)inElapsedSinceLastCall;
    (void)inElapsedTimeSinceLastFlightLoop;
    (void)inCounter;
    (void)inRefcon;

    // Process any pending commands from client
    ProcessCommands();

    // Update all dataref values
    if (g_dataRefManager) {
        g_dataRefManager->UpdateAllValues();
    }

    // Copy values to shared memory
    UpdateSharedMemory();

    // Return interval for next call (100ms = 10 Hz)
    return 0.1f;
}

static void ProcessCommands()
{
    if (!g_sharedMemory || !g_sharedMemory->IsValid() || !g_dataRefManager) {
        return;
    }

    if (!g_sharedMemory->Lock(10)) {
        return;
    }

    MamSharedMemory* shm = g_sharedMemory->GetData();
    MamCommandEntry& cmd = shm->command;

    // Check if there's a pending command
    if (cmd.type != MAM_CMD_NONE && !cmd.processed) {
        switch (cmd.type) {
            case MAM_CMD_REGISTER_DATAREF: {
                uint32_t id = g_dataRefManager->RegisterDataRef(cmd.path);
                if (id > 0) {
                    cmd.resultId = id;
                    cmd.resultCode = MAM_OK;
                } else {
                    cmd.resultId = 0;
                    cmd.resultCode = MAM_ERROR_DATAREF_NOT_FOUND;
                }
                break;
            }

            case MAM_CMD_UNREGISTER_DATAREF: {
                if (g_dataRefManager->UnregisterDataRef(cmd.resultId)) {
                    cmd.resultCode = MAM_OK;
                } else {
                    cmd.resultCode = MAM_ERROR_INVALID_ID;
                }
                break;
            }

            case MAM_CMD_CHECK_DATAREF: {
                XPLMDataRef ref = XPLMFindDataRef(cmd.path);
                cmd.resultCode = ref ? MAM_OK : MAM_ERROR_DATAREF_NOT_FOUND;
                break;
            }

            case MAM_CMD_READ_STRING: {
                XPLMDataRef ref = XPLMFindDataRef(cmd.path);
                cmd.resultString[0] = '\0';
                if (ref) {
                    int len = XPLMGetDatab(ref, cmd.resultString, 0, MAM_STRING_VALUE_MAX - 1);
                    if (len > 0) {
                        if (len >= MAM_STRING_VALUE_MAX) len = MAM_STRING_VALUE_MAX - 1;
                        cmd.resultString[len] = '\0';
                        cmd.resultCode = MAM_OK;
                    } else {
                        cmd.resultCode = MAM_ERROR_TYPE_MISMATCH;
                    }
                } else {
                    cmd.resultCode = MAM_ERROR_DATAREF_NOT_FOUND;
                }
                break;
            }

            default:
                cmd.resultCode = MAM_ERROR_INVALID_ID;
                break;
        }

        cmd.processed = 1;
        cmd.type = MAM_CMD_NONE;
    }

    g_sharedMemory->Unlock();
}

static void UpdateSharedMemory()
{
    if (!g_sharedMemory || !g_sharedMemory->IsValid() || !g_dataRefManager) {
        return;
    }

    if (!g_sharedMemory->Lock(10)) {
        return;
    }

    MamSharedMemory* shm = g_sharedMemory->GetData();

    // Copy dataref entries
    const MamDataRefEntry* entries = g_dataRefManager->GetEntries();
    memcpy(shm->datarefs, entries, sizeof(shm->datarefs));

    // Update counters
    shm->registeredCount = g_dataRefManager->GetRegisteredCount();
    shm->updateCounter++;

    g_sharedMemory->Unlock();
}
