//
//  Extrapolation.cpp
//  Vortex2D

#include "Extrapolation.h"

#include "vortex2d_generated_spirv.h"

namespace Vortex2D
{
namespace Fluid
{
Extrapolation::Extrapolation(Renderer::Device& device,
                             const glm::ivec2& size,
                             Renderer::GenericBuffer& valid,
                             Velocity& velocity,
                             int iterations)
    : mDevice(device)
    , mValid(device, size.x * size.y)
    , mVelocity(velocity)
    , mExtrapolateVelocity(device, size, SPIRV::ExtrapolateVelocity_comp)
    , mExtrapolateVelocityBound(
          mExtrapolateVelocity.Bind({valid, mValid, velocity, velocity.Output()}))
    , mExtrapolateVelocityBackBound(
          mExtrapolateVelocity.Bind({mValid, valid, velocity.Output(), velocity}))
    , mConstrainVelocity(device, size, SPIRV::ConstrainVelocity_comp)
    , mExtrapolateCmd(device, false)
    , mConstrainCmd(device, false)
{
  mExtrapolateCmd.Record([&, iterations](Renderer::CommandEncoder& command) {
    command.DebugMarkerBegin("Extrapolate", {0.60f, 0.87f, 0.12f, 1.0f});
    for (int i = 0; i < iterations / 2; i++)
    {
      mExtrapolateVelocityBound.Record(command);
      velocity.Output().Barrier(command,
                                vk::ImageLayout::eGeneral,
                                vk::AccessFlagBits::eShaderWrite,
                                vk::ImageLayout::eGeneral,
                                vk::AccessFlagBits::eShaderRead);
      mValid.Barrier(command, vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
      mExtrapolateVelocityBackBound.Record(command);
      velocity.Barrier(command,
                       vk::ImageLayout::eGeneral,
                       vk::AccessFlagBits::eShaderWrite,
                       vk::ImageLayout::eGeneral,
                       vk::AccessFlagBits::eShaderRead);
      valid.Barrier(command, vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
    }
    command.DebugMarkerEnd();
  });
}

void Extrapolation::Extrapolate()
{
  mExtrapolateCmd.Submit();
}

void Extrapolation::ConstrainBind(Renderer::Texture& solidPhi)
{
  mConstrainVelocityBound = mConstrainVelocity.Bind({solidPhi, mVelocity, mVelocity.Output()});

  mConstrainCmd.Record([&](Renderer::CommandEncoder& command) {
    command.DebugMarkerBegin("Constrain Velocity", {0.82f, 0.20f, 0.20f, 1.0f});
    mConstrainVelocityBound.Record(command);
    mVelocity.CopyBack(command);
    command.DebugMarkerEnd();
  });
}

void Extrapolation::ConstrainVelocity()
{
  mConstrainCmd.Submit();
}

}  // namespace Fluid
}  // namespace Vortex2D
