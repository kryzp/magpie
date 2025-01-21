@echo off

set DXC_PATH=D:/dxc_2024_07_31/bin/x64/dxc.exe

%DXC_PATH% -spirv -T vs_6_0 -E main model_vs.hlsl -Fo model_vs.spv
%DXC_PATH% -spirv -T vs_6_0 -E main post_process_vs.hlsl -Fo post_process_vs.spv

%DXC_PATH% -spirv -T ps_6_0 -E main skybox.hlsl -Fo skybox.spv
%DXC_PATH% -spirv -T ps_6_0 -E main texturedPBR.hlsl -Fo texturedPBR.spv
%DXC_PATH% -spirv -T ps_6_0 -E main post_process_ps.hlsl -Fo post_process_ps.spv
