@echo off

:: shaders will cause vkCreate<>Pipelines() to have an access violation crash for some reason
::set DXC=D:/DevTools/dxc/bin/x64/dxc.exe -fspv-debug=vulkan-with-source

set DXC=D:/DevTools/dxc/bin/x64/dxc.exe

:: vertex shaders
%DXC% -spirv -T vs_6_0 -E main src/model_vs.hlsl							-Fo compiled/model_vs.spv
%DXC% -spirv -T vs_6_0 -E main src/primitive_vs.hlsl						-Fo compiled/primitive_vs.spv
%DXC% -spirv -T vs_6_0 -E main src/fullscreen_triangle_vs.hlsl			-Fo compiled/fullscreen_triangle_vs.spv
%DXC% -spirv -T vs_6_0 -E main src/skybox_vs.hlsl						-Fo compiled/skybox_vs.spv

:: pixel shaders
%DXC% -spirv -T ps_6_0 -E main src/skybox_ps.hlsl						-Fo compiled/skybox_ps.spv
%DXC% -spirv -T ps_6_0 -E main src/texturedPBR_ps.hlsl					-Fo compiled/texturedPBR_ps.spv
::%DXC% -spirv -T ps_6_0 -E main src/subsurface_refraction_ps.hlsl			-Fo compiled/subsurface_refraction_ps.spv
%DXC% -spirv -T ps_6_0 -E main src/equirectangular_to_cubemap_ps.hlsl	-Fo compiled/equirectangular_to_cubemap_ps.spv
%DXC% -spirv -T ps_6_0 -E main src/irradiance_convolution_ps.hlsl		-Fo compiled/irradiance_convolution_ps.spv
%DXC% -spirv -T ps_6_0 -E main src/prefilter_convolution_ps.hlsl			-Fo compiled/prefilter_convolution_ps.spv
%DXC% -spirv -T ps_6_0 -E main src/brdf_integrator_ps.hlsl				-Fo compiled/brdf_integrator_ps.spv
%DXC% -spirv -T ps_6_0 -E main src/bloom_downsample_ps.hlsl				-Fo compiled/bloom_downsample_ps.spv
%DXC% -spirv -T ps_6_0 -E main src/bloom_upsample_ps.hlsl				-Fo compiled/bloom_upsample_ps.spv
%DXC% -spirv -T ps_6_0 -E main src/texture_uv_ps.hlsl					-Fo compiled/texture_uv_ps.spv

:: compute shaders
%DXC% -spirv -T cs_6_0 -E main compute/hdr_tonemapping_cs.hlsl			-Fo compiled/hdr_tonemapping_cs.spv
