@echo off
setlocal
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
set "OUT=%TASK_CODE%\Build\ai_astar_20261031\containers"
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /DDEDICATED=1 /DBF2_IMAGE_AIDLL /I"%TASK_CODE%\BF2\AI\AIDLL\AI" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0astar_containers_runtime.cpp" /Fo"%OUT%\fixture.obj" >"%OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /SUBSYSTEM:CONSOLE /OUT:"%OUT%\astar_containers_runtime.exe" "%OUT%\fixture.obj" >"%OUT%\link.log" 2>&1
if errorlevel 1 goto failed
"%OUT%\astar_containers_runtime.exe"
exit /b %errorlevel%
:failed
type "%OUT%\compile.log"
if exist "%OUT%\link.log" type "%OUT%\link.log"
exit /b 1
