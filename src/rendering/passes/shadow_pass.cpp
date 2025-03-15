#include "shadow_pass.h"

#include "vulkan/core.h"
#include "vulkan/command_buffer.h"

llt::ShadowPass llt::g_shadowPass;

using namespace llt;

void ShadowPass::init()
{
}

void ShadowPass::dispose()
{
}

void ShadowPass::render(CommandBuffer &cmd, const Vector<SubMesh *> &renderList)
{
}
