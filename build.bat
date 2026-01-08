@echo off
setlocal enabledelayedexpansion
REM ========================================
REM MSVC Build Script for Karma Project
REM Incremental Compilation Support
REM ========================================

REM --- Setup MSVC Environment ---
echo Setting up MSVC environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to setup MSVC environment
    echo Make sure Visual Studio 2022 is installed
    exit /b 1
)

REM --- Configuration ---
set BUILD_DIR=build
set SRC_DIR=src
set INCLUDE_DIR=include
set LIB_DIR=libs

REM Create build directory
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

echo.
echo ========================================
echo Building Karma Project with MSVC
echo Incremental Build - Only compiling changed files
echo ========================================
echo.

REM --- Compilation Tracking ---
set COMPILE_COUNT=0
set SKIP_COUNT=0

REM --- Check if critical headers changed (force rebuild if they did) ---
set FORCE_REBUILD_GRAPHICS=0

REM Check Shaders.h
if exist "%SRC_DIR%\cpp\graphics\3d\core\Shaders.h" (
    if exist "%BUILD_DIR%\Renderer.obj" (
        for %%S in ("%SRC_DIR%\cpp\graphics\3d\core\Shaders.h") do set SHADERS_TIME=%%~tS
        for %%O in ("%BUILD_DIR%\Renderer.obj") do set RENDERER_TIME=%%~tO
        if "!SHADERS_TIME!" GTR "!RENDERER_TIME!" (
            echo [HEADER CHANGED] Shaders.h modified - forcing graphics rebuild
            del "%BUILD_DIR%\Renderer.obj" "%BUILD_DIR%\ShaderManager.obj" "%BUILD_DIR%\PostProcessing.obj" 2>nul
            set FORCE_REBUILD_GRAPHICS=1
        )
    )
)

REM Check PostProcessing.h
if exist "%SRC_DIR%\cpp\graphics\3d\effects\PostProcessing.h" (
    if exist "%BUILD_DIR%\Renderer.obj" (
        for %%S in ("%SRC_DIR%\cpp\graphics\3d\effects\PostProcessing.h") do set POSTPROC_TIME=%%~tS
        for %%O in ("%BUILD_DIR%\Renderer.obj") do set RENDERER_TIME=%%~tO
        if "!POSTPROC_TIME!" GTR "!RENDERER_TIME!" (
            echo [HEADER CHANGED] PostProcessing.h modified - forcing Renderer rebuild
            del "%BUILD_DIR%\Renderer.obj" "%BUILD_DIR%\PostProcessing.obj" 2>nul
        )
    )
)

REM Check GLTFLoader.h (affects Renderer)
if exist "%SRC_DIR%\cpp\graphics\3d\loaders\GLTFLoader.h" (
    if exist "%BUILD_DIR%\Renderer.obj" (
        for %%S in ("%SRC_DIR%\cpp\graphics\3d\loaders\GLTFLoader.h") do set GLTF_TIME=%%~tS
        for %%O in ("%BUILD_DIR%\Renderer.obj") do set RENDERER_TIME=%%~tO
        if "!GLTF_TIME!" GTR "!RENDERER_TIME!" (
            echo [HEADER CHANGED] GLTFLoader.h modified - forcing rebuild
            del "%BUILD_DIR%\Renderer.obj" "%BUILD_DIR%\GLTFLoader.obj" 2>nul
        )
    )
)

REM --- Compile Assembly ---
call :compile_asm "main.asm" "%SRC_DIR%\asm\main.asm" "%BUILD_DIR%\main.obj"
if errorlevel 1 goto :error

REM --- Compile C/C++ Files ---
call :compile_c_cpp "glad.c" "%SRC_DIR%\cpp\glad.c" "%BUILD_DIR%\glad.obj" "/c /nologo /MT /std:c11 /I%INCLUDE_DIR%"
if errorlevel 1 goto :error

call :compile_c_cpp "Window.cpp" "%SRC_DIR%\cpp\system\Window.cpp" "%BUILD_DIR%\Window.obj" "/c /nologo /EHsc /MT /std:c++20 /I%INCLUDE_DIR%"
if errorlevel 1 goto :error

call :compile_c_cpp "startup.cpp" "%SRC_DIR%\cpp\startup.cpp" "%BUILD_DIR%\startup.obj" "/c /nologo /EHsc /MT /std:c++20 /I%INCLUDE_DIR%"
if errorlevel 1 goto :error

call :compile_c_cpp "VideoPlayer.cpp" "%SRC_DIR%\cpp\multimedia\VideoPlayer.cpp" "%BUILD_DIR%\VideoPlayer.obj" "/c /nologo /EHsc /MT /std:c++20 /I%INCLUDE_DIR%"
if errorlevel 1 goto :error

call :compile_c_cpp "PlayVideo.cpp" "%SRC_DIR%\cpp\multimedia\PlayVideo.cpp" "%BUILD_DIR%\PlayVideo.obj" "/c /nologo /EHsc /MT /std:c++20 /I%INCLUDE_DIR%"
if errorlevel 1 goto :error

call :compile_c_cpp "AudioPlayer.cpp" "%SRC_DIR%\cpp\multimedia\AudioPlayer.cpp" "%BUILD_DIR%\AudioPlayer.obj" "/c /nologo /EHsc /MT /std:c++20 /I%INCLUDE_DIR%"
if errorlevel 1 goto :error

call :compile_c_cpp "PlayAudio.cpp" "%SRC_DIR%\cpp\multimedia\PlayAudio.cpp" "%BUILD_DIR%\PlayAudio.obj" "/c /nologo /EHsc /MT /std:c++20 /I%INCLUDE_DIR%"
if errorlevel 1 goto :error

REM --- Compile Modular Graphics Classes (with multiple includes) ---
set GFX_INC=/I%INCLUDE_DIR% /I%SRC_DIR%\cpp\graphics\3d\core /I%SRC_DIR%\cpp\graphics\3d\scene /I%SRC_DIR%\cpp\graphics\3d\camera /I%SRC_DIR%\cpp\graphics\3d\loaders /I%SRC_DIR%\cpp\graphics\3d\effects

call :compile_c_cpp "ShaderManager.cpp" "%SRC_DIR%\cpp\graphics\3d\core\ShaderManager.cpp" "%BUILD_DIR%\ShaderManager.obj" "/c /nologo /EHsc /MT /std:c++20 !GFX_INC!"
if errorlevel 1 goto :error

call :compile_c_cpp "Camera.cpp" "%SRC_DIR%\cpp\graphics\3d\camera\Camera.cpp" "%BUILD_DIR%\Camera.obj" "/c /nologo /EHsc /MT /std:c++20 !GFX_INC!"
if errorlevel 1 goto :error

call :compile_c_cpp "Material.cpp" "%SRC_DIR%\cpp\graphics\3d\scene\Material.cpp" "%BUILD_DIR%\Material.obj" "/c /nologo /EHsc /MT /std:c++20 !GFX_INC!"
if errorlevel 1 goto :error

call :compile_c_cpp "Light.cpp" "%SRC_DIR%\cpp\graphics\3d\scene\Light.cpp" "%BUILD_DIR%\Light.obj" "/c /nologo /EHsc /MT /std:c++20 !GFX_INC!"
if errorlevel 1 goto :error

call :compile_c_cpp "Mesh.cpp" "%SRC_DIR%\cpp\graphics\3d\scene\Mesh.cpp" "%BUILD_DIR%\Mesh.obj" "/c /nologo /EHsc /MT /std:c++20 !GFX_INC!"
if errorlevel 1 goto :error

call :compile_c_cpp "GLTFLoader.cpp" "%SRC_DIR%\cpp\graphics\3d\loaders\GLTFLoader.cpp" "%BUILD_DIR%\GLTFLoader.obj" "/c /nologo /EHsc /MT /std:c++20 !GFX_INC!"
if errorlevel 1 goto :error

call :compile_c_cpp "PostProcessing.cpp" "%SRC_DIR%\cpp\graphics\3d\effects\PostProcessing.cpp" "%BUILD_DIR%\PostProcessing.obj" "/c /nologo /EHsc /MT /std:c++20 !GFX_INC!"
if errorlevel 1 goto :error

call :compile_c_cpp "InputHandler.cpp" "%SRC_DIR%\cpp\graphics\3d\camera\InputHandler.cpp" "%BUILD_DIR%\InputHandler.obj" "/c /nologo /EHsc /MT /std:c++20 !GFX_INC!"
if errorlevel 1 goto :error

call :compile_c_cpp "Renderer.cpp" "%SRC_DIR%\cpp\graphics\3d\core\Renderer.cpp" "%BUILD_DIR%\Renderer.obj" "/c /nologo /EHsc /MT /std:c++20 !GFX_INC!"
if errorlevel 1 goto :error

call :compile_c_cpp "tiny_gltf.cpp" "%SRC_DIR%\cpp\tiny_gltf.cpp" "%BUILD_DIR%\tiny_gltf.obj" "/c /nologo /EHsc /MT /std:c++20 /I%INCLUDE_DIR%"
if errorlevel 1 goto :error

REM --- Link ---
echo.
echo [LINK] Linking program.exe...
link /NOLOGO ^
     %BUILD_DIR%\main.obj ^
     %BUILD_DIR%\glad.obj ^
     %BUILD_DIR%\Window.obj ^
     %BUILD_DIR%\startup.obj ^
     %BUILD_DIR%\VideoPlayer.obj ^
     %BUILD_DIR%\PlayVideo.obj ^
     %BUILD_DIR%\AudioPlayer.obj ^
     %BUILD_DIR%\PlayAudio.obj ^
     %BUILD_DIR%\ShaderManager.obj ^
     %BUILD_DIR%\Camera.obj ^
     %BUILD_DIR%\Material.obj ^
     %BUILD_DIR%\Light.obj ^
     %BUILD_DIR%\Mesh.obj ^
     %BUILD_DIR%\GLTFLoader.obj ^
     %BUILD_DIR%\PostProcessing.obj ^
     %BUILD_DIR%\InputHandler.obj ^
     %BUILD_DIR%\Renderer.obj ^
     %BUILD_DIR%\tiny_gltf.obj ^
     /LIBPATH:"%LIB_DIR%" ^
     glfw3dll.lib ^
     libvlc.lib ^
     libvlccore.lib ^
     opengl32.lib ^
     gdi32.lib ^
     glu32.lib ^
     user32.lib ^
     shell32.lib ^
     shlwapi.lib ^
     /OUT:%BUILD_DIR%\program.exe
if errorlevel 1 goto :error

REM --- Copy DLLs ---
echo.
echo Copying runtime DLLs...
if exist "%LIB_DIR%\glfw3.dll" (
    copy /Y "%LIB_DIR%\glfw3.dll" "%BUILD_DIR%\" >nul
    echo   - glfw3.dll
)
if exist "%LIB_DIR%\libvlc.dll" (
    copy /Y "%LIB_DIR%\libvlc.dll" "%BUILD_DIR%\" >nul
    echo   - libvlc.dll
)
if exist "%LIB_DIR%\libvlccore.dll" (
    copy /Y "%LIB_DIR%\libvlccore.dll" "%BUILD_DIR%\" >nul
    echo   - libvlccore.dll
)

REM Copy VLC plugins folder if it exists
if exist "%LIB_DIR%\plugins" (
    if not exist "%BUILD_DIR%\plugins" mkdir "%BUILD_DIR%\plugins"
    xcopy /E /I /Y /Q "%LIB_DIR%\plugins" "%BUILD_DIR%\plugins" >nul
    echo   - plugins folder
)

echo.
echo ========================================
echo Build Statistics:
echo   Compiled: %COMPILE_COUNT% files
echo   Skipped:  %SKIP_COUNT% files (up to date)
echo   Total:    19 files
echo ========================================
echo Build completed successfully!
echo Executable: %BUILD_DIR%\program.exe
echo ========================================
goto :end

:error
echo.
echo ========================================
echo BUILD FAILED!
echo ========================================
exit /b 1

REM ========================================
REM Helper Function: Compile Assembly
REM ========================================
:compile_asm
set FILE_NAME=%~1
set SOURCE_PATH=%~2
set OBJ_PATH=%~3

REM Check if object exists and is up to date
if exist "%OBJ_PATH%" (
    for %%S in ("%SOURCE_PATH%") do set SOURCE_TIME=%%~tS
    for %%O in ("%OBJ_PATH%") do set OBJ_TIME=%%~tO

    REM Compare timestamps
    if "!OBJ_TIME!" GEQ "!SOURCE_TIME!" (
        echo [SKIP] %FILE_NAME% ^(up to date^)
        set /a SKIP_COUNT+=1
        exit /b 0
    )
)

REM Compile the assembly file
echo [COMPILE] %FILE_NAME%
"C:\Program Files\NASM\nasm.exe" -f win64 "%SOURCE_PATH%" -o "%OBJ_PATH%"
if errorlevel 1 exit /b 1
set /a COMPILE_COUNT+=1
exit /b 0

REM ========================================
REM Helper Function: Compile C/C++
REM ========================================
:compile_c_cpp
set FILE_NAME=%~1
set SOURCE_PATH=%~2
set OBJ_PATH=%~3
set COMPILE_FLAGS=%~4

REM Check if object exists and is up to date
if exist "%OBJ_PATH%" (
    for %%S in ("%SOURCE_PATH%") do set SOURCE_TIME=%%~tS
    for %%O in ("%OBJ_PATH%") do set OBJ_TIME=%%~tO

    REM Compare timestamps
    if "!OBJ_TIME!" GEQ "!SOURCE_TIME!" (
        echo [SKIP] %FILE_NAME% ^(up to date^)
        set /a SKIP_COUNT+=1
        exit /b 0
    )
)

REM Compile the file
echo [COMPILE] %FILE_NAME%
cl %COMPILE_FLAGS% "%SOURCE_PATH%" /Fo"%OBJ_PATH%"
if errorlevel 1 exit /b 1
set /a COMPILE_COUNT+=1
exit /b 0

:end
endlocal
