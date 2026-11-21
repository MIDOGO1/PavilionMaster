@echo off
setlocal EnableExtensions
title Pavilion Master - Uninstaller

set "APP_DIR=%ProgramFiles%\Pavilion Master"
set "SELF=%~f0"

:: ---- 1. Make sure we are running as administrator -------------------------
net session >nul 2>&1
if errorlevel 1 (
    echo Requesting administrator permission...
    powershell -NoProfile -Command "Start-Process -FilePath $env:SELF -Verb RunAs"
    if errorlevel 1 (
        echo Administrator permission was not granted. Nothing was removed.
        pause
    )
    exit /b
)

:: ---- 2. Confirm (only on the first run, not on the temp copy below) --------
if /i "%~dp0"=="%APP_DIR%\" (
    echo.
    echo This will remove Pavilion Master, its saved settings and the WinRing0 driver service.
    choice /c YN /m "Continue"
    if errorlevel 2 exit /b 0
    :: Run from a temp copy so this file can delete its own folder.
    copy /y "%~f0" "%TEMP%\PavilionMaster-Uninstall.bat" >nul
    call "%TEMP%\PavilionMaster-Uninstall.bat"
    exit /b
)

echo.
echo Removing Pavilion Master...

:: ---- 3. Stop the app and the driver ----------------------------------------
taskkill /f /im PavilionMaster.exe >nul 2>&1
taskkill /f /im PavilionMasterCore.exe >nul 2>&1
taskkill /f /im ryzenadj.exe >nul 2>&1
taskkill /f /im ryzenadj-real.exe >nul 2>&1
sc stop WinRing0_1_2_0 >nul 2>&1
timeout /t 2 /nobreak >nul
sc delete WinRing0_1_2_0 >nul 2>&1

:: ---- 4. Remove the power plan the app created ------------------------------
powercfg /getactivescheme | find /i "5a7c1e2b-9d34-4f6a-8b1c-0e7d2f4a6b93" >nul 2>&1
if not errorlevel 1 powercfg /setactive 381b4222-f694-41f0-9685-ff5bb260df2e >nul 2>&1
powercfg /delete 5a7c1e2b-9d34-4f6a-8b1c-0e7d2f4a6b93 >nul 2>&1

:: ---- 5. Shortcuts and registry ---------------------------------------------
del /q "%ProgramData%\Microsoft\Windows\Start Menu\Programs\Pavilion Master.lnk" >nul 2>&1
del /q "%PUBLIC%\Desktop\Pavilion Master.lnk" >nul 2>&1
del /q "%USERPROFILE%\Desktop\Pavilion Master.lnk" >nul 2>&1
reg delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\PavilionMaster" /f >nul 2>&1
reg delete "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers" /v "%APP_DIR%\PavilionMaster.exe" /f >nul 2>&1
reg delete "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers" /v "%APP_DIR%\PavilionMasterCore.exe" /f >nul 2>&1
reg delete "HKCU\Software\MIDO5659\GControlCenter" /f >nul 2>&1

:: ---- 6. Delete the install folder ------------------------------------------
cd /d "%TEMP%"
rmdir /s /q "%APP_DIR%" >nul 2>&1
if exist "%APP_DIR%" (
    echo.
    echo Some files in "%APP_DIR%" could not be deleted ^(still in use^).
    echo Restart the PC and delete that folder manually.
) else (
    echo.
    echo Pavilion Master was removed.
)
echo.
pause
:: Delete this temporary copy of the script.
(goto) 2>nul & del "%~f0"
