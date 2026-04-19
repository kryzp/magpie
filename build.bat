@echo off

setlocal

:: initialize the visual studio environment
call "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

set opts=-FC -GR- -EHa- -nologo -Zi -Od -W4
set code=%cd%\src
set libs=/LIBPATH:"D:\DevLibs\assimp\lib\Release" /LIBPATH:"D:\DevLibs\SDL3\VisualC\x64\Release"
set includes=/I"%code%" /I"D:\VulkanSDK\1.4.341.1\Include" /I"D:\DevLibs\assimp\include" /I"D:\DevLibs\SDL3\include"
set supwarn=/wd4201 /wd4018 /wd4996 /wd4100 /wd4244

if not exist build mkdir build
pushd build

:: compile vulkan memory allocator separately
if not exist vk_mem_alloc.obj (
    echo Compiling Vulkan Memory Allocator...
    cl %opts% %includes% /std:c++20 /c "%code%\ext\vk_mem_alloc.cpp" /Fo:vk_mem_alloc.obj
)

:: compile app (platform-agnostic) code
echo Compiling app...
cl %opts% %supwarn% %includes% /TC /c "%code%\app.c" /Fo:app.obj

:: link app into DLL
echo Linking app...
link /DLL /DEBUG /OUT:app.dll app.obj vk_mem_alloc.obj %libs% assimp-vc143-mt.lib

:: compile platform-dependent executable if not running
tasklist /FI "IMAGENAME eq magpie_win32.exe" | find /I "magpie_win32.exe" >nul

if errorlevel 1 (
    echo Compiling platform layer...
    cl %opts% %supwarn% %includes% /TC "%code%\os\win32\win32_main.c" /Fe:magpie_win32 /link %libs% SDL3.lib shell32.lib
)

popd

echo Finished!

endlocal
