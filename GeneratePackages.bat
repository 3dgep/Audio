@ECHO OFF
SETLOCAL

REM ****************************************************************************
REM GeneratePackages.bat
REM
REM Builds the prebuilt packages that are published on the GitHub Releases page
REM (the release workflow in .github\workflows\release.yml makes the same ones):
REM
REM     out\packages\Audio-<version>-windows-x64-static.zip   (Audio.lib)
REM     out\packages\Audio-<version>-windows-x64-shared.zip   (Audio.dll + Audio.lib)
REM
REM Each package contains the headers, the Debug and the Release library, and the
REM CMake files for find_package( Audio ).
REM
REM Usage:
REM     Double-click this file, or run GeneratePackages.bat from a command prompt.
REM     It can be run from any folder.
REM
REM Requirements:
REM     Visual Studio 2022 or newer with the "Desktop development with C++"
REM     workload. CMake is used from the PATH if it is installed, otherwise the
REM     CMake that comes with Visual Studio.
REM
REM Which Visual Studio is used:
REM     Visual Studio can't link a static library that was built with a newer
REM     version of its compiler (toolset). To make packages that work with Visual
REM     Studio 2022 and newer, they are built with the Visual Studio 2022 toolset
REM     (v143) when it is available:
REM       1. Visual Studio 2022 is installed: use the vs17 presets.
REM       2. Visual Studio 2026 with the v143 build tools installed: use the vs18
REM          presets with the v143 toolset (-T v143).
REM       3. Otherwise, use the vs18 presets with the Visual Studio 2026 toolset and
REM          show a warning: the static package then requires Visual Studio 2026.
REM ****************************************************************************

REM Run from the folder that contains this script (the root of the repository),
REM because the CMake presets and output folders are relative to it.
PUSHD "%~dp0"

SET VSWHERE="%~dp0tools\vswhere\vswhere.exe"
SET PACKAGE_DIR=out\packages

REM ----------------------------------------------------------------------------
REM Find Visual Studio.
REM ----------------------------------------------------------------------------

REM VS2022_PATH: the installation folder of Visual Studio 2022 (version 17.x),
REM if it is installed with the C++ build tools.
SET VS2022_PATH=
FOR /F "usebackq delims=" %%i IN (`%VSWHERE% -version [17.0^,18.0^) -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) DO (
    SET "VS2022_PATH=%%i"
)

REM VS_PATH and VS_VERSION: the installation folder and major version (17, 18, ...)
REM of the newest Visual Studio with the C++ build tools.
SET VS_PATH=
SET VS_VERSION=
FOR /F "usebackq delims=" %%i IN (`%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) DO (
    SET "VS_PATH=%%i"
)
FOR /F "usebackq delims=." %%i IN (`%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion`) DO (
    SET VS_VERSION=%%i
)

IF "%VS_PATH%" == "" (
    ECHO No Visual Studio installation with the C++ build tools was found.
    ECHO Please install Visual Studio 2022 or newer with the "Desktop development with C++"
    ECHO workload.
    GOTO :Error
)

REM ----------------------------------------------------------------------------
REM Find CMake: use the one on the PATH, or else the one that comes with
REM Visual Studio.
REM ----------------------------------------------------------------------------

SET CMAKE=cmake
SET CPACK=cpack
WHERE /Q cmake
IF ERRORLEVEL 1 (
    SET CMAKE="%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    SET CPACK="%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cpack.exe"
)

REM ----------------------------------------------------------------------------
REM Choose the CMake presets (vs17 or vs18) and the toolset. See "Which Visual
REM Studio is used" at the top of this file.
REM ----------------------------------------------------------------------------

SET TOOLSET_ARGS=
IF NOT "%VS2022_PATH%" == "" (
    SET PRESET=vs17
    ECHO Building the packages with Visual Studio 2022.
) ELSE IF "%VS_VERSION%" == "18" (
    SET PRESET=vs18
    REM Visual Studio 2026 lists the toolsets it can use in this folder. The v143
    REM toolset is there when the "MSVC v143" build tools component is installed.
    IF EXIST "%VS_PATH%\MSBuild\Microsoft\VC\v180\Platforms\x64\PlatformToolsets\v143" (
        SET TOOLSET_ARGS=-T v143
        ECHO Building the packages with Visual Studio 2026 and the v143 ^(Visual Studio 2022^) toolset.
    ) ELSE (
        ECHO.
        ECHO WARNING: Visual Studio 2022 and the v143 build tools were not found.
        ECHO          The packages are built with the Visual Studio 2026 toolset, so the static
        ECHO          package can only be used with Visual Studio 2026 or newer.
        ECHO          To fix this, install "MSVC v143 - VS 2022 C++ x64/x86 build tools" with the
        ECHO          Visual Studio Installer ^(Individual components^).
        ECHO.
    )
) ELSE (
    REM There are no presets for this version of Visual Studio yet. Add them to
    REM CMakePresets.json and to the checks above.
    ECHO Unsupported Visual Studio version: %VS_VERSION%
    GOTO :Error
)

REM ----------------------------------------------------------------------------
REM Build both packages and collect them in out\packages.
REM ----------------------------------------------------------------------------

REM Start with an empty folder, so it only contains the packages of this build.
IF EXIST %PACKAGE_DIR% RMDIR /S /Q %PACKAGE_DIR%
MKDIR %PACKAGE_DIR%

CALL :Package static || GOTO :Error
CALL :Package shared || GOTO :Error

ECHO.
ECHO === All packages created successfully in %PACKAGE_DIR% ===
DIR /B %PACKAGE_DIR%\*.zip
POPD
EXIT /B 0

:Error
ECHO.
ECHO === Creating the packages failed ===
POPD
EXIT /B 1

REM ----------------------------------------------------------------------------
REM :Package <static|shared>
REM
REM Configures, builds (Debug and Release) and packages one variant with the
REM <preset>-<variant> presets from CMakePresets.json, then copies the package to
REM out\packages. Returns 1 if any step fails.
REM ----------------------------------------------------------------------------
:Package
SET VARIANT=%PRESET%-%1

ECHO.
ECHO === Configuring %VARIANT% ===
REM --fresh: start from a clean CMake cache, because CMake can't change the
REM toolset of an existing build folder.
%CMAKE% --preset %VARIANT% --fresh %TOOLSET_ARGS%
IF ERRORLEVEL 1 EXIT /B 1

ECHO.
ECHO === Building %VARIANT% Debug ===
%CMAKE% --build --preset %VARIANT%-debug
IF ERRORLEVEL 1 EXIT /B 1

ECHO.
ECHO === Building %VARIANT% Release ===
%CMAKE% --build --preset %VARIANT%-release
IF ERRORLEVEL 1 EXIT /B 1

ECHO.
ECHO === Packaging %VARIANT% ===
REM Remove packages from earlier builds (which may have had a different name),
REM so only the new package is copied to out\packages.
IF EXIST out\build\%VARIANT%\*.zip DEL /Q out\build\%VARIANT%\*.zip
REM The package presets include both the Debug and the Release configuration.
%CPACK% --preset %VARIANT%-zip
IF ERRORLEVEL 1 EXIT /B 1

COPY /Y out\build\%VARIANT%\*.zip %PACKAGE_DIR% > NUL
IF ERRORLEVEL 1 EXIT /B 1
EXIT /B 0
