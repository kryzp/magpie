# Magpie

**Magpie** is primarily a Vulkan renderer/game engine (-ish, closer to a more developed framework) written in pure C.

![](images/indirect_deferred.png)
![](images/cool.png)
![](images/sponza.png)


## About the Project
Basically, this is a testing ground for whatever programming project I want to try at any given point. I hope maybe this project helps someone else. Ask me anything!

Yes, it's over-engineered for a solo project, but I enjoy good code. No guarantees on quality though, I'm a second-year CS student. Most of this code is probably bad, some of it is maybe good :).

!I also hate that I have to clarify that none of this is vibecoded (yes I talk to myself via comments get over it, though some may be a bit vulgar). This is actually something I care about, and put time into. Check the commit history if you don't believe me.

This isn't my first Vulkan or game engine project. If you look into the repository's history, it's gone through about 3-4 seperate re-writes (the original version, "Lilythorn" I accidentally wiped from the history entirely, oops...), and even then it was initially based off of my (very crudely written) [Wyvern](https://github.com/kryzp/wyvern) game engine which I made for my NEA all the way back in Year 13 for my A-Levels.


### Resources Used
- Real Time Rendering 4th Edition
- Game Engine Architecture 3rd Edition
- Vulkan Guide

---

## Notable Features
- **True Hot-Code Reloading** for everything outside of `/os/` (app code written to DLL)
- **Render Graph** that handles resource management, pipeline barriers, and synchronization
- **Bindless Resource Design**
- **GPU Driven Rendering:** Bindless materials and meshes (global vertex buffer, vertex pulling, etc.)
- **IBL** (Image-Based Lighting)
- **Compute Frustum Culling**
- **Indirect Deferred Rendering**
- **Point Lights** with compute-culled shadow-mapping
- **ImGui Integration** (C bindings)
- **Right-Handed Z-up Coordinates** (as it SHOULD be)
- **Async Generic Asset Streaming**
- **Asset Hot-Reloading**
- **Arena Memory Allocation System**
- **Page-Allocated Geometry Data** for the Render Scene
- **Lockless (almost) fiber-based job system**, with an even lower-latency spin mode
- **Controller Support**
- **3D Audio**
- **Debug Rendering** (lines, circles, spheres, AABB, OBB, crosses, etc.)
- **GPU Profiler**
- **Modular Entity System**
- **Logging System** with levels (trace, debug, info, ...), channels (graphics, assets, ...), file output, collapsing repeated messages (deduplication), ...


## Roadmap

### Planned Features (in rough order of what's next)
- Text / Font Rendering
- Bone / Joint Based Animation
- More sophisticated debug logging / tracing system
- Volumetrics (Clouds, Smoke, etc.)
- Multi-Threaded CPU profiler integrated with the Job system (difficult due to fibers, as there is no guarantee a job will start and end on the same thread)
- ImGui Visualisation for Profilers (CPU & GPU)
- Irradiance Probes with Spherical Harmonics, which automatically reposition themselves
- Reflective Objects (such as mirrors)
- Fur / Hair Rendering
- 2D Batch Quad Renderer (basically the SpriteBatch from XNA/FNA/MonoGame)
- UI System (needs 2D batch renderer)
- Compositive Post Processing Pipeline
- Hybrid Clustered Deferred + Forward Rendering (For Transparency)
- Refraction
- Shader Permutations(?)


### Non-Graphics Planned Features (general engine stuff that interests me)
- Custom (simple) physics engine
- Complete the entity system
- Cutscenes!


### Mini Project Ideas
- Realistic Ocean water rendering based on FFT
- Terrain Generator based on No Man's Sky GDC Talk
- Compute particles based on Naughty Dog's *The Last of Us Part II* SigGraph 2020 Talk
- *Half-Life: Alyx* style reticle / gun scope effect (scope is only visible through the front holographic "lens", probably done through stencil magic)

---

## Architecture
The codebase is split into a tiered system to make development easier and more compartmentalized. This is akin to the Source engine (Tier0, Tier1, ...) or the Decima Engine (OS, PIGS, ...), though a little more granular.


### Namespaces
Each layer follows a strict namespace system. Since this is C, I'm referring to typically 2-3 (rarely 4, sometimes 1) capitalized characters in front of each exposed type or function in the layer indicating where it comes from. This prevents naming collisions and makes code much easier to read. `/core/` is the exception to this rule, and has no namespace for brevity, as it contains common types used throughout the codebase (maths functions, `typedef`s for unsigned types, etc.).


### Unity Build
Both headers and source are `#include`'d in a single compilation unit. This simplifies compilation to just compiling a single file (+ external libraries if needed) which is much, much faster than compiling traditionally. It means no more incremental builds (but when were those ever useful anyway eh?), and it also means you don't need to bother with `#include`s, which is nice.

Headers exist to document the API from a higher level because it's nice to be able to read everything at a glance.


### The Layer Organisation
Layers strictly only propogate upwards, that is to say, a layer *A* that uses functionality by layer *B* will never have it's own functionality used by layer *B*. This means that circular dependencies are essentially impossible, and terrible architecture is usually pretty obvious when you realise you need to do some pretty sketchy stuff to get something to work. That being said, *dependency injection* is perfectly fine. Callbacks are used all over the codebase (for a simple example, `/core/` doesn't have access to `/log/` directly, so it exposes a fatal handler function that we can set to print a log from a higher level).

You can intuitively see how some layers clearly depend on others, for instance, *rendering* needs to have access to low level *graphics* operations, but also *assets* such as textures and models (which ultimately also need to use the *graphics* layer). Other layers effectively lie parallel to each other, such *audio* and *rendering*. Entities naturally lie above the core engine systems such as physics and rendering but below higher level things like timelines. The editor needs access to all engine systems so it lies above everything.

The hierarchy of layers is visible via the order of `#include`'s in `app.c`.


### Memory Arenas
To simplify memory management in C I use a thing called a "memory arena" for pretty much anything. I'm not going to go into why because [this](https://www.youtube.com/watch?v=TZ5a3gCCZYo) talk by Ryan Fleury explains it way better. Essentially, they let you "group" allocations and formalize the idea of a "scope" for some object. The `/os/` layer allocates one massive arena up front and the app then allocates itself onto that arena and partitions it accordingly. Only one actual memory allocation happens in the entire program.

#### But... synchronisation!
Thank you for asking :) Memory arenas, since they're passed all across the project can in theory be pushed to and modified by multiple different jobs in parallel. This is a real problem, because then we have race conditions and the memory gets all fucked up.

I fix this by the simplest solution possible - at the program init we "partition" the process memory between the different subsystems (assets, rendering, graphics, entity, what have you ...) and each system then internally synchronises the allocations however it needs to according to the work it's doing.

There is no "main" allocator. Each system gets its own region of memory to which it synchronises pushes. The assumption, obviously, is that cross-system allocations don't happen (that is, a system will never *directly* access another systems arena and allocate from it).



### Job System
The job system is effectively at the core of the project. It is a fiber-based job system based off-of the pioneering work of Naughty Dog in their presentation "Parallelizing the Naughty Dog by Christian Gyrling". It's pretty complicated but the basic idea is that using a naiive job queue is that when a job dispatches more jobs and waits on them, the thread running that job stalls when it could be working! This can get even worse, when in some cases it can straight up lead to a deadlock as there might not be any free thread to complete the child job!

To fix this problem, we turn to a thing called a "fiber". The special thing about fibers is that they maintain their own call stack, which allows us to context-switch between any job at any time we want. Therefore, every time we "wait" at a counter (renamed - "Yield" because it's more apt) the fiber running that job context switches to a different job, and once all the jobs that the counter is waiting on are completed, then we resume that job by context-switching back into it.

If you're more curious I wrote a blog post about my implementation approach [here](https://kryzp.github.io/posts/fiber-job-system/).


### Assets
I'm actually pretty proud of this system! Assets are loaded entirely asynchronously since they're integrated with the job system. However, that leads to a problem, which is that an asset actually goes through multiple stages until it's considered "ready". In a naiive asset system, you typically have a single `Load()` function that handles everything from finding the file on the disk, to load it into memory, and potentally uploading to the GPU. That's pretty terrible because you have to wait for the entire asset to load in before moving on to the next one, when you could be loading multiple in parallel.

You can't just parallelize the whole thing because while loading an asset into memory is super simple, when it comes to things like uploading onto the GPU it's not so simple and we want to minimize the number of command buffers we use / submit. Therefore, I split asset loading into four stages:

1. The CPU Stage: This is where we find the file in memory, load it in, and allocate some temporary memory for it. We haven't modified the actual asset record yet. For example, loading in pixel data from a texture file.
2. The Alloc / Realloc Stage. This is where we actually move that temporary data into the asset and allocate any resources.
3. The GPU Stage. This is where we upload the data onto the GPU, exposing a command buffer and staging buffer which can be used.
4. The Cleanup Stage. Any temporary data that was allocated in previous stages can be cleaned up here. Only really necessary for external API's (e.g: cleaning up texture data from `stbi`) because we use arenas for everything internally anyway.

Essentially, we parallelize the CPU stage, and try to record as much instructions onto the command buffer in the GPU stage before submitting.

Hooking it up was more "awkward" than "hard" necessarily but it works and that's what matters.

The asset system also supports fancy things like hot-reloading of assets which was also harder than I initially assumed because some assets, like shaders or models, also have dependencies. So, we have to watch those for modifications as well. Fun stuff!


### The Entity System
I really dislike entity component systems with a viceral, primal, gut-wrenching hatred, but inheritance-based ones suck as well, so we go with the best solution in my opinion: you split entity behaviours into just... structs.

You just call to them for shared functionality (imagine - NPC's like shopkeepers, raiders, etc... all have some shared functionality (maybe you can talk to them, or they have an opinion of you, etc...) so you just move it out into a struct. Simple as!). Of course each entity has unique data (otherwise your game would be a little more exciting then staring at a mountain erode via rainfall) and it's easy as pie to do with this system. Best of both worlds!

It's kinda like a mega-structure entity approach except it doesn't feel like someone stabbing you 591224 times in the pancreas every time you try to add new behaviour into your three trillion line header file and find that you've yet again hit a naming conflict, and have become lost entirely. If you want physics, ask the physics engine for a handle and then use it. If you want audio, AI, rendering, all the same. An entity is just a glorified function call anyway.

I use some fancy X-Macro magic to streamline the process, like automatically generating a type description per-entity.

Everything an entity needs to inform decisions (pointers to other systems, the world, etc...) all just gets passed as a single context struct to it's tick functions.


### Rendering Structure
Rendering is fundamentally abstracted into what should be three layers, but is only two right now:

1. **The Vulkan abstraction layer (`/graphics/`)**: Makes up the bulk of the graphics layer. Handles synchronization, resource management, etc. GPU resources are assigned handles (keys) by the graphics device, and only get resolved when they're used. Pretty much all high-level interactions with Vulkan either go through the device or command buffer (which has to get submitted to the device anyway).
2. **The Scene abstraction layer (WIP, currently in `/render/`)**: Manages meshes, materials, and lights.
3. **The Rendering abstraction layer (`/render/`)**: Consists of a render graph abstraction and (generally) stateless render stage code, such as the culling, geometry, and lighting stages.


### Hot-Code Reloading
The actual implementation of this is simple in theory. All memory is allocated at once in the beginning of the program, so all we have to do is reload the DLL (thank you Handmade Hero!).

This does mean that global data is a little tricky because it gets reset whenever the app is re-compiled at runtime. However, it can be fixed by:
1. Using as little globally-accessible data as possible.
2. Re-Setting it whenever we hot reload.

Data that simply cannot survive a hot-reload, such as OS level features (i.e. the fiber-based job system) all lie in `/os/`, where the actual entry point code per-platform is stored.

`/os/` ultimately doesn't rely on the app at all or any layer "higher" than `/core/` and `/input/`, it just makes some assumptions about specific functions inside the DLL (namely, `AppXXX`) which take in the context pointer (generic, just some allocated data returned by `AppInit`) alongside maybe some other data (such as input, in the case of `AppTick`).

```
+--------------------------------------------------+
|  magpie_win32.exe                                |
|                                                  |
|  All OS primitives, job scheduler, etc...        |
|  Exposes API via function pointer table          |
|                                                  |
|  W32_RootJobEntry  --> AppInit(bootstrap_data)   |
|  W32_FrameJobEntry --> AppTick(input)            |
+--------------------------------------------------+
              |
              |  Passes API Table
              V
+--------------------------------------------------+
|  app.dll                                         |
|                                                  |
|  AppInit(...)                                    |
|  AppTick(...)                                    |
|  ...                                             |
|                                                  |
|  Calls jobs via: api->JobKick(...)               |
+--------------------------------------------------+
```


### Formatting
Files in each namespace begin with the full name of the namespace followed by an underscore, and include guards are just the capitalized name of the file.

All structs and enums are typedef'd (sorry Torvalds) to improve readability. However, avoid using misleading typedefs:

```C
typedef SomeOtherThing *MyDataType;
```

This is TERRIBLE, EVIL, and puppies GENUINELY DIE when you do this.

In most cases, you could probably do something like this anyway:

```C
typedef struct MyDataType MyDataType;
struct MyDataType
{
	SomeOtherThing *value;
};
```

Therefore, only do this when you really HAVE to use an opaque type that isn't portable at all (so much so that you're okay with sacrificing an innocent puppy for the sake of the code, hope it was worth it!), and in those cases make it clear that it's a pointer or anything that isn't plain ol' data.

Macros follow the naming convention of whatever makes the most sense: if it's meant to act like a function use `PascalCase`, if it's a constant use `SCREAMING_SNAKE_CASE`, etc.


### Cool Stuff
Interesting files that you might wanna have a look at if you're just starting with the codebase.

- `app`
- `core/core_arena`
- `core/core_scratch`
- `os/*`
- `os/job/*`
- `os/win32/*`
- `render/render_graph`
- `render/render_scene`
- `asset/asset_manager`
- `asset/serializer/*`
- `graphics/graphics_device`
- `entity/*`


## QnA

> Where's the cool engine UI?????

Not a game engine. And even if it was, I'd only make the editor UI after deciding on the game.
