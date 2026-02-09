#include <cstdio>
#include <cstdlib>
#include <windows.h>
#include "../client/src/mam_client.h"

// Common ACARS datarefs
static const char* DATAREFS[] = {
    "sim/flightmodel/position/latitude",
    "sim/flightmodel/position/longitude",
    "sim/flightmodel/position/elevation",
    "sim/flightmodel/position/indicated_airspeed",
    "sim/flightmodel/position/groundspeed",
    "sim/flightmodel/position/true_heading",
    "sim/flightmodel/position/mag_heading",
    "sim/flightmodel/position/vh_ind_fpm",  // Vertical speed
    "sim/cockpit2/gauges/indicators/altitude_ft_pilot",
    nullptr
};

int main(int argc, char* argv[])
{
    printf("===========================================\n");
    printf("  MAM ACARS Bridge - Test Client\n");
    printf("===========================================\n\n");

    // Connect to plugin
    printf("Connecting to MAM ACARS Bridge plugin...\n");
    MamResult result = MAM_Connect();
    if (result != MAM_OK) {
        printf("ERROR: Failed to connect: %s\n", MAM_GetErrorString(result));
        printf("\nMake sure X-Plane is running with the MAM ACARS Bridge plugin loaded.\n");
        printf("Press Enter to exit...\n");
        getchar();
        return 1;
    }
    printf("Connected successfully!\n\n");

    // Check if plugin is active
    if (!MAM_IsPluginActive()) {
        printf("WARNING: Plugin is connected but not active.\n");
        printf("Make sure the plugin is enabled in X-Plane.\n\n");
    }

    // Read aircraft identification
    char icao[256] = {0};
    char author[256] = {0};
    char descrip[256] = {0};
    char tailnum[256] = {0};

    printf("Aircraft identification:\n");
    if (MAM_GetString("sim/aircraft/view/acf_ICAO", icao, sizeof(icao)) == MAM_OK)
        printf("  ICAO:        %s\n", icao);
    if (MAM_GetString("sim/aircraft/view/acf_author", author, sizeof(author)) == MAM_OK)
        printf("  Author:      %s\n", author);
    if (MAM_GetString("sim/aircraft/view/acf_descrip", descrip, sizeof(descrip)) == MAM_OK)
        printf("  Description: %s\n", descrip);
    if (MAM_GetString("sim/aircraft/view/acf_tailnum", tailnum, sizeof(tailnum)) == MAM_OK)
        printf("  Tail number: %s\n", tailnum);
    printf("\n");

    // Check aircraft-specific dataref
    const char* kaDataRef = "KA350/ianim/pSubpanel/strobeLights";
    if (MAM_DataRefExists(kaDataRef)) {
        printf("[DataRef Check] %s -> EXISTS\n\n", kaDataRef);
    } else {
        printf("[DataRef Check] %s -> NOT FOUND\n\n", kaDataRef);
    }

    // Register datarefs
    printf("Registering datarefs...\n");
    int ids[16] = {0};
    int idCount = 0;

    for (int i = 0; DATAREFS[i] != nullptr && idCount < 16; i++) {
        int id = MAM_RegisterDataRef(DATAREFS[i]);
        if (id > 0) {
            printf("  [OK] %s (id=%d)\n", DATAREFS[i], id);
            ids[idCount++] = id;
        } else {
            printf("  [FAIL] %s\n", DATAREFS[i]);
        }
    }

    if (idCount == 0) {
        printf("\nNo datarefs registered. Check if X-Plane is running.\n");
        MAM_Disconnect();
        printf("Press Enter to exit...\n");
        getchar();
        return 1;
    }

    printf("\n%d datarefs registered.\n", idCount);
    printf("Press Ctrl+C to exit, or close this window.\n\n");

    // Main loop - display values
    uint32_t lastCounter = 0;
    while (1) {
        // Wait for update
        uint32_t counter = MAM_GetUpdateCounter();
        if (counter == lastCounter) {
            Sleep(50);
            continue;
        }
        lastCounter = counter;

        // Clear screen and print header
        system("cls");
        printf("===========================================\n");
        printf("  MAM ACARS Bridge - Live Data\n");
        printf("  Update #%u | %d datarefs\n", counter, MAM_GetRegisteredCount());
        printf("===========================================\n\n");

        // Print values
        for (int i = 0; i < idCount; i++) {
            double value = 0.0;
            MamResult r = MAM_GetDouble(ids[i], &value);
            if (r == MAM_OK) {
                printf("%-50s %12.4f\n", DATAREFS[i], value);
            } else {
                printf("%-50s ERROR: %s\n", DATAREFS[i], MAM_GetErrorString(r));
            }
        }

        printf("\nPress Ctrl+C to exit...\n");

        Sleep(100);  // 10 Hz update
    }

    MAM_Disconnect();
    return 0;
}
