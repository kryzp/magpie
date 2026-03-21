@echo off

setlocal enabledelayedexpansion

set SHADER_DIR=.

for %%f in (%SHADER_DIR%\passes\*.comp.slang) do (
    set "name=%%~nf"
    echo "Compiling %%f..."

    slangc %%f -I %SHADER_DIR% ^
        -profile glsl_450 -target spirv ^
        -entry ComputeMain ^
        -o "%%~dpnf.spv"
)

for %%f in (%SHADER_DIR%\passes\*.slang) do (
    set "name=%%~nf"
    echo "Compiling %%f (vertex + fragment)..."

    slangc %%f -I %SHADER_DIR% ^
        -profile glsl_450 -target spirv ^
        -entry VertexMain ^
        -o "%%~dpnf.vert.spv"

    slangc %%f -I %SHADER_DIR% ^
        -profile glsl_450 -target spirv ^
        -entry FragmentMain ^
        -o "%%~dpnf.frag.spv"
)

echo Done!

pause
