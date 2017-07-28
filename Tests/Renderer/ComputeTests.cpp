//
//  ComputeTests.cpp
//  Vortex2D
//

#include <gtest/gtest.h>

#include <Vortex2D/Renderer/Pipeline.h>
#include <Vortex2D/Renderer/DescriptorSet.h>
#include <Vortex2D/Renderer/Work.h>
#include <Vortex2D/Renderer/CommandBuffer.h>

#include "Verify.h"

using namespace Vortex2D::Renderer;

extern Device* device;

TEST(ComputeTests, WriteBuffer)
{
    std::vector<float> data(100, 23.4f);
    Buffer buffer(*device, vk::BufferUsageFlagBits::eUniformBuffer, true, sizeof(float) * data.size());

    buffer.CopyFrom(data);

    CheckBuffer(data, buffer);
}

TEST(ComputeTests, BufferCopy)
{
    std::vector<float> data(100, 23.4f);
    Buffer buffer(*device, vk::BufferUsageFlagBits::eUniformBuffer, false, sizeof(float) * data.size());
    Buffer inBuffer(*device, vk::BufferUsageFlagBits::eUniformBuffer, true, sizeof(float) * data.size());
    Buffer outBuffer(*device, vk::BufferUsageFlagBits::eUniformBuffer, true, sizeof(float) * data.size());

    inBuffer.CopyFrom(data);

    ExecuteCommand(*device, [&](vk::CommandBuffer commandBuffer)
    {
       buffer.CopyFrom(commandBuffer, inBuffer);
       outBuffer.CopyFrom(commandBuffer, buffer);
    });

    CheckBuffer(data, outBuffer);
}

struct Particle
{
    alignas(8) glm::vec2 position;
    alignas(8) glm::vec2 velocity;
};

struct UBO
{
    alignas(4) float deltaT;
    alignas(4) int particleCount;
};

TEST(ComputeTests, BufferCompute)
{
    std::vector<Particle> particles(100, {{1.0f, 1.0f}, {10.0f, 10.0f}});

    Buffer buffer(*device, vk::BufferUsageFlagBits::eStorageBuffer, true, sizeof(Particle) * 100);
    Buffer uboBuffer(*device, vk::BufferUsageFlagBits::eUniformBuffer, true, sizeof(UBO));

    UBO ubo = {0.2f, 100};

    buffer.CopyFrom(particles);
    uboBuffer.CopyFrom(ubo);

    auto shader = device->GetShaderModule("Buffer.comp.spv");

    auto descriptorLayout = DescriptorSetLayoutBuilder()
            .Binding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, 1)
            .Binding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute, 1)
            .Create(*device);

    auto descriptorSet = MakeDescriptorSet(*device, descriptorLayout);

    DescriptorSetUpdater(*descriptorSet)
            .WriteBuffers(0, 0, vk::DescriptorType::eStorageBuffer).Buffer(buffer)
            .WriteBuffers(1, 0, vk::DescriptorType::eUniformBuffer).Buffer(uboBuffer)
            .Update(device->Handle());

    auto layout = PipelineLayoutBuilder()
            .DescriptorSetLayout(descriptorLayout)
            .Create(device->Handle());

    auto pipeline = MakeComputePipeline(device->Handle(), shader, *layout);

    ExecuteCommand(*device, [&](vk::CommandBuffer commandBuffer)
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *layout, 0, {*descriptorSet}, {});
        commandBuffer.dispatch(1, 1, 1);
    });

    std::vector<Particle> output(100);
    buffer.CopyTo(output);

    for (int i = 0; i < 100; i++)
    {
        auto particle = output[i];
        Particle expectedParticle = {{3.0f, 3.0f}, {10.0f, 10.0f}};

        EXPECT_FLOAT_EQ(expectedParticle.position.x, particle.position.x) << "Value not equal at " << i;
        EXPECT_FLOAT_EQ(expectedParticle.position.x, particle.position.y) << "Value not equal at " << i;
        EXPECT_FLOAT_EQ(expectedParticle.velocity.x, particle.velocity.x) << "Value not equal at " << i;
        EXPECT_FLOAT_EQ(expectedParticle.velocity.y, particle.velocity.y) << "Value not equal at " << i;
    }
}

TEST(ComputeTests, ImageCompute)
{
    Texture stagingTexture(*device, 50, 50, vk::Format::eR32Sfloat, true);
    Texture inTexture(*device, 50, 50, vk::Format::eR32Sfloat, false);
    Texture outTexture(*device, 50, 50, vk::Format::eR32Sfloat, false);

    std::vector<float> data(50*50,1.0f);
    stagingTexture.CopyFrom(data);

    auto shader = device->GetShaderModule("Image.comp.spv");

    auto descriptorSetLayout = DescriptorSetLayoutBuilder()
            .Binding(0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 1)
            .Binding(1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 1)
            .Create(*device);

    auto descriptorSet = MakeDescriptorSet(*device, descriptorSetLayout);

    DescriptorSetUpdater(*descriptorSet)
            .WriteImages(0, 0, vk::DescriptorType::eStorageImage).Image({}, inTexture, vk::ImageLayout::eGeneral)
            .WriteImages(1, 0, vk::DescriptorType::eStorageImage).Image({}, outTexture, vk::ImageLayout::eGeneral)
            .Update(device->Handle());

    auto layout = PipelineLayoutBuilder()
            .DescriptorSetLayout(descriptorSetLayout)
            .Create(device->Handle());

    auto pipeline = MakeComputePipeline(device->Handle(), shader, *layout);

    ExecuteCommand(*device, [&](vk::CommandBuffer commandBuffer)
    {
        inTexture.CopyFrom(commandBuffer, stagingTexture);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *layout, 0, {*descriptorSet}, {});
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
        commandBuffer.dispatch(16, 16, 1);
        stagingTexture.CopyFrom(commandBuffer, outTexture);
    });

    std::vector<float> doubleData(data.size(), 2.0f);
    CheckTexture(doubleData, stagingTexture);
}

TEST(ComputeTests, Work)
{
    Buffer buffer(*device, vk::BufferUsageFlagBits::eStorageBuffer, true, sizeof(float)*16*16);
    Work work(*device, {16, 16}, "Work.comp.spv", {vk::DescriptorType::eStorageBuffer});

    auto boundWork = work.Bind({buffer});

    ExecuteCommand(*device, [&](vk::CommandBuffer commandBuffer)
    {
        boundWork.Record(commandBuffer);
    });

    std::vector<float> expectedOutput(16*16);
    for (int i = 0; i < 16; i ++)
    {
        for (int j = 0; j < 16; j++)
        {
            if ((i + j) % 2 == 0)
            {
                expectedOutput[i + j * 16] = 1;
            }
            else
            {
                expectedOutput[i + j * 16] = 0;
            }
        }
    }

    CheckBuffer(expectedOutput, buffer);
}