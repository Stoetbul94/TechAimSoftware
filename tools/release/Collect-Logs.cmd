@echo off
REM Tech Aim RC3B-DIAG - collect the range evidence.
REM
REM Double-clickable on purpose. The 2026-08-23 investigation could not explain
REM part of what happened on one tablet because the logs for the failure window
REM were never collected: they live in %TEMP%, and %TEMP% gets cleaned. An
REM operator at a range with a rifle in one hand will not type a PowerShell
REM command line, so this is the command line.
REM
REM Writes TechAim-Support-<date>.zip to the Desktop. No passwords, no personal
REM files, no other athlete's sessions - session data is opt-in and is NOT
REM included here. To include one, run instead:
REM     powershell -ExecutionPolicy Bypass -File Make-SupportBundle.ps1 -SessionId <id>

setlocal
cd /d "%~dp0"

if not exist "Make-SupportBundle.ps1" (
    echo.
    echo   Make-SupportBundle.ps1 is missing from this folder.
    echo   Collect %%TEMP%%\tachus_log*.log by hand and report that this file was absent.
    echo.
    pause
    exit /b 1
)

echo.
echo   Collecting Tech Aim logs and configuration...
echo.
powershell -ExecutionPolicy Bypass -NoProfile -File "Make-SupportBundle.ps1" %*
set RC=%ERRORLEVEL%

echo.
if "%RC%"=="0" (
    echo   Done. The bundle is on your Desktop.
    echo   Collect one after EVERY test, pass or fail.
) else (
    echo   Collection FAILED with code %RC%.
    echo   Copy %%TEMP%%\tachus_log*.log by hand before shutting the tablet down.
)
echo.
pause
exit /b %RC%
