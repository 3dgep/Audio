@ECHO OFF

REM ****************************************************************************
REM MIT License
REM 
REM Copyright (c) 2023 Jeremiah van Oosten
REM 
REM Permission is hereby granted, free of charge, to any person obtaining a copy
REM of this software and associated documentation files (the "Software"), to deal
REM in the Software without restriction, including without limitation the rights
REM to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
REM copies of the Software, and to permit persons to whom the Software is
REM furnished to do so, subject to the following conditions:
REM 
REM The above copyright notice and this permission notice shall be included in all
REM copies or substantial portions of the Software.
REM 
REM THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
REM IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
REM FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
REM AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
REM LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
REM OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
REM SOFTWARE.
REM ****************************************************************************

REM This script will generate the solution files for the currently installed version
REM of Visual Studio detected on the user's computer. Supported version of Visual Studio are:
REM     Visual Studio 2026
REM     Visual Studio 2022
REM
REM The solution files (including the example projects) will be created in
REM out\build\vs18-static if Visual Studio 2026 was detected or in
REM out\build\vs17-static if Visual Studio 2022 was detected.

PUSHD "%~dp0"

SET VSWHERE="%~dp0\tools\vswhere\vswhere.exe"
SET CMAKE="cmake"

REM Detect latest version of Visual Studio.
FOR /F "usebackq delims=." %%i IN (`%VSWHERE% -latest -prerelease -requires Microsoft.VisualStudio.Workload.NativeGame -property installationVersion`) DO (
    SET VS_VERSION=%%i
)

IF %VS_VERSION% == 18 (
    SET CMAKE_PRESET="vs18-static"
    SET CMAKE_BINARY_DIR=out\build\vs18-static
    SET SOLUTION_FILE=Audio.slnx
) ELSE IF %VS_VERSION% == 17 (
    SET CMAKE_PRESET="vs17-static"
    SET CMAKE_BINARY_DIR=out\build\vs17-static
    SET SOLUTION_FILE=Audio.sln
) ELSE (
    ECHO.
    ECHO ***********************************************************************
    ECHO *                                                                     *
    ECHO *                                ERROR                                *
    ECHO *                                                                     *
    ECHO ***********************************************************************
    ECHO No compatible version of Microsoft Visual Studio detected.
    ECHO Please make sure you have Visual Studio 2022 ^(or newer^) and the
    ECHO "Game Development with C++" workload installed before running this script.
    ECHO Go to https://www.visualstudio.com/downloads/ to download the latest version of Visual Studio.
    ECHO. 
    PAUSE
    GOTO :Exit
)

:GenerateProjectFiles
%CMAKE% --preset %CMAKE_PRESET% -DAUDIO_BUILD_EXAMPLES=ON -Wno-dev

IF %ERRORLEVEL% NEQ 0 (
    PAUSE
) ELSE (
    START %CMAKE_BINARY_DIR%\%SOLUTION_FILE%
)

:Exit

POPD
