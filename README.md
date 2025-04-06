# Magpie

It's a renderer (pt. 2) with a low-level Vulkan wrapper. Vulkan is the main focus so when available I just directly use Vulkan types since there's no plans for a generic graphics API interface.

I just wanna try implementing a bunch of cool stuff, like volumetric smoke, global illumination, gpu-based compute particles, etc... :)

Feel free to use any of the code in personal / professional / public / private projects as long as you credit me.

### Currently Working On
	Render graph implementation based on [this](https://themaister.net/blog/2017/08/15/render-graphs-and-vulkan-a-deep-dive/) blog post by Hans-Kristian Arntzen.

### Features
	- Vulkan abstraction layer in /vulkan/
	- Parallel bindless resource system (any viable image view, buffer and sampler is automatically added to the bindless set)
	- GLTF Model loading
	- Material System (bindless)
	- PBR
		- Opaque Materials
		- Compute HDR Tonemapping
	- ImGui Support
	- Render Graph (pretty primitive so far)

### Project Structure
	- /core/			Core modules resposible for opening the window, main game loop, polling for events, etc...
	- /input/			Input state management
	- /io/				File and Memory streams
	- /math/			Maths utilities (Colour, Rectangle, ...)
	- /rendering/		High-Level rendering like materials, models, cameras, scenes, ...
	- /vulkan/			Low-level vulkan abstraction layer for images, buffers, samplers, command buffers, queues, descriptors, ...
	- /third_party/		Third party libraries
	- /assets/			[planned]

### Stuff to implement
	[ ] Well commented codebase
	[ ] Assertations to make bugs easier to catch
	[ ] Volumetrics
	[ ] Bone Animation
	[ ] Fur / Hair Rendering
	[ ] Text & UI Rendering
	[ ] Realistic ocean water based on FFT
	[ ] Terrain Generator based on No Man's Sky GDC Talk
	[ ] Post Processing Pipeline (Compositor?)
	[ ] GPGPU Particles based on Naughty Dog's Last of Us 2 SigGraph Talk
	[ ] Shadow Mapping: Point lights & Directional lights
	[ ] Realistic Refraction
	[ ] Hybrid Clustered Deferred + Forward Rendering (for transparency)
	[ ] Asset System
		- Hot Reloading
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
