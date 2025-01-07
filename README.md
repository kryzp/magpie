# Lilythorn

Basically, I'm making a renderer, but I tend to overcomplicate things and then not learn anything so I'm strictly limiting it to just being a Vulkan/SDL3 based renderer, no fluff just features. I don't intend for it to be used for anything, I just wanna try implementing a bunch of cool stuff, like volumetrics :)

You'll notice it's very similar to my other Vulkan engine, because I basically ripped it out, removed all the garbage that was completely unnecessary and left only the Vulkan and SDL components, plus some maths and containers.

I recently bought Real Time Rendering, so it's time to make the investment worth it, I guess.

TODO:
- shader buffers + ubos should be set individually in their own function, then data just gets sent to the models
- merge shader buffer and shader parameters?
- endSingleTimeCommands should not have to vkQueueWaitIdle every time
- implement a render graph system!
- macro LLT_VK_CHECK to take care of the if(vkXXX() == bad result) { LLT_ERR ... }
- use multiple descriptor sets for different update frequencies, i.e: per frame, per instance, etc...
- make texture transitioning nicer
- global staging buffer rather than constantly creating a new one then deleting it
- texture transitioning when moving into and out of a graphics queue render call is a bit messy, wayyyy too much vkQueueSubmit -> vkQueueWaitIdle then-back-to-vkQueueSubmit's!!!
- VK_FORMAT_R32G32B32_SFLOAT isnt supported??? wtf???
- drawindirect support (proper)
- stencil buffer support!
- I think queues arent currently assigned properly(?)
- some kind of compute queue id system, so graphics pipelines can specifically decide to wait on select compute pipelines to finish.

BACKBURNER:
- imgui
- pbr
- volumetrics
- particles (compute shaders!!)
- fluid sims
- refraction and stuff

other todo that isnt really important:
- use multiple compute queues
- mac support "works" but things like the file io have to be changed since SDL stream functions arent availabvle on mac for some reason??? so figure that out
