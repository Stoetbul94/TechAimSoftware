@echo off
REM ---------------------------------------------------------------------------
REM  Tech Aim RMS - range launcher
REM
REM  Runs the RMS window from the development build on THIS laptop. It puts the
REM  Qt runtime on PATH rather than copying it next to the executable, so it
REM  works only on a machine that has Qt 6.5.3 MinGW installed. That is fine for
REM  a range test on the build laptop; it is NOT a distributable package.
REM
REM  RMS is LISTEN-ONLY. It receives on UDP 7755 and cannot command a target.
REM ---------------------------------------------------------------------------
setlocal

set "QTBIN=C:\Qt\6.5.3\mingw_64\bin"
set "MINGWBIN=C:\Qt\Tools\mingw1120_64\bin"

if not exist "%QTBIN%\Qt6Core.dll" (
  echo.
  echo   Qt 6.5.3 MinGW was not found at %QTBIN%
  echo   RMS cannot start without it. Nothing has been changed.
  echo.
  pause
  exit /b 1
)

set "PATH=%QTBIN%;%MINGWBIN%;%PATH%"
set "QT_FORCE_STDERR_LOGGING=1"

echo Starting Tech Aim RMS - listening on UDP 7755, receive only.
echo Close this window to stop RMS.
echo.

"%~dp0rms\release\TechAimRMS.exe" %*

endlocal
