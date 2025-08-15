# Magpie

It's a (vulkan) renderer written in C. It's got *complete* hot code reloading. That means the C code itself, not just shaders. :)

I've noticed that most, if not *all* beginner tutorials to Vulkan are kind of terrible for learning what actual Vulkan rendering code looks like.
They often just have long blocks of endless functions doing arbitrary things with very little abstraction and a completely fixed render loop - they show you how to make a coloured rotating cube, but good luck trying to make the program do anything else (looking at you, [vulkan-tutorial](https://vulkan-tutorial.com/), though regardless it's a good start for learning how Vulkan is fundamentally designed, even if it is pretty outdated by now (you really don't want to be going through the pain of framebuffers and renderpasses on your first engine, they're practically only used in mobile where they actually have a performance benefit)).

[vk-guide](https://vkguide.dev/) is arguably better, but I'd argue the code there still falls into the same pitfall of not being extensible. That being said it does actually have some good modular components, like the descriptor builder system.

The real test in learning Vulkan turns out to be whether you've got the passion (or perhaps more aptly, willpower) to spend hours reading through blog posts and other pre-existing codebases and parsing out their approach to abstractions and rendering.
If you look at the commit history of this project you'd find that I tried out tons of different (mostly terrible) abstraction layers before I settled on the one I have right now (originally I tried to have an OpenGL style abstraction layer, please don't try doing that).

I hope maybe this project helps someone else :). Feel free to use any of the code in personal / professional / public / private projects as long as you credit me. No gurantees on reliability though.

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
	- Half Life: Alyx-style reticle/gun scope effect
