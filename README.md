# Magpie

Vulkan renderer written in C++, with some other features added on (basically a testing ground for whatever programming project I wanna try at any point).

I hope maybe this project helps someone else. Feel free to use any of the code in any projects as long as you credit me. No gurantees on quality though. Some of this code is (probably) bad, some of it is (maybe) good ;).

### Interesting Parts
- `app`
- `core/class_db`
- `assets/*`
- `graphics/render_graph`
- `graphics/render_scene`
- `graphics/device`
- `graphics/renderers/*`
- `job/*`
- `res/frustum_culling`

### Notable Features
- Right-handed Z-up coordinates (as it SHOULD be)
- Modern bindless resource design
- GPU Driven Rendering: Bindless materials and meshes (global vertex buffer, vertex pulling, etc...)
- IBL (Image-Based Lighting)
- Compute Frustum Culling
- Render Graph that handles resource management, pipeline barriers and synchronization
- Indirect Deferred Rendering
- Page-allocated Render Scene
- High-performance lockless fiber-based job system, with low-latency spin mode. Input and OS-events are handled on the main thread while the main game loop runs on a seperate "root".
- RTTI macro system allowing for type introspection and other cool stuff like iterating over fields
- ImGui Integration
- Controller support I guess :p
- Timeline semaphores for synchronization (except for swapchain, that still has to use binary ones...)
- Linear GPU allocators
- Async Asset Streaming (integrated with job system)

### Ideas / Todo List
- Asset Hot-Reloading
- More sophisticated debug logging / tracing system
- GPU profiler
- Multi-Threaded CPU profiler integrated with job system
- ImGui visualisation for profilers
- Volumetrics
- Irradiance probes with spherical harmonics
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
- Shader Permutations?

### Pictures
![](images/indirect_deferred.png)
![](images/cool.png)
