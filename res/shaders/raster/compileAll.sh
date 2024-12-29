#!/bin/sh

rm *.spv

for file in *.frag; do
  glslc -fshader-stage=fragment "$file" -o "${file%.*}.spv"
done

for file in *.vert; do
  glslc -fshader-stage=vertex "$file" -o "${file%.*}.spv"
done
