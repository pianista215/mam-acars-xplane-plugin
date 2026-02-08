@echo off
REM Script to install the plugin into X-Plane 12
REM Run after compiling in Release mode

set XPLANE_PATH=C:\Program Files\X-Plane 12
set PLUGIN_DIR=%XPLANE_PATH%\Resources\plugins\MAM_ACARS_Bridge\64

echo ============================================
echo  MAM ACARS Bridge Plugin - Installer
echo ============================================
echo.

REM Check that the X-Plane directory exists
if not exist "%XPLANE_PATH%" (
    echo ERROR: X-Plane not found at %XPLANE_PATH%
    echo Please edit this script with the correct path.
    pause
    exit /b 1
)

REM Create plugin directory if it doesn't exist
if not exist "%PLUGIN_DIR%" (
    echo Creating plugin directory...
    mkdir "%PLUGIN_DIR%"
)

REM Check that the compiled plugin exists
if not exist "bin\Release\plugin\win.xpl" (
    echo ERROR: bin\Release\plugin\win.xpl not found
    echo Please compile the project in Release mode first.
    pause
    exit /b 1
)

REM Copy the plugin
echo Copying win.xpl to %PLUGIN_DIR%...
copy /Y "bin\Release\plugin\win.xpl" "%PLUGIN_DIR%\"

echo.
echo ============================================
echo  Plugin installed successfully!
echo ============================================
echo.
echo Location: %PLUGIN_DIR%\win.xpl
echo.
echo To verify:
echo 1. Start X-Plane 12
echo 2. Go to Developer ^> Plugin Admin
echo 3. Look for "MAM ACARS Bridge"
echo 4. Check Log.txt for [MAM] messages
echo.
pause
