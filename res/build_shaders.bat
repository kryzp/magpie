@echo off
setlocal enabledelayedexpansion

for %%f in (*.slang) do (
    set "filename=%%~nf"
    slangc "%%f" -profile glsl_450 -target spirv -o "!filename!_vertex.spv" -entry VertexMain
    slangc "%%f" -profile glsl_450 -target spirv -o "!filename!_fragment.spv" -entry FragmentMain
)

echo All shaders compiled!
pause
