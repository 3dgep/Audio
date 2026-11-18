@ECHO OFF
SETLOCAL

REM ****************************************************************************
REM GenerateProjectFiles.bat
REM
REM Generates a Visual Studio solution for the Audio library and the examples with
REM CMake, and opens it:
REM
REM     out\build\vs18-static\Audio.slnx   (Visual Studio 2026)
REM     out\build\vs17-static\Audio.sln    (Visual Studio 2022)
REM
REM Usage:
REM     Double-click this file, or run GenerateProjectFiles.bat from a command prompt.
REM     It can be run from any folder. Run it again after adding or removing source
REM     files; the solution is updated.
REM
REM Requirements:
REM     Visual Studio 2022 or newer with the "Desktop development with C++"
REM     workload. If both Visual Studio 2022 and 2026 are installed, Visual Studio
REM     2026 is used.
REM
REM     CMake 4.0 or newer (needed for the examples). CMake is used from the PATH
REM     if it is installed, otherwise the CMake that comes with Visual Studio.
REM     Visual Studio 2026 comes with CMake 4; Visual Studio 2022 comes with an
REM     older version, so install CMake from https://cmake.org/download/ if you
REM     use Visual Studio 2022.
REM ****************************************************************************

REM Run from the folder that contains this script (the root of the repository),
REM because the CMake presets and output folders are relative to it.
PUSHD "%~dp0"

SET VSWHERE="%~dp0tools\vswhere\vswhere.exe"

REM ----------------------------------------------------------------------------
REM Find the newest Visual Studio with the C++ build tools: its installation
REM folder (VS_PATH) and major version (VS_VERSION: 17, 18, ...).
REM ----------------------------------------------------------------------------

SET VS_PATH=
SET VS_VERSION=
FOR /F "usebackq delims=" %%i IN (`%VSWHERE% -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) DO (
    SET "VS_PATH=%%i"
)
FOR /F "usebackq delims=." %%i IN (`%VSWHERE% -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion`) DO (
    SET VS_VERSION=%%i
)

REM Choose the CMake preset (from CMakePresets.json) for this version.
IF "%VS_VERSION%" == "18" (
    SET CMAKE_PRESET=vs18-static
    SET SOLUTION_FILE=Audio.slnx
) ELSE IF "%VS_VERSION%" == "17" (
    SET CMAKE_PRESET=vs17-static
    SET SOLUTION_FILE=Audio.sln
) ELSE (
    ECHO.
    ECHO ***********************************************************************
    ECHO *                                                                     *
    ECHO *                                ERROR                                *
    ECHO *                                                                     *
    ECHO ***********************************************************************
    ECHO No compatible version of Microsoft Visual Studio detected.
    ECHO Please make sure you have Visual Studio 2022 ^(or newer^) with the
    ECHO "Desktop development with C++" workload installed before running this script.
    ECHO Go to https://visualstudio.microsoft.com/downloads/ to download the latest version of Visual Studio.
    ECHO.
    GOTO :Error
)
SET CMAKE_BINARY_DIR=out\build\%CMAKE_PRESET%

REM ----------------------------------------------------------------------------
REM Find CMake: use the one on the PATH, or else the one that comes with
REM Visual Studio.
REM ----------------------------------------------------------------------------

SET CMAKE=cmake
WHERE /Q cmake
IF ERRORLEVEL 1 (
    SET CMAKE="%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)

REM ----------------------------------------------------------------------------
REM Generate the solution (with the examples) and open it.
REM ----------------------------------------------------------------------------

%CMAKE% --preset %CMAKE_PRESET% -DAUDIO_BUILD_EXAMPLES=ON -Wno-dev
IF ERRORLEVEL 1 (
    ECHO.
    ECHO Generating the solution failed. If CMake reports that CMake 4.0 or higher is
    ECHO required, install the latest CMake from https://cmake.org/download/
    GOTO :Error
)

REM The empty "" is the window title. Without it, START would use the quoted
REM path as the title instead of opening it.
START "" "%CMAKE_BINARY_DIR%\%SOLUTION_FILE%"

POPD
EXIT /B 0

:Error
REM Keep the window open when the script was started by double-clicking it.
PAUSE
POPD
EXIT /B 1
