#include "VulkanContext.h"

#include <iostream>
#include <array>

#include "Tools.h"
#include "VulkanDefenitions.h"
#include "../window/Window.h"
#include "device/GraphicsPipeline.h"
#include "uniforms/BackgroundUniform.h"
#include "uniforms/FogUniform.h"
#include "uniforms/LightUniform.h"
#include "uniforms/ProjectionViewUniform.h"
#include "uniforms/SkyboxUniform.h"
#include "uniforms/StateUniform.h"

constexpr uint32_t DESCRIPTOR_SET_COUNT = 1000;

namespace vulkan {

    PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKhr = nullptr;

    bool VulkanContext::vulkanEnabled = false;

    void UniformBuffersHolder::initBuffers() {
        m_buffers.emplace_back(std::make_unique<UniformBuffer>(sizeof(StateUniform)));
        m_buffers.emplace_back(std::make_unique<UniformBuffer>(sizeof(LightUniform)));
        m_buffers.emplace_back(std::make_unique<UniformBuffer>(sizeof(FogUniform)));
        m_buffers.emplace_back(std::make_unique<UniformBuffer>(sizeof(ProjectionViewUniform)));
        m_buffers.emplace_back(std::make_unique<UniformBuffer>(sizeof(BackgroundUniform)));
        m_buffers.emplace_back(std::make_unique<UniformBuffer>(sizeof(SkyboxUniform)));
    }

    const UniformBuffer* UniformBuffersHolder::operator[](Type index) const {
        return m_buffers.at(index).get();
    }

    void UniformBuffersHolder::destroy() {
        for (auto &uniformBuffer : m_buffers) {
            uniformBuffer.reset();
        }
    }

    VulkanContext::VulkanContext()
        : m_instance(Instance::create()),
          m_surface(m_instance.createSurface()),
          m_device(m_instance, m_surface),
          m_swapchain(std::make_unique<Swapchain>(m_surface, m_device)),
          m_allocator(m_instance, m_device) {
        vkCmdPushDescriptorSetKhr = reinterpret_cast<PFN_vkCmdPushDescriptorSetKHR>(vkGetDeviceProcAddr(m_device, "vkCmdPushDescriptorSetKHR"));
    }

    void VulkanContext::initDescriptorPool() {
        const std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, DESCRIPTOR_SET_COUNT },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DESCRIPTOR_SET_COUNT }
        };

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.maxSets = DESCRIPTOR_SET_COUNT;
        descriptorPoolCreateInfo.poolSizeCount = descriptorPoolSizes.size();
        descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
        descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

        CHECK_VK(vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool));
    }

    void VulkanContext::initDepth() {
        const auto swapChainExtent = m_swapchain->getExtent();
        m_imageDepth = std::make_unique<ImageDepth>(VkExtent3D{swapChainExtent.width, swapChainExtent.height, 1});
    }

    void VulkanContext::initFrameDatas() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = m_device.getGraphis().getIndex();

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        CHECK_VK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_presentSemaphore));
        CHECK_VK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderSemaphore));
        CHECK_VK(vkCreateFence(m_device, &fenceInfo, nullptr, &m_renderFence));

        for (auto &m_frameData : m_frameDatas) {
            CHECK_VK(vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_frameData.commandPool));

            commandBufferAllocateInfo.commandPool = m_frameData.commandPool;

            CHECK_VK(vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &m_frameData.commandBuffer));
        }
    }

    void VulkanContext::initUniformBuffers() {
        m_uniformBuffersHolder.initBuffers();
    }

    void VulkanContext::destroy() {
        vkDeviceWaitIdle(m_device);

        for (auto &frameData : m_frameDatas) {
            vkDestroyCommandPool(m_device, frameData.commandPool, nullptr);
        }

        vkDestroyFence(m_device, m_renderFence, nullptr);
        vkDestroySemaphore(m_device, m_presentSemaphore, nullptr);
        vkDestroySemaphore(m_device, m_renderSemaphore, nullptr);

        m_uniformBuffersHolder.destroy();
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_imageDepth->destroy();
        m_allocator.destroy();
        m_swapchain->destroy();
        m_device.destroy();
        m_surface.destroy();
        m_instance.destroy();
    }

    const Device& VulkanContext::getDevice() const {
        return m_device;
    }

    const Allocator &VulkanContext::getAllocator() const {
        return m_allocator;
    }

    const State &VulkanContext::getCurrentState() const {
        return m_state;
    }

    const Swapchain& VulkanContext::getSwapchain() const {
        return *m_swapchain;
    }

    const ImageDepth& VulkanContext::getDepth() const {
        return *m_imageDepth;
    }

    VkDescriptorPool VulkanContext::getDescriptorPool() const {
        return m_descriptorPool;
    }

    const UniformBuffer* VulkanContext::getUniformBuffer(UniformBuffersHolder::Type type) const {
        return m_uniformBuffersHolder[type];
    }

    void VulkanContext::recreateSwapChain() {
        m_swapchain->destroy();
        m_swapchain.reset();
        m_swapchain = std::make_unique<Swapchain>(m_surface, m_device);
    }

    void VulkanContext::updateState(GraphicsPipeline* pipeline) {
        m_state.pipeline = pipeline;
    }

    void VulkanContext::updateState(VkCommandBuffer commandBuffer) {
        m_state.commandbuffer = commandBuffer;
    }

    void VulkanContext::beginDraw(float r, float g, float b, VkAttachmentLoadOp loadOp) {
        CHECK_VK(vkWaitForFences(m_device, 1, &m_renderFence, VK_TRUE, UINT64_MAX));
        CHECK_VK(vkResetFences(m_device, 1, &m_renderFence));

        CHECK_VK(vkAcquireNextImageKHR(m_device, *m_swapchain, UINT64_MAX, m_presentSemaphore, VK_NULL_HANDLE, &m_currentImage));

        CHECK_VK(vkResetCommandBuffer(m_frameDatas[m_currentFrame].commandBuffer, 0));

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = 0;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;

        CHECK_VK(vkBeginCommandBuffer(m_frameDatas[m_currentFrame].commandBuffer, &commandBufferBeginInfo));

        tools::insertImageMemoryBarrier(m_frameDatas[m_currentFrame].commandBuffer,
            m_swapchain->getImages().at(m_currentImage),
            0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

        tools::insertImageMemoryBarrier(m_frameDatas[m_currentFrame].commandBuffer,
            m_imageDepth->getImage(),
            0,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 });

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = m_swapchain->getImageViews().at(m_currentImage);
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = loadOp;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = { r, g, b, 1.0f };

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = m_imageDepth->getView();
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = { {0, 0}, {Window::width, Window::height} };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;
        renderingInfo.pStencilAttachment = &depthAttachment;

        vkCmdBeginRendering(m_frameDatas[m_currentFrame].commandBuffer, &renderingInfo);

        updateState(m_frameDatas[m_currentFrame].commandBuffer);
    }

    void VulkanContext::endDraw() {

        vkCmdEndRendering(m_frameDatas[m_currentFrame].commandBuffer);

        tools::insertImageMemoryBarrier(m_frameDatas[m_currentFrame].commandBuffer,
            m_swapchain->getImages().at(m_currentImage),
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            0,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

        CHECK_VK(vkEndCommandBuffer(m_frameDatas[m_currentFrame].commandBuffer));

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        const std::array waitSemaphores = { m_presentSemaphore };
        constexpr VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_frameDatas[m_currentFrame].commandBuffer;

        const std::array signalSemaphores = { m_renderSemaphore };
        submitInfo.signalSemaphoreCount = signalSemaphores.size();
        submitInfo.pSignalSemaphores = signalSemaphores.data();

        vkQueueSubmit(m_device.getGraphis(), 1, &submitInfo, m_renderFence);

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = signalSemaphores.size();
        presentInfo.pWaitSemaphores = signalSemaphores.data();

        const VkSwapchainKHR swapchains[] = { *m_swapchain };

        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &m_currentImage;

        CHECK_VK(vkQueuePresentKHR(m_device.getPresent(), &presentInfo));

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    VulkanContext &VulkanContext::get() {
        static VulkanContext context;
        return context;
    }

    void VulkanContext::initialize() {
        auto &context = get();
        vulkanEnabled = true;
        context.initDescriptorPool();
        context.initDepth();
        context.initFrameDatas();
        context.initUniformBuffers();
    }

    void VulkanContext::waitIdle() {
        auto &device = get().getDevice();
        device.waitIdle();
    }

    void VulkanContext::finalize() {
        auto &context = get();
        context.destroy();
    }

    bool VulkanContext::isVulkanEnabled() {
        return vulkanEnabled;
    }
} // vulkan