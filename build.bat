@echo off

setlocal

:: initialize the visual studio environment
call "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

set opts=-FC -GR- -EHa- -nologo -Zi -Od -W4
set code=%cd%\src
set libs=/LIBPATH:"D:\VulkanSDK\1.4.341.1\Lib" /LIBPATH:"D:\DevLibs\SDL3\VisualC\x64\Release"
set includes=/I"%code%" /I"D:\VulkanSDK\1.4.341.1\Include" /I"D:\DevLibs\SDL3\include"
set supwarn=/wd4201 /wd4018 /wd4996 /wd4100 /wd4244

if not exist build mkdir build
pushd build

:: compile vulkan memory allocator separately
if not exist vk_mem_alloc.obj (
    echo Compiling Vulkan Memory Allocator...
    cl %opts% %includes% /std:c++20 /c "%code%\ext\vma\vk_mem_alloc.cpp" /Fo:vk_mem_alloc.obj
	
	if errorlevel 1 (
		echo.
		echo Vulkan Memory Allocator compilation failed.
		popd
		exit /b 1
	)
)

:: compile the slang compiler separately
if not exist slang_compiler.obj (
    echo Compiling Slang Compiler...
    cl %opts% %includes% /std:c++20 /c "%code%\ext\slang\slang_compiler.cpp" /Fo:slang_compiler.obj
	
	if errorlevel 1 (
		echo.
		echo Slang compiler compilation failed.
		popd
		exit /b 1
	)
)

:: compile app (platform agnostic code)
echo Compiling app...
cl %opts% %supwarn% %includes% /TC /c "%code%\app.c" /Fo:app.obj

if errorlevel 1 (
    echo.
    echo App compilation failed.
    popd
    exit /b 1
)

:: link app into DLL
echo Linking app...
link /DLL /DEBUG /OUT:app.dll app.obj vk_mem_alloc.obj slang_compiler.obj %libs% slang.lib

if errorlevel 1 (
    echo.
    echo Linking failed.
    popd
    exit /b 1
)

:: compile platform launch executable if not running
tasklist /FI "IMAGENAME eq magpie_win32.exe" | find /I "magpie_win32.exe" >nul

if errorlevel 1 (
    echo Compiling platform layer...
    cl %opts% %supwarn% %includes% /TC "%code%\os\win32\win32_main.c" /Fe:magpie_win32 /link %libs% SDL3.lib shell32.lib
        
    if errorlevel 1 (
        echo.
        echo Platform compilation failed.
        popd
        exit /b 1
    )
)

popd

echo Finished!

if exist .\build\magpie_win32.exe (
    echo Running...
    .\build\magpie_win32.exe
)

endlocal
