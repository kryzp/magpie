@echo off
setlocal enabledelayedexpansion

for %%f in (*.slang) do (
  set "filename=%%~nf"
  set "ending=!filename:~-5!"
  echo !filename!
  if /i "!ending!"==".comp" (
    slangc "%%f" -profile glsl_450 -target spirv -o "!filename!.spv" -entry ComputeMain
  ) else (
    slangc "%%f" -profile glsl_450 -target spirv -o "!filename!.vert.spv" -entry VertexMain
    slangc "%%f" -profile glsl_450 -target spirv -o "!filename!.frag.spv" -entry FragmentMain
  )
)

echo All shaders compiled!
pause
