# Magpie

It's a (vulkan) renderer written in C. It's got *complete* hot code reloading. That means the C code itself, not just shaders. :)

Currently it's in pretty early stages so it's missing a lot of polish, but the groundwork is there.

I've noticed that most, if not *all* beginner tutorials to Vulkan are kind of terrible for learning what actual Vulkan rendering code looks like.
They often just have long blocks of endless functions doing arbitrary things with very little abstraction and a completely fixed render loop - they show you how to make a coloured rotating cube, but good luck trying to make the program do anything else.

The real test in learning Vulkan turns out to be whether you've got the passion (or perhaps more aptly, willpower) to spend hours reading through blog posts and other pre-existing codebases and parsing out their approach to abstractions and rendering.

I hope maybe this project helps someone else. Feel free to use any of the code in any projects as long as you credit me. No gurantees on quality though. Some of this code is probably bad, some of it is maybe good.

### Pictures
TODO (It looks super cool and awesome trust me).

### Notable Features
	- Right-handed Z-up coordinates (as it SHOULD be)
	- Fully bindless + BDA
	- Hot reloading of source code
	- IBL (Image-Based Lighting)
	- GPU Driven Rendering
	- Compute Frustum Culling
	- Render Graph that handles pipeline barriers and synchronization
	- Deferred Rendering

### Ideas
	- Generic Job System
	- Volumetrics
	- Irradiance probes
	- Reflective mirrors
	- Bone / Joint based animation
	- Fur / Hair Rendering
	- Text / UI Rendering
	- Realistic ocean water based on FFT
	- Terrain Generator based on No Man's Sky GDC Talk
	- Compositive Post Processing Pipeline
	- Compute particles based on Naughty Dog's Last of Us 2 SigGraph 2020 Talk
	- Hybrid Clustered Deferred + Forward Rendering (for transparency)
	- Refraction
	- Half Life: Alyx style reticle / gun scope effect (scope is only visible through the front holographic "lens", probably done through stencil magic)

### Why C?
I like the language.
