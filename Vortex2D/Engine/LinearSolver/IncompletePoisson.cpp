//
//  IncompletePoisson.cpp
//  Vortex2D
//

#include "IncompletePoisson.h"

namespace Vortex2D { namespace Fluid {

IncompletePoisson::IncompletePoisson(const Renderer::Device& device, const glm::ivec2& size)
    : mIncompletePoisson(device, size, "../Vortex2D/IncompletePoisson.comp.spv",
                        {vk::DescriptorType::eStorageBuffer,
                         vk::DescriptorType::eStorageBuffer,
                         vk::DescriptorType::eStorageBuffer,
                         vk::DescriptorType::eStorageBuffer})
{

}

void IncompletePoisson::Init(Renderer::Buffer& d,
                             Renderer::Buffer& l,
                             Renderer::Buffer& b,
                             Renderer::Buffer& pressure)
{
    mIncompletePoissonBound = mIncompletePoisson.Bind({d, l, b, pressure});
}

void IncompletePoisson::Record(vk::CommandBuffer commandBuffer)
{
    mIncompletePoissonBound.Record(commandBuffer);
}

}}