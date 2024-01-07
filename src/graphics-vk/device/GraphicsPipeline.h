//
// Created by chelovek on 11/17/23.
//

#ifndef GRAPHICSPIPELINE_H
#define GRAPHICSPIPELINE_H

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Shader.h"
#include "../ShaderType.h"

namespace initializers {
    struct UniformBufferInfo;
}

class Device;

class GraphicsPipeline {
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    VkPipelineCache m_cache = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_uniformsSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_samplerSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_uniformSet = VK_NULL_HANDLE;
    VkDescriptorSet m_samplerSet = VK_NULL_HANDLE;
    ShaderType m_shaderType;
public:
    GraphicsPipeline(const std::vector<initializers::UniformBufferInfo> &bufferInfos, VkPipeline pipeline, VkPipelineLayout layout, VkPipelineCache cache, VkDescriptorSetLayout uniformSetLayout, VkDescriptorSetLayout samplerSetLayout, ShaderType shaderType);
    ~GraphicsPipeline();

    operator VkPipeline() const;

    ShaderType getType() const;
    VkPipelineLayout getLayout() const;
    VkDescriptorSet getUniformSet() const;
    VkDescriptorSet getSamplerSet() const;

    void bind(VkCommandBuffer commandBuffer, VkExtent2D extent2D);
    void bindDiscriptorSet(VkCommandBuffer commandBuffer, uint32_t dynamiOffsetCount = 0, const uint32_t *pDynamicOffsets = nullptr);

    void destroy();


    static std::shared_ptr<GraphicsPipeline> create(const std::vector<VkPipelineShaderStageCreateInfo> &stages, const std::vector<initializers::UniformBufferInfo> &bufferInfos, ShaderType type);
};



#endif //GRAPHICSPIPELINE_H
