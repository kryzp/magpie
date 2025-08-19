#!/bin/sh

slangc hdr_to_environment_cubemap.slang -profile glsl_450 -target spirv -o hdr_to_environment_cubemap_vertex.spv -entry VertexMain
slangc hdr_to_environment_cubemap.slang -profile glsl_450 -target spirv -o hdr_to_environment_cubemap_fragment.spv -entry FragmentMain

slangc irradiance_convolution.slang -profile glsl_450 -target spirv -o irradiance_convolution_vertex.spv -entry VertexMain
slangc irradiance_convolution.slang -profile glsl_450 -target spirv -o irradiance_convolution_fragment.spv -entry FragmentMain

slangc prefilter_convolution.slang -profile glsl_450 -target spirv -o prefilter_convolution_vertex.spv -entry VertexMain
slangc prefilter_convolution.slang -profile glsl_450 -target spirv -o prefilter_convolution_fragment.spv -entry FragmentMain

slangc model.slang -profile glsl_450 -target spirv -o model_vertex.spv -entry VertexMain
slangc model.slang -profile glsl_450 -target spirv -o model_fragment.spv -entry FragmentMain
