# MAM ACARS Bridge

Plugin de X-Plane 12 que expone datarefs a aplicaciones externas mediante memoria compartida.

## Requisitos

- Visual Studio 2022 con workload "Desktop development with C++"
- X-Plane SDK 4.2.0 extraído en `C:\sources\XPSDK`
- X-Plane 12

## Estructura del Proyecto

```
mam-xplane-plugin/
├── shared/
│   └── mam_protocol.h          # Estructuras compartidas
├── plugin/
│   └── src/
│       ├── main.cpp            # Punto de entrada del plugin
│       ├── dataref_manager.h/cpp
│       ├── shared_memory.h/cpp
│       └── exports.def
├── client/
│   └── src/
│       ├── mam_client.h        # API pública
│       ├── mam_client.cpp
│       └── mam_client.def
├── test/
│   └── test_client.cpp         # Aplicación de prueba
└── mam-xplane-plugin.sln
```

## Compilación

1. Descargar X-Plane SDK de https://developer.x-plane.com/sdk/plugin-sdk-downloads/
2. Extraer en `C:\sources\XPSDK`
3. Abrir `mam-xplane-plugin.sln` en Visual Studio
4. Seleccionar configuración `Release | x64`
5. Build > Build Solution

## Instalación

### Automática
Ejecutar `install_plugin.bat` después de compilar.

### Manual
1. Crear carpeta: `X-Plane 12\Resources\plugins\MAM_ACARS_Bridge\64\`
2. Copiar `bin\Release\plugin\win.xpl` a esa carpeta

## Verificación

1. Iniciar X-Plane 12
2. Ir a Developer > Plugin Admin
3. Verificar que "MAM ACARS Bridge" aparece
4. Revisar `X-Plane 12\Log.txt` - buscar mensajes `[MAM]`

## Uso desde Aplicaciones Externas

### Incluir en tu proyecto
```cpp
#include "mam_client.h"
// Linkear con mam_client.lib
```

### Ejemplo de uso
```cpp
#include "mam_client.h"

int main() {
    // Conectar al plugin
    if (MAM_Connect() != MAM_OK) {
        printf("Error: No se puede conectar al plugin\n");
        return 1;
    }

    // Registrar datarefs
    int idLat = MAM_RegisterDataRef("sim/flightmodel/position/latitude");
    int idLon = MAM_RegisterDataRef("sim/flightmodel/position/longitude");
    int idAlt = MAM_RegisterDataRef("sim/flightmodel/position/elevation");

    // Leer valores
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

## API del Cliente

| Función | Descripción |
|---------|-------------|
| `MAM_Connect()` | Conectar al plugin |
| `MAM_Disconnect()` | Desconectar |
| `MAM_IsConnected()` | Verificar conexión |
| `MAM_IsPluginActive()` | Verificar si el plugin está activo |
| `MAM_RegisterDataRef(path)` | Registrar un dataref (retorna ID) |
| `MAM_GetDouble(id, &value)` | Leer valor double |
| `MAM_GetFloat(id, &value)` | Leer valor float |
| `MAM_GetInt(id, &value)` | Leer valor int |

## Datarefs Comunes para ACARS

| Dataref | Tipo | Descripción |
|---------|------|-------------|
| `sim/flightmodel/position/latitude` | double | Latitud |
| `sim/flightmodel/position/longitude` | double | Longitud |
| `sim/flightmodel/position/elevation` | double | Altitud (metros) |
| `sim/flightmodel/position/indicated_airspeed` | float | IAS (knots) |
| `sim/flightmodel/position/groundspeed` | float | GS (m/s) |
| `sim/flightmodel/position/true_heading` | float | Rumbo verdadero |
| `sim/flightmodel/position/vh_ind_fpm` | float | VS (fpm) |

## Depuración

- **Logs del plugin**: `X-Plane 12\Log.txt` (buscar `[MAM]`)
- **Debug en VS**: Debug > Attach to Process > X-Plane.exe
- **DataRefTool**: Plugin gratuito para explorar datarefs

## Arquitectura

```
+------------------+     Shared Memory      +------------------+
|   X-Plane 12     |  (Windows FileMapping) |   ACARS App      |
|  +------------+  |                        |  +------------+  |
|  | MAM Plugin |<------------------------>|  | MAM Client |  |
|  |   (.xpl)   |  |                        |  |   (.dll)   |  |
|  +------------+  |                        |  +------------+  |
+------------------+                        +------------------+
```

- El plugin crea la memoria compartida al iniciar
- El cliente se conecta a la memoria compartida existente
- El plugin actualiza valores a 10 Hz (cada 100ms)
- Sincronización mediante mutex de Windows
