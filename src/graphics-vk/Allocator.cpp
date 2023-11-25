//
// Created by chelovek on 11/18/23.
//

#include "Allocator.h"

#include "Instance.h"
#include "Device.h"
#include "VulkanDefenitions.h"

namespace vulkan {
    Allocator::Allocator(const Instance &instance, const Device &device) {
        VmaVulkanFunctions vulkanFunctions{};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.vulkanApiVersion = instance.getApiVersion();
        allocatorCreateInfo.instance = instance;
        allocatorCreateInfo.physicalDevice = instance.getPhysicalDevice();
        allocatorCreateInfo.device = device;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

        vmaCreateAllocator(&allocatorCreateInfo, &m_allocator);
    }

    Allocator::operator VmaAllocator() const {
        return m_allocator;
    }

    void Allocator::createImage(VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage &image, VmaAllocation &allocation) const {
        VmaAllocationCreateInfo allocationCreateInfo{};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocationCreateInfo.flags = properties;

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent = extent;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.format = format;
        imageCreateInfo.tiling = tiling;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = usage;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        CHECK_VK(vmaCreateImage(m_allocator, &imageCreateInfo, &allocationCreateInfo, &image, &allocation, nullptr));
    }

    void Allocator::destroyImage(VkImage image, VmaAllocation allocation) const {
        vmaDestroyImage(m_allocator, image, allocation);
    }

    void Allocator::destroy() {
        vmaDestroyAllocator(m_allocator);
    }
} // vulkan