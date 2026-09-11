@echo off
title AEGIS-SEAS Public Mission Control Tunnel
echo ===================================================================
echo   AEGIS-SEAS: Launching Public HTTPS Mission Control Tunnel
echo   Forwarding traffic to local C++20 engine on port 8080...
echo ===================================================================
echo.
echo Local LAN URL: http://192.168.1.8:8080
echo.
echo Launching live HTTPS public link...
ssh -p 443 -R0:localhost:8080 -o StrictHostKeyChecking=no a.pinggy.io
pause
