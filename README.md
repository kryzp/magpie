# Lilythorn

It's a renderer, I tend to overcomplicate things and then not learn anything so I'm strictly limiting it to just being a Vulkan/SDL3 based renderer, no fluff just features. I don't intend for it to be used for anything, I just wanna try implementing a bunch of cool stuff, like volumetric smoke, global illumination, gpu-based compute particles, etc... :)

TODO (no particular order):
- shader buffers + ubos should be set individually in their own function, then data just gets sent to the models
- drawindirect support (proper)
- stencil buffer support!
- I think queues arent currently assigned properly(?)
- some kind of compute queue id system, so graphics pipelines can specifically decide to wait on select compute pipelines to finish.
- properly comment the code
- should be multiple bind descriptor sets for the different frequencies (per pass, per material, per frame etc)
- renderinfo and rendertarget are super similar. am i certain i cant merge them somehow
- command pool abstraction
- implement a render graph system!
- bindless material rendering
- hdr resolve tonemapper should be compute shader
- "vulkan backend" needs to be changed into "device"
- "backbuffer" needs to be changed into "swapchain"
- double-ended queue implementation is broken (see render target)
- spruit_sunrise_greg_zaal when generating the prefilter and irradiance it has too high values which cause blobs of white that look terrible :/ - cap it somehow

BACKBURNER:
- volumetrics
- clustered deferred rendering
- shadowmapping: directional and point lights
- particles (compute shaders!! follow the naughty dog last of us 2 siggraph)
- realistic ocean water based on fft
- refraction and stuff
- terrain generation based on no mans sky talk
- fur / hair rendering
- post processing pipeline
- ui rendering

other todo that isnt really important:
- use multiple compute queues
- mac support "works" but things like the file io have to be changed since SDL stream functions arent availabvle on mac for some reason??? so figure that out
