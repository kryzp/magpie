# Magpie

It's a renderer (pt. 2) with a low-level Vulkan wrapper. Vulkan is the main focus so when available I just directly use Vulkan types since there's no plans for a generic graphics API interface, though I still try to have utility wrappers around common types like command buffers, images, buffers, samplers, etc...

I've noticed that most, if not *all* beginner tutorials to Vulkan are kind of terrible for learning what actual Vulkan rendering code looks like.
They often just have long blocks of endless functions doing arbitrary things with very little abstraction and a completely fixed render loop - they show you how to make a coloured rotating cube, but good luck trying to make the program do anything else (looking at you, [vulkan-tutorial](https://vulkan-tutorial.com/), though regardless it's a good start for learning how Vulkan is fundamentally designed, even if it is pretty outdated by now (you really don't want to be going through the pain of framebuffers and renderpasses on your first engine, they're practically only used in mobile where they actually have a performance benefit)).

[vk-guide](https://vkguide.dev/) is arguably better, but I'd argue the code there still falls into the same pitfall of not being extensible. That being said it does actually have some good modular components, like the descriptor builder system.

The real test in learning Vulkan turns out to be whether you've got the passion (or perhaps more aptly, willpower) to spend hours reading through blog posts and other pre-existing codebases and parsing out their approach to abstractions and rendering.
If you look at the commit history of this project you'd find that I tried out tons of different (mostly terrible) abstraction layers before I settled on the one I have right now (originally I tried to have an OpenGL style abstraction layer, please don't try doing that).

I hope maybe this project helps someone else :), it's not perfect though. I just wanna try implementing a bunch of cool stuff, like volumetric smoke, global illumination, gpu-based compute particles, etc... :)

Feel free to use any of the code in personal / professional / public / private projects as long as you credit me.

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
    - Shadow Mapping [working on this]
        - [~] Infrastructure
        - [~] Directional Lights
        - [ ] Point Lights

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
    [ ] Realistic Refraction
    [ ] Hybrid Clustered Deferred + Forward Rendering (for transparency)
    [ ] Asset System
        - Hot Reloading
        - Textures
        - Shaders
    [ ] Good macOS support
    [ ] Half Life: Alyx-style reticle/gun scope effect

### Project Structure
	- /core/            Core modules resposible for opening the window, main game loop, polling for events, etc...
	- /input/           Input state management
	- /io/              File and Memory streams
	- /math/            Maths utilities (Colour, Rectangle, ...)
	- /rendering/       High-Level rendering like materials, models, cameras, scenes, ...
	- /vulkan/          Low-level vulkan abstraction layer for images, buffers, samplers, command buffers, queues, descriptors, ...
	- /third_party/     Third party libraries
	- /assets/          [planned]

### Libraries Used
    - SDL 3
    - Vulkan + Volk + Vulkan Memory Allocator
    - GLM
    - ImGui
    - Assimp
    - STB (image, image write, truetype)
