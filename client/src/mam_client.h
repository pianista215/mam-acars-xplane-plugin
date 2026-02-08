#ifndef MAM_CLIENT_H
#define MAM_CLIENT_H

#include "../../shared/mam_protocol.h"

#ifdef MAM_CLIENT_EXPORTS
    #define MAM_API __declspec(dllexport)
#else
    #define MAM_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Connect to the MAM ACARS Bridge plugin
 * @return MAM_OK on success, error code otherwise
 */
MAM_API MamResult MAM_Connect(void);

/**
 * @brief Disconnect from the plugin
 */
MAM_API void MAM_Disconnect(void);

/**
 * @brief Check if connected to the plugin
 * @return 1 if connected, 0 otherwise
 */
MAM_API int MAM_IsConnected(void);

/**
 * @brief Check if the plugin is active (X-Plane is running with the plugin)
 * @return 1 if active, 0 otherwise
 */
MAM_API int MAM_IsPluginActive(void);

/**
 * @brief Register a dataref for reading
 * @param path The dataref path (e.g., "sim/flightmodel/position/latitude")
 * @return Dataref ID (>0) on success, 0 on failure
 */
MAM_API int MAM_RegisterDataRef(const char* path);

/**
 * @brief Unregister a previously registered dataref
 * @param id The dataref ID returned by MAM_RegisterDataRef
 * @return MAM_OK on success, error code otherwise
 */
MAM_API MamResult MAM_UnregisterDataRef(int id);

/**
 * @brief Get a double value from a registered dataref
 * @param id The dataref ID
 * @param outValue Pointer to store the value
 * @return MAM_OK on success, error code otherwise
 */
MAM_API MamResult MAM_GetDouble(int id, double* outValue);

/**
 * @brief Get a float value from a registered dataref
 * @param id The dataref ID
 * @param outValue Pointer to store the value
 * @return MAM_OK on success, error code otherwise
 */
MAM_API MamResult MAM_GetFloat(int id, float* outValue);

/**
 * @brief Get an integer value from a registered dataref
 * @param id The dataref ID
 * @param outValue Pointer to store the value
 * @return MAM_OK on success, error code otherwise
 */
MAM_API MamResult MAM_GetInt(int id, int* outValue);

/**
 * @brief Get the update counter (incremented every update cycle)
 * @return Update counter value, or 0 if not connected
 */
MAM_API uint32_t MAM_GetUpdateCounter(void);

/**
 * @brief Get the number of registered datarefs
 * @return Number of registered datarefs, or 0 if not connected
 */
MAM_API uint32_t MAM_GetRegisteredCount(void);

/**
 * @brief Get a human-readable error message
 * @param result The result code
 * @return Error message string
 */
MAM_API const char* MAM_GetErrorString(MamResult result);

#ifdef __cplusplus
}
#endif

#endif // MAM_CLIENT_H
