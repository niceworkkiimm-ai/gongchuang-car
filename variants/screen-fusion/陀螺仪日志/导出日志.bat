@echo off
setlocal
chcp 65001 >nul
set "GYRO_INPUT=%~1"
if not defined GYRO_INPUT set "GYRO_INPUT=%~dp0..\PRJ\gyro_log.hex"
python -X utf8 "%~dp0gyro_log_decode.py" "%GYRO_INPUT%" --output "%~dp0gyro_log.csv"
pause
