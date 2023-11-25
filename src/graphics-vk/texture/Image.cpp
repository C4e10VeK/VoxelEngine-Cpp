//
// Created by chelovek on 11/18/23.
//

#include "Image.h"

#include <iostream>

#include "../VulkanContext.h"
#include "../../constants.h"

VkFormat Image::selectSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling,
    VkFormatFeatureFlags featureFlags) {
    return vulkan::VulkanContext::get().getDevice().selectSupportedFormat(formats, tiling, featureFlags);
}

Image::Image(VkExtent3D extent, VkFormat format, VkImageAspectFlags aspectFlags, VkImageTiling tiling,
             VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
    : m_format(format),
      m_extent3D(extent) {

    auto &device = vulkan::VulkanContext::get().getDevice();
    auto &allocator = vulkan::VulkanContext::get().getAllocator();

    allocator.createImage(extent, format, tiling, usage, properties, m_image, m_allocation);

    m_imageView = device.createImageView(m_image, format, aspectFlags, {
        VK_COMPONENT_SWIZZLE_R,
        VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A
    });
}

Image::~Image() {
    destroy();
}

Image::operator VkImage() const {
    return m_image;
}

VkImage Image::getImage() const {
    return m_image;
}

VkImageView Image::getView() const {
    return m_imageView;
}

VkSampler Image::getSampler() const {
    return m_sampler;
}

VkFormat Image::getFormat() const {
    return m_format;
}

void Image::destroy() {
    if (m_destroyed) return;

    auto &device = vulkan::VulkanContext::get().getDevice();
    auto &allocator = vulkan::VulkanContext::get().getAllocator();

    if (m_sampler != VK_NULL_HANDLE)
        vkDestroySampler(device, m_sampler, nullptr);

    vkDestroyImageView(device, m_imageView, nullptr);
    allocator.destroyImage(m_image, m_allocation);

    m_destroyed = true;
}
