@echo off
setlocal enabledelayedexpansion

title Installing CropMarksAI Native Plugin (.aip)
cd /d "%~dp0"

:: --- 1. Check Administrator Privileges (UAC) ---
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [INFO] Requesting Administrator privileges to install into Program Files...
    if "%~1"=="" (
        powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    ) else (
        powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -ArgumentList '%*' -Verb RunAs"
    )
    exit /b
)

set "AUTO_MODE=0"
if /i "%~1"=="/auto" set "AUTO_MODE=1"
if /i "%~1"=="/y" set "AUTO_MODE=1"

cd /d "%~dp0"
cls

echo ================================================================================
echo       INSTALLING CROPMARKS AI NATIVE C++ PLUGIN (.aip)
echo           High-Performance Prepress Crop Marks Generator
echo ================================================================================
echo.

:: --- 2. Check for Compiled Plugin ---
set "AIP_SRC="
if exist "%~dp0CropMarksAI.aip" set "AIP_SRC=%~dp0CropMarksAI.aip"
if not defined AIP_SRC (
    if exist "%~dp0build\x64\CropMarksAI.aip" set "AIP_SRC=%~dp0build\x64\CropMarksAI.aip"
)
if not defined AIP_SRC (
    if exist "%~dp0build.bat" (
        echo [INFO] Compiled plugin file not found.
        echo [*] Starting automatic build via build.bat...
        call "%~dp0build.bat"
        if exist "%~dp0build\x64\CropMarksAI.aip" set "AIP_SRC=%~dp0build\x64\CropMarksAI.aip"
    )
)

if not defined AIP_SRC (
    echo.
    echo ================================================================================
    echo  [ERROR] Failed to find or compile CropMarksAI.aip
    echo  Looked in:
    echo    1. %~dp0CropMarksAI.aip
    echo    2. %~dp0build\x64\CropMarksAI.aip
    echo ================================================================================
    echo.
    echo Press any key to exit...
    pause >nul
    exit /b 1
)

:: --- 3. Check for Running Illustrator Process ---
tasklist /FI "IMAGENAME eq Illustrator.exe" 2>nul | find /I /N "Illustrator.exe" >nul
if "%ERRORLEVEL%"=="0" (
    echo [WARNING] Adobe Illustrator is currently running!
    echo Plugin files cannot be replaced while Illustrator is open.
    echo.
    if "%AUTO_MODE%"=="1" (
        echo [*] Auto mode: Closing Illustrator.exe...
        taskkill /F /IM Illustrator.exe >nul 2>&1
        timeout /t 2 /nobreak >nul
    ) else (
        set "CLOSE_AI=Y"
        set /p "CLOSE_AI=Close Adobe Illustrator automatically now? [Y/N] (default Y): "
        if /i "!CLOSE_AI!"=="Y" (
            echo [*] Closing Illustrator.exe...
            taskkill /F /IM Illustrator.exe >nul 2>&1
            timeout /t 2 /nobreak >nul
        ) else (
            echo [INFO] Please save your work and close Illustrator, then press any key to continue...
            pause >nul
        )
    )
    echo.
)

:: --- 4. Scan for Installed Adobe Illustrator Versions ---
echo [*] Scanning for installed Adobe Illustrator versions...
echo.

set "TOTAL_FOUND=0"
for /d %%D in ("C:\Program Files\Adobe\Adobe Illustrator *") do (
    if exist "%%D\Support Files\Contents\Windows\Illustrator.exe" (
        set /a TOTAL_FOUND+=1
        set "AI_DIR_!TOTAL_FOUND!=%%D"
        set "AI_NAME_!TOTAL_FOUND!=%%~nxD"
    ) else if exist "%%D\Plug-ins" (
        set /a TOTAL_FOUND+=1
        set "AI_DIR_!TOTAL_FOUND!=%%D"
        set "AI_NAME_!TOTAL_FOUND!=%%~nxD"
    )
)

if %TOTAL_FOUND% equ 0 (
    echo ================================================================================
    echo  [ERROR] No Adobe Illustrator installations found in 'C:\Program Files\Adobe\'
    echo ================================================================================
    echo.
    echo Press any key to exit...
    pause >nul
    exit /b 1
)

:: --- 5. Interactive Version Selection Menu ---
echo ================================================================================
echo  FOUND ADOBE ILLUSTRATOR VERSIONS (%TOTAL_FOUND% total):
echo ================================================================================
echo.
echo   [0] Install into ALL found versions (Recommended)
for /L %%I in (1,1,%TOTAL_FOUND%) do (
    echo   [%%I] !AI_NAME_%%I!
)
echo   [Q] Cancel and Exit
echo.
echo ================================================================================

:PROMPT_CHOICE
if "%AUTO_MODE%"=="1" (
    set "USER_CHOICE=0"
    goto PROCEED_INSTALL
)
set "USER_CHOICE=0"
set /p "USER_CHOICE=Select installation target [0-%TOTAL_FOUND%] (default 0): "

if /i "%USER_CHOICE%"=="Q" (
    echo.
    echo [INFO] Installation cancelled by user.
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

:PROCEED_INSTALL
:: --- 6. Copy Plugin to Target Locations ---
set "INSTALLED_COUNT=0"
echo.

if "%USER_CHOICE%"=="0" (
    for /L %%I in (1,1,%TOTAL_FOUND%) do (
        set "TARGET_DIR=!AI_DIR_%%I!\Plug-ins"
        if not exist "!TARGET_DIR!" mkdir "!TARGET_DIR!" >nul 2>&1
        echo [*] Copying to: !AI_NAME_%%I!\Plug-ins\
        if exist "!TARGET_DIR!\MakeCropMarksAI.aip" del /f /q "!TARGET_DIR!\MakeCropMarksAI.aip" >nul 2>&1
        copy /y "%AIP_SRC%" "!TARGET_DIR!\CropMarksAI.aip" >nul
        if errorlevel 1 (
            echo  [-] ERROR: Failed to copy to !AI_NAME_%%I!
        ) else (
            echo  [OK] Successfully installed into: !AI_NAME_%%I!\Plug-ins\CropMarksAI.aip
            set /a INSTALLED_COUNT+=1
        )
    )
) else (
    set "TARGET_DIR=!AI_DIR_%USER_CHOICE%!\Plug-ins"
    set "TARGET_NAME=!AI_NAME_%USER_CHOICE%!"
    if not exist "!TARGET_DIR!" mkdir "!TARGET_DIR!" >nul 2>&1
    echo [*] Copying to: !TARGET_NAME!\Plug-ins\
    if exist "!TARGET_DIR!\MakeCropMarksAI.aip" del /f /q "!TARGET_DIR!\MakeCropMarksAI.aip" >nul 2>&1
    copy /y "%AIP_SRC%" "!TARGET_DIR!\CropMarksAI.aip" >nul
    if errorlevel 1 (
        echo  [-] ERROR: Failed to copy to !TARGET_NAME!
    ) else (
        echo  [OK] Successfully installed into: !TARGET_NAME!\Plug-ins\CropMarksAI.aip
        set /a INSTALLED_COUNT+=1
    )
)

echo.
echo ================================================================================
if %INSTALLED_COUNT% gtr 0 (
    echo                    INSTALLATION COMPLETED SUCCESSFULLY
    echo ================================================================================
    echo.
    echo  CropMarksAI plugin was successfully installed into %INSTALLED_COUNT% version(s).
    echo.
    echo  HOW TO USE IN ADOBE ILLUSTRATOR:
    echo  1. Launch Adobe Illustrator.
    echo  2. Open the top menu:
    echo     Window -^> CropMarksAI       (dockable tool panel)
    echo     Object -^> Quick Apply Crop Marks (Shift+F4)
    echo.
    echo ================================================================================

    powershell -NoProfile -Command "Add-Type -AssemblyName PresentationFramework; [System.Windows.MessageBox]::Show('CropMarksAI plugin has been successfully installed into Adobe Illustrator.' + [Environment]::NewLine + [Environment]::NewLine + 'Restart Illustrator and open:' + [Environment]::NewLine + 'Window -> CropMarksAI', 'CropMarksAI - Installation Complete', 'OK', 'Information')" >nul 2>&1

) else (
    echo                         INSTALLATION FAILED
    echo ================================================================================
    echo  Failed to copy plugin file. Please ensure Illustrator is closed
    echo  and you are running this installer as Administrator.
    echo ================================================================================
)

echo.
if "%AUTO_MODE%"=="1" (
    echo [OK] Auto installation complete.
    timeout /t 2 >nul
    exit /b 0
)
echo Press any key to exit...
pause >nul
exit /b 0
