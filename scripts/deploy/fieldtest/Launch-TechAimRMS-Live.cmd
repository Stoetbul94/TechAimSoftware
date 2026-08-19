@echo off
REM ---------------------------------------------------------------------------
REM  Tech Aim RMS - LIVE
REM
REM  Listens for real target stations broadcasting on UDP 7755.
REM
REM  RMS RECEIVES ONLY. It cannot start, stop, reset or otherwise control a
REM  target, and closing it does not affect any match in progress.
REM
REM  If no station is broadcasting, the range will correctly show nothing.
REM  That is not a fault - it means nothing was heard.
REM ---------------------------------------------------------------------------
cd /d "%~dp0"
start "" "%~dp0TechAimRMS.exe" --live
