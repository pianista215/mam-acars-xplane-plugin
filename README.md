# MAM ACARS Bridge

X-Plane 12 plugin that exposes datarefs to external applications via shared memory, similar to how FSUIPC works for Microsoft Flight Simulator.

## Related Projects

- [MAM](https://github.com/pianista215/mam) - Main web application for airline management
- [MAM ACARS](https://github.com/pianista215/mam-acars) - Flight recorder for the MAM ecosystem

## What it does

- Reads X-Plane datarefs (position, speed, altitude, heading, etc.)
- Exposes them via Windows shared memory (FileMapping)
- Provides a client DLL for external applications to read the data
- Updates values at 10 Hz (every 100ms)
- Supports checking if a dataref exists (useful for aircraft detection)

## Requirements

- Windows 10/11
- Visual Studio 2022 with "Desktop development with C++" workload
- Windows 11 SDK (installed via Visual Studio Installer)
- X-Plane SDK 4.2.0
- X-Plane 12

## Project Structure

```
mam-xplane-plugin/
├── shared/
│   └── mam_protocol.h          # Shared data structures
├── plugin/
│   └── src/
│       ├── main.cpp            # Plugin entry point
│       ├── dataref_manager.h/cpp
│       ├── shared_memory.h/cpp
│       └── exports.def
├── client/
│   └── src/
│       ├── mam_client.h        # Public API
│       ├── mam_client.cpp
│       └── mam_client.def
├── test/
│   └── test_client.cpp         # Console test application
└── mam-xplane-plugin.sln
```

## Building

1. Download X-Plane SDK from https://developer.x-plane.com/sdk/plugin-sdk-downloads/
2. Extract to `C:\sources\XPSDK`

### From Visual Studio IDE

1. Open `mam-xplane-plugin.sln` in Visual Studio
2. Select configuration `Release | x64`
3. Build > Build Solution

### From command line

Use the **Developer Command Prompt for VS 2022** (search for it in the Start menu). A regular `cmd` will not work because it lacks the compiler environment variables.

```cmd
cd C:\sources\mam-xplane-plugin
msbuild mam-xplane-plugin.sln /p:Configuration=Release /p:Platform=x64
```

### Output files

| File | Description |
|------|-------------|
| `bin\Release\plugin\win.xpl` | X-Plane plugin |
| `bin\Release\mam_client.dll` | Client DLL for external apps |
| `bin\Release\mam_client.lib` | Import library for linking |
| `bin\Release\test_client.exe` | Console test application |

## Installation

1. Create folder: `X-Plane 12\Resources\plugins\MAM_ACARS_Bridge\64\`
2. Copy `bin\Release\plugin\win.xpl` to that folder

## Verification

1. Start X-Plane 12
2. Go to Developer > Plugin Admin
3. Verify that "MAM ACARS Bridge" appears
4. Check `X-Plane 12\Log.txt` for `[MAM]` messages

## Usage from External Applications

### Include in your project
```cpp
#include "mam_client.h"
// Link with mam_client.lib
```

### Example usage
```cpp
#include "mam_client.h"

int main() {
    // Connect to plugin
    if (MAM_Connect() != MAM_OK) {
        printf("Error: Cannot connect to plugin\n");
        return 1;
    }

    // Check aircraft-specific datarefs
    if (MAM_DataRefExists("KA350/ianim/pSubpanel/strobeLights")) {
        printf("King Air 350 detected!\n");
    }

    // Register datarefs
    int idLat = MAM_RegisterDataRef("sim/flightmodel/position/latitude");
    int idLon = MAM_RegisterDataRef("sim/flightmodel/position/longitude");
    int idAlt = MAM_RegisterDataRef("sim/flightmodel/position/elevation");

    // Read values
    double lat, lon, alt;
    while (true) {
        MAM_GetDouble(idLat, &lat);
        MAM_GetDouble(idLon, &lon);
        MAM_GetDouble(idAlt, &alt);

        printf("Pos: %.6f, %.6f, %.0fm\n", lat, lon, alt);
        Sleep(100);
    }

    MAM_Disconnect();
    return 0;
}
```

## Client API

| Function | Description |
|----------|-------------|
| `MAM_Connect()` | Connect to plugin |
| `MAM_Disconnect()` | Disconnect |
| `MAM_IsConnected()` | Check connection status |
| `MAM_IsPluginActive()` | Check if the plugin is active |
| `MAM_RegisterDataRef(path)` | Register a dataref (returns ID) |
| `MAM_DataRefExists(path)` | Check if a dataref exists (returns 1/0) |
| `MAM_GetDouble(id, &value)` | Read double value |
| `MAM_GetFloat(id, &value)` | Read float value |
| `MAM_GetInt(id, &value)` | Read int value |

## Common ACARS Datarefs

| Dataref | Type | Description |
|---------|------|-------------|
| `sim/flightmodel/position/latitude` | double | Latitude |
| `sim/flightmodel/position/longitude` | double | Longitude |
| `sim/flightmodel/position/elevation` | double | Altitude (meters) |
| `sim/flightmodel/position/indicated_airspeed` | float | IAS (knots) |
| `sim/flightmodel/position/groundspeed` | float | GS (m/s) |
| `sim/flightmodel/position/true_heading` | float | True heading |
| `sim/flightmodel/position/vh_ind_fpm` | float | VS (fpm) |

## Debugging

- **Plugin logs**: `X-Plane 12\Log.txt` (search for `[MAM]`)
- **Debug in VS**: Debug > Attach to Process > X-Plane.exe
- **DataRefTool**: Free plugin for exploring available datarefs

## Architecture

```
+------------------+     Shared Memory      +------------------+
|   X-Plane 12     |  (Windows FileMapping) |   ACARS App      |
|  +------------+  |                        |  +------------+  |
|  | MAM Plugin |<------------------------>|  | MAM Client |  |
|  |   (.xpl)   |  |                        |  |   (.dll)   |  |
|  +------------+  |                        |  +------------+  |
+------------------+                        +------------------+
```

- The plugin creates the shared memory on startup
- The client connects to the existing shared memory
- The plugin updates values at 10 Hz (every 100ms)
- Synchronization via Windows mutex

## License

This project is licensed under the **GNU Affero General Public License v3.0 (AGPL-3.0)**.

This means:
- You can use, modify, and distribute this software
- Any derivative work must also be licensed under AGPL-3.0
- If you run a modified version as a network service, you must make the source code available to users
- See [LICENSE](LICENSE) for the full text

Copyright (c) 2026 Unai Sarasola Alvarez
