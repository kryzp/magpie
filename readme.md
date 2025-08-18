# Magpie

It's a (vulkan) renderer written in C. It's got *complete* hot code reloading. That means the C code itself, not just shaders. :)

Currently (almost) all the logic is held between ``graphics_device`` and ``renderer``. I plan to seperate out the functionality into seperate files soon (tm). ``graphics_device`` is responsible for abstracting away a lot of raw Vulkan boilerplate code such as setting up , and the swapchain. ``renderer`` then uses the functionality given by ``graphics_device`` to do the actual high-level rendering of the world.

Currently there is only one "renderer", but in the future if 2D rendering is to be added, then I'll split it up into two seperate renderers. I've also got plans to add a debug renderer, allowing for debug lines / spheres, etc...

I've noticed that most, if not *all* beginner tutorials to Vulkan are kind of terrible for learning what actual Vulkan rendering code looks like.
They often just have long blocks of endless functions doing arbitrary things with very little abstraction and a completely fixed render loop - they show you how to make a coloured rotating cube, but good luck trying to make the program do anything else.

The real test in learning Vulkan turns out to be whether you've got the passion (or perhaps more aptly, willpower) to spend hours reading through blog posts and other pre-existing codebases and parsing out their coreroach to abstractions and rendering.

I hope maybe this project helps someone else. Feel free to use any of the code in personal / professional / public / private projects as long as you credit me. No gurantees on reliability though. Some (most) of this code is probably bad, some of it is maybe good, I'm just a beginner.

### Ideas
	- Volumetrics
	- Bone / Joint based Animation
	- Fur & Hair Rendering
	- Text & UI Rendering
	- Realistic ocean water based on FFT
	- Terrain Generator based on No Man's Sky GDC Talk
	- Compositive Post Processing Pipeline
	- GPGPU Particles based on Naughty Dog's Last of Us 2 SigGraph 2020 Talk
	- Hybrid Clustered Deferred + Forward Rendering (for transparency)
	- Refraction
	- Half Life: Alyx style reticle/gun scope effect (scope is only visible through the front holographic "lens", probably done through stencil magic)
