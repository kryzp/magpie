@echo off

set DXC_PATH=D:/dxc_2024_07_31/bin/x64/dxc.exe

%DXC_PATH% -spirv -T vs_6_0 -fspv-debug=vulkan-with-source -E main model_vs.hlsl -Fo model_vs.spv
%DXC_PATH% -spirv -T vs_6_0 -fspv-debug=vulkan-with-source -E main primitive_vs.hlsl -Fo primitive_vs.spv
%DXC_PATH% -spirv -T vs_6_0 -fspv-debug=vulkan-with-source -E main primitive_quad_vs.hlsl -Fo primitive_quad_vs.spv

%DXC_PATH% -spirv -T ps_6_0 -fspv-debug=vulkan-with-source -E main skybox.hlsl -Fo skybox.spv
%DXC_PATH% -spirv -T ps_6_0 -fspv-debug=vulkan-with-source -E main texturedPBR.hlsl -Fo texturedPBR.spv
%DXC_PATH% -spirv -T ps_6_0 -fspv-debug=vulkan-with-source -E main equirectangular_to_cubemap.hlsl -Fo equirectangular_to_cubemap.spv
%DXC_PATH% -spirv -T ps_6_0 -fspv-debug=vulkan-with-source -E main irradiance_convolution.hlsl -Fo irradiance_convolution.spv
%DXC_PATH% -spirv -T ps_6_0 -fspv-debug=vulkan-with-source -E main prefilter_convolution.hlsl -Fo prefilter_convolution.spv
%DXC_PATH% -spirv -T ps_6_0 -fspv-debug=vulkan-with-source -E main brdf_integrator.hlsl -Fo brdf_integrator.spv
%DXC_PATH% -spirv -T ps_6_0 -fspv-debug=vulkan-with-source -E main hdr_tonemapping.hlsl -Fo hdr_tonemapping.spv
%DXC_PATH% -spirv -T ps_6_0 -fspv-debug=vulkan-with-source -E main bloom_downsample.hlsl -Fo bloom_downsample.spv
%DXC_PATH% -spirv -T ps_6_0 -fspv-debug=vulkan-with-source -E main bloom_upsample.hlsl -Fo bloom_upsample.spv
