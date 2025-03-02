# Lilythorn

It's a renderer, I tend to overcomplicate things and then not learn anything so I'm strictly limiting it to just being a Vulkan/SDL3 based renderer, no fluff just features. I don't intend for it to be used for anything, I just wanna try implementing a bunch of cool stuff, like volumetric smoke, global illumination, gpu-based compute particles, etc... :)

TODO (no particular order):
- shader buffers + ubos should be set individually in their own function, then data just gets sent to the models
- implement a render graph system!
- make texture transitioning nicer
- VK_FORMAT_R32G32B32_SFLOAT isnt supported??? wtf???
- drawindirect support (proper)
- stencil buffer support!
- I think queues arent currently assigned properly(?)
- some kind of compute queue id system, so graphics pipelines can specifically decide to wait on select compute pipelines to finish.
- command buffer abstraction
- properly comment the code
- sometimes way more pipelines get created then usual, for some reason?? (the blend attachments loop is the perpetrator for some reason)
- should be multiple bind descriptor sets for the different frequencies (per pass, per material, per frame etc)
- file structure is a mess re-organise it, like why is renderer.cpp not in /graphics/???
- imgui is all over the place and should be organised more
- renderinfo and rendertarget are super similar. am i certain i cant merge them somehow

BACKBURNER:
- volumetrics
- particles (compute shaders!! follow the naughty dog siggraph)
- realistic ocean water
- refraction and stuff
- post processing pipeline

other todo that isnt really important:
- use multiple compute queues
- mac support "works" but things like the file io have to be changed since SDL stream functions arent availabvle on mac for some reason??? so figure that out
