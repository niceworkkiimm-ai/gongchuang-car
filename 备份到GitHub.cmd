@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\backup.ps1"
if errorlevel 1 (
  echo Backup did not finish. Review the message above; local files are retained.
) else (
  echo Backup completed.
)
pause
