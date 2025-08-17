#!/bin/sh

slangc shader.slang -profile glsl_450 -target spirv -o vertex.spv -entry VertexMain
slangc shader.slang -profile glsl_450 -target spirv -o fragment.spv -entry FragmentMain

