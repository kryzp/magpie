#!/bin/bash

for f in *.slang; do
    filename="${f%.*}" # filename w/o extension
    ending="${filename: -5}" # last 5 characters
    
    echo "$filename"
    
    if [[ "$ending" == ".comp" ]]; then
        slangc "$f" -Wno-39001 -profile glsl_450 -target spirv -o "$filename.spv" -entry ComputeMain
    else
        slangc "$f" -Wno-39001 -profile glsl_450 -target spirv -o "$filename.vert.spv" -entry VertexMain
        slangc "$f" -Wno-39001 -profile glsl_450 -target spirv -o "$filename.frag.spv" -entry FragmentMain
    fi
done

echo "All shaders compiled!"
