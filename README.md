# Magpie

It's a renderer (pt. 2) with a low-level Vulkan wrapper. Vulkan is the main focus so when available I just directly use Vulkan types since there's no plans for a generic graphics API interface.

I just wanna try implementing a bunch of cool stuff, like volumetric smoke, global illumination, gpu-based compute particles, etc... :)

Yes, the code is a little messy at some points. The focus isn't on micro-optimisations, but macro-optimisations since the idea is smaller optimisations can always be made later down the line. It is also single-threaded since going fully multi-threaded for my first "proper" vulkan renderer would be... difficult.

Feel free to use any of the code in personal / professional / public / private projects as long as you credit me.

### (Mostly cool) Stuff to implement
	[ ] Well commented codebase + Assertations to make bugs easier to catch
	[ ] Volumetrics
	[ ] Parallel bindless resource system
	[ ] Fur / Hair Rendering
	[ ] Text & UI Rendering
	[ ] Realistic ocean water based on FFT
	[ ] Render Graph
	[ ] Terrain Generator based on No Man's Sky GDC Talk
	[ ] Post Processing Pipeline (Compositor?)
	[ ] GPGPU Particles based on Naughty Dog's Last of Us 2 SigGraph Talk
	[ ] Shadow Mapping: Point lights & Directional lights
	[ ] Realistic Refraction
	[ ] Hybrid Clustered Deferred + Forward Rendering (for transparency)
	[ ] Asset System + Hot Reloading
		- Textures
		- Shaders
	[ ] Good macOS support
	[ ] Half Life: Alyx-style reticle/gun scope effect

### Libraries Used
	- SDL 3
	- Vulkan + Volk + Vulkan Memory Allocator
	- GLM
	- ImGui
	- Assimp
	- STB (image, image write, truetype)
