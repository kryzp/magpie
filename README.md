# Magpie

**Magpie** is primarily a Vulkan renderer/game engine (-ish, closer to a more developed framework) written in C.

![](images/indirect_deferred.png)
![](images/cool.png)
![](images/sponza.png)

## About the Project
Basically, this is a testing ground for whatever programming project I want to try at any given point. I hope maybe this project helps someone else, feel free to use any of the code in your own projects, as long as you credit me a little. I also hate that I have to clarify that none of this is vibecoded (yes I talk to myself via comments get over it). This is actually something I care about, and put time into. Check the commit history if you don't believe me.

Yes, it's over-engineered for a solo project, but I enjoy good code. No guarantees on quality though, I'm a second-year CS student. Most of this code is probably bad, some of it is maybe good :).

This project has been the death of me.

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
- **Page-Allocated Geometry Data** for the Scene
- **Lockless (almost) fiber-based job system**, with an even lower-latency spin mode
- **Controller Support**
- **3D Audio**
- **Debug Rendering** (lines, circles, spheres, AABB, OBB, crosses, etc.)
- **GPU Profiler**

## Roadmap

### Planned Features (In rough order of what's next)
- Text / Font Rendering
- Bone / Joint Based Animation
- More sophisticated debug logging / tracing system
- Volumetrics (Clouds, Smoke, etc.)
- Multi-Threaded CPU profiler integrated with the Job system (difficult due to fibers, as there is no guarantee a job will start and end on the same thread)
- ImGui Visualisation for Profilers (CPU & GPU)
- Irradiance Probes with Spherical Harmonics, which automatically reposition themselves
- Reflective Objects (such as mirrors)
- Fur / Hair Rendering
- UI System
- Compositive Post Processing Pipeline
- Hybrid Clustered Deferred + Forward Rendering (For Transparency)
- Refraction
- Shader Permutations(?)

### Mini Project Ideas
- Realistic Ocean water rendering based on FFT
- Terrain Generator based on No Man's Sky GDC Talk
- Compute particles based on Naughty Dog's *The Last of Us Part II* SigGraph 2020 Talk
- *Half-Life: Alyx* style reticle / gun scope effect (scope is only visible through the front holographic "lens", probably done through stencil magic)

---

## Under the Hood: Architecture

The codebase is split into a tiered system to make development easier and more compartmentalized. This is akin to the Source engine (Tier0, Tier1, ...) or the Decima Engine (OS, PIGS, ...), though a little more granular.

### Namespaces & Unity Build
Each layer follows a strict namespace system. Since this is C, I'm referring to typically 2-3 (rarely 4, sometimes 1) capitalized characters in front of each exposed type or function in the layer indicating where it comes from. This prevents naming collisions and makes code much easier to read. `/core/` is the exception to this rule, and has no namespace for brevity, as it contains common types used throughout the codebase (maths functions, `typedef`s for unsigned types, etc.).

It is a **unity build**. Both headers and source are `#include`'d in a single compilation unit. This simplifies compilation to just compiling a single file (+ external libraries if needed) which is much, much faster than compiling traditionally. It means no more incremental builds (but when were those ever useful anyway eh?), and it also means you don't need to bother with `#include`s, which is nice.

Headers exist to document the API from a higher level because it's nice to be able to read everything at a glance.

### Rendering Structure
Rendering is fundamentally abstracted into what should be three layers, but is only two right now:

1. **The Vulkan abstraction layer (`/graphics/`)**: Makes up the bulk of the graphics layer. Handles synchronization, resource management, etc. GPU resources are assigned handles (keys) by the graphics device, and only get resolved when they're used. Pretty much all high-level interactions with Vulkan either go through the device or command buffer (which has to get submitted to the device anyway).
2. **The Rendering abstraction layer (`/render/`)**: Consists of a render graph abstraction and (generally) stateless render stage code, such as the culling, geometry, and lighting stages.
3. **The Scene abstraction layer (WIP)**: Manages meshes, materials, and lights.

### Hot-Code Reloading
The actual implementation of this is relatively simple. All memory is allocated at once in the beginning of the program, so all we have to do is reload the DLL (thank you Handmade Hero!). 

This does mean that global data is a little tricky because it gets reset whenever the app is re-compiled at runtime. However, it can be fixed by:
1. Using as little globally-accessible data as possible.
2. When it is used, using it in the form of a pointer into the main app struct which is set each frame anyway.

Data that simply cannot survive a hot-reload, such as OS level features (i.e. the fiber-based job system) all lie in `/os/`, where the actual entry point code per-platform is stored. Thread-local state is maintained in `/os/`, for example.

`/os/` ultimately doesn't rely on the app at all or any layer "higher" than `/core/` and `/input/`, it just makes some assumptions about specific functions inside the DLL (namely, `AppXXX`) which take in the context pointer (generic, just some allocated data returned by `AppInit`) alongside maybe some other data (such as input, in the case of `AppTick`).

```
+--------------------------------------------------+
|  win32_main.exe                                  |
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

## QnA

> Where's the cool engine UI?????

Not a game engine. And even if it was, I'd only make the editor UI after deciding on the game.
