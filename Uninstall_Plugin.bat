@echo off
setlocal enabledelayedexpansion

title Uninstalling CropMarksAI Native Plugin (.aip)
cd /d "%~dp0"

:: --- 1. Check Administrator Privileges (UAC) ---
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [INFO] Requesting Administrator privileges to remove from Program Files...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

cd /d "%~dp0"
cls

echo ================================================================================
echo       UNINSTALLING CROPMARKS AI NATIVE C++ PLUGIN (.aip)
echo ================================================================================
echo.

:: --- 2. Scan for Installed Copies of CropMarksAI.aip ---
echo [*] Scanning for installed copies of CropMarksAI.aip...
echo.

set "TOTAL_FOUND=0"
for /d %%D in ("C:\Program Files\Adobe\Adobe Illustrator *") do (
    set "HAS_PLUGIN=0"
    if exist "%%D\Plug-ins\CropMarksAI.aip" set "HAS_PLUGIN=1"
    if exist "%%D\Plug-ins\MakeCropMarksAI.aip" set "HAS_PLUGIN=1"
    if "!HAS_PLUGIN!"=="1" (
        set /a TOTAL_FOUND+=1
        set "AI_DIR_!TOTAL_FOUND!=%%D"
        set "AI_NAME_!TOTAL_FOUND!=%%~nxD"
    )
)

if %TOTAL_FOUND% equ 0 (
    echo ================================================================================
    echo  [INFO] No installed copies of CropMarksAI.aip were found.
    echo ================================================================================
    echo.
    echo Press any key to exit...
    pause >nul
    exit /b 0
)

:: --- 3. Interactive Version Selection Menu ---
echo ================================================================================
echo  FOUND INSTALLED COPIES IN (%TOTAL_FOUND% total):
echo ================================================================================
echo.
echo   [0] Uninstall from ALL found versions (Recommended)
for /L %%I in (1,1,%TOTAL_FOUND%) do (
    echo   [%%I] !AI_NAME_%%I!
)
echo   [Q] Cancel and Exit
echo.
echo ================================================================================

:PROMPT_CHOICE
set "USER_CHOICE=0"
set /p "USER_CHOICE=Select uninstall target [0-%TOTAL_FOUND%] (default 0): "

if /i "%USER_CHOICE%"=="Q" (
    echo.
    echo [INFO] Uninstall cancelled by user.
    echo Press any key to exit...
    pause >nul
    exit /b 0
)

set "VALID_CHOICE=0"
if "%USER_CHOICE%"=="0" set "VALID_CHOICE=1"
for /L %%I in (1,1,%TOTAL_FOUND%) do (
    if "%USER_CHOICE%"=="%%I" set "VALID_CHOICE=1"
)

if "%VALID_CHOICE%"=="0" (
    echo [WARNING] Invalid selection. Please enter a number between 0 and %TOTAL_FOUND% or Q.
    goto PROMPT_CHOICE
)

:: --- 4. Remove Plugin Files ---
set "REMOVED_COUNT=0"
echo.

if "%USER_CHOICE%"=="0" (
    for /L %%I in (1,1,%TOTAL_FOUND%) do (
        set "TARGET_FILE=!AI_DIR_%%I!\Plug-ins\CropMarksAI.aip"
        set "LEGACY_FILE=!AI_DIR_%%I!\Plug-ins\MakeCropMarksAI.aip"
        echo [*] Removing from: !AI_NAME_%%I!\Plug-ins\
        if exist "!TARGET_FILE!" del /f /q "!TARGET_FILE!" >nul 2>&1
        if exist "!LEGACY_FILE!" del /f /q "!LEGACY_FILE!" >nul 2>&1
        if not exist "!TARGET_FILE!" (
            echo  [OK] Successfully removed from: !AI_NAME_%%I!\Plug-ins\
            set /a REMOVED_COUNT+=1
        ) else (
            echo  [-] ERROR: Failed to remove from !AI_NAME_%%I!
        )
    )
) else (
    set "TARGET_FILE=!AI_DIR_%USER_CHOICE%!\Plug-ins\CropMarksAI.aip"
    set "LEGACY_FILE=!AI_DIR_%USER_CHOICE%!\Plug-ins\MakeCropMarksAI.aip"
    set "TARGET_NAME=!AI_NAME_%USER_CHOICE%!"
    echo [*] Removing from: !TARGET_NAME!\Plug-ins\
    if exist "!TARGET_FILE!" del /f /q "!TARGET_FILE!" >nul 2>&1
    if exist "!LEGACY_FILE!" del /f /q "!LEGACY_FILE!" >nul 2>&1
    if not exist "!TARGET_FILE!" (
        echo  [OK] Successfully removed from: !TARGET_NAME!\Plug-ins\
        set /a REMOVED_COUNT+=1
    ) else (
        echo  [-] ERROR: Failed to remove from !TARGET_NAME!
    )
)

echo.
echo ================================================================================
if %REMOVED_COUNT% gtr 0 (
    echo                    UNINSTALL COMPLETED SUCCESSFULLY
    echo ================================================================================
    echo.
    echo  CropMarksAI plugin was removed from %REMOVED_COUNT% version(s).
    echo.
    echo ================================================================================

    powershell -NoProfile -Command "Add-Type -AssemblyName PresentationFramework; [System.Windows.MessageBox]::Show('CropMarksAI plugin has been successfully removed.', 'CropMarksAI - Uninstall Complete', 'OK', 'Information')" >nul 2>&1

) else (
    echo                         UNINSTALL FAILED
    echo ================================================================================
    echo  Failed to remove plugin file. Please ensure Illustrator is closed
    echo  and you are running this uninstaller as Administrator.
    echo ================================================================================
)

echo.
echo Press any key to exit...
pause >nul
exit /b 0
