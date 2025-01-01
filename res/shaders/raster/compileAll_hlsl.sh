#!/bin/sh

rm *.spv

dxc -spirv -T vs_6_0 -E main model_vs.hlsl -Fo model_vs.spv
dxc -spirv -T ps_6_0 -E main skybox.hlsl -Fo skybox.spv
dxc -spirv -T ps_6_0 -E main texturedPBR.hlsl -Fo texturedPBR.spv
