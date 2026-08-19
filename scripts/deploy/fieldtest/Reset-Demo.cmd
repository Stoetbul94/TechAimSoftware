@echo off
REM ---------------------------------------------------------------------------
REM  Tech Aim RMS - RESET THE DEMONSTRATION
REM
REM  Throws away the demo profile and starts the demonstration again from its
REM  known initial state: six lanes, six athletes, a fresh plan, no shots.
REM
REM  This touches the DEMO profile only. Your real LIVE range configuration is
REM  in a different place and is not affected.
REM ---------------------------------------------------------------------------
cd /d "%~dp0"
start "" "%~dp0TechAimRMS.exe" --demo-range --reset-demo
