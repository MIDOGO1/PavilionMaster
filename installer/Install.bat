@echo off
setlocal EnableExtensions
title Pavilion Master - Installer

set "APP_DIR=%ProgramFiles%\Pavilion Master"
set "SRC=%~dp0app"
set "SELF=%~f0"

:: ---- 1. Make sure we are running as administrator -------------------------
net session >nul 2>&1
if errorlevel 1 (
    echo Requesting administrator permission...
    powershell -NoProfile -Command "Start-Process -FilePath $env:SELF -Verb RunAs"
    if errorlevel 1 (
        echo Administrator permission was not granted. Nothing was installed.
        pause
    )
    exit /b
)

:: ---- 2. Sanity checks ------------------------------------------------------
if not exist "%SRC%\PavilionMaster.exe" (
    echo.
    echo ERROR: The "app" folder was not found next to Install.bat.
    echo Extract the WHOLE zip file first, then run Install.bat from the extracted folder.
    echo.
    pause
    exit /b 1
)

echo.
echo  Pavilion Master installer
echo  -------------------------
echo  Install location: %APP_DIR%
echo.

:: ---- 3. Close any running copy and release the driver ----------------------
echo Closing any running copy...
taskkill /f /im PavilionMaster.exe >nul 2>&1
taskkill /f /im PavilionMasterCore.exe >nul 2>&1
taskkill /f /im ryzenadj.exe >nul 2>&1
taskkill /f /im ryzenadj-real.exe >nul 2>&1
sc stop WinRing0_1_2_0 >nul 2>&1
timeout /t 2 /nobreak >nul

:: ---- 4. Copy files ---------------------------------------------------------
echo Copying files...
robocopy "%SRC%" "%APP_DIR%" /E /R:2 /W:1 /NFL /NDL /NJH /NJS /NP >nul
if errorlevel 8 (
    echo.
    echo ERROR: Could not copy the files to "%APP_DIR%".
    echo Close anything that might be using that folder and try again.
    pause
    exit /b 1
)
copy /y "%~dp0Uninstall.bat" "%APP_DIR%\Uninstall.bat" >nul

:: ---- 5. Shortcuts (Start menu + desktop, all users) ------------------------
echo Creating shortcuts...
set "PM_DIR=%APP_DIR%"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$w=New-Object -ComObject WScript.Shell; $t=$env:PM_DIR+'\PavilionMaster.exe'; foreach($p in @(($env:ProgramData+'\Microsoft\Windows\Start Menu\Programs\Pavilion Master.lnk'),($env:PUBLIC+'\Desktop\Pavilion Master.lnk'))){ $l=$w.CreateShortcut($p); $l.TargetPath=$t; $l.WorkingDirectory=$env:PM_DIR; $l.IconLocation=$t+',0'; $l.Description='Pavilion Master'; $l.Save() }"

:: ---- 6. Always run as administrator + "Apps & features" entry --------------
reg add "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers" /v "%APP_DIR%\PavilionMaster.exe" /t REG_SZ /d "~ RUNASADMIN" /f >nul
reg add "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers" /v "%APP_DIR%\PavilionMasterCore.exe" /t REG_SZ /d "~ RUNASADMIN" /f >nul

set "VER="
if exist "%~dp0version.txt" set /p VER=<"%~dp0version.txt"
set "UKEY=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\PavilionMaster"
reg add "%UKEY%" /v DisplayName /t REG_SZ /d "Pavilion Master" /f >nul
reg add "%UKEY%" /v DisplayVersion /t REG_SZ /d "%VER%" /f >nul
reg add "%UKEY%" /v Publisher /t REG_SZ /d "MIDO5659" /f >nul
reg add "%UKEY%" /v InstallLocation /t REG_SZ /d "%APP_DIR%" /f >nul
reg add "%UKEY%" /v DisplayIcon /t REG_SZ /d "%APP_DIR%\PavilionMaster.exe" /f >nul
reg add "%UKEY%" /v UninstallString /t REG_SZ /d "\"%APP_DIR%\Uninstall.bat\"" /f >nul
reg add "%UKEY%" /v NoModify /t REG_DWORD /d 1 /f >nul
reg add "%UKEY%" /v NoRepair /t REG_DWORD /d 1 /f >nul

:: ---- 7. Done ---------------------------------------------------------------
echo.
echo Installed successfully. Starting Pavilion Master...
start "" "%APP_DIR%\PavilionMaster.exe"
echo.
echo To remove it later: Settings ^> Apps ^> Pavilion Master ^> Uninstall,
echo or run Uninstall.bat from "%APP_DIR%".
echo.
pause
exit /b 0
