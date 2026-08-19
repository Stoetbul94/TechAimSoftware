@echo off
REM ---------------------------------------------------------------------------
REM  Tech Aim RMS - FIELD-TEST DEMONSTRATION
REM
REM  Starts RMS with a scripted demonstration range. No hardware is needed and
REM  nothing on screen is real: every station, athlete, shot and score is
REM  generated inside this program.
REM
REM  It writes to its OWN demo profile. The range you configure in LIVE mode is
REM  not opened, not read and not changed by this.
REM ---------------------------------------------------------------------------
cd /d "%~dp0"
start "" "%~dp0TechAimRMS.exe" --demo-range
