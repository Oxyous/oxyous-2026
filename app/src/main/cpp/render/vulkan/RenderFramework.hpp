//
// Created by Mr Steven J Baldwin on 16/06/2026.
//

#ifndef OXYOUS_2026_RENDERFRAMEWORK_HPP
#define OXYOUS_2026_RENDERFRAMEWORK_HPP

#include "../../includes.hpp"
#include "../../DataStructures.hpp"
#include "../vulkan/RenderDevice.hpp"

class RenderFramework {
public:

    /* Allocate Command Pool */
    inline static bool
    allocateCommandPool(VkDevice device, VkCommandPool *commandPool, uint32_t queueFamilyIndex) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndex;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(device, &poolInfo, nullptr, commandPool) != VK_SUCCESS) {
            aout << "Failed to create command pool!" << std::endl;
            return false;
        }

        return true;
    }

    /* Allocate Command Buffer*/
    inline static bool
    allocateCommandBuffer(VkCommandPool commandPool, VkCommandBuffer &commandBuffer) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(RENDER_DEVICE->getDevice(), &allocInfo, &commandBuffer) !=
            VK_SUCCESS) {
            aout << "Failed to allocate command buffer!" << std::endl;
            return false;
        }

        return true;
    }

    /* Begin Command Helper function */
    inline static bool
    beginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *beginInfo) {
        if (beginInfo != nullptr) {
            if (vkBeginCommandBuffer(commandBuffer, beginInfo) != VK_SUCCESS) {
                aout << "Failed to begin recording command buffer!" << std::endl;
                return false;
            }
        } else {
            VkCommandBufferInheritanceInfo inheritanceInfo = {};
            inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            inheritanceInfo.renderPass = VK_NULL_HANDLE;
            inheritanceInfo.framebuffer = VK_NULL_HANDLE;
            inheritanceInfo.subpass = 0;
            inheritanceInfo.occlusionQueryEnable = VK_FALSE;
            inheritanceInfo.queryFlags = 0;
            inheritanceInfo.pipelineStatistics = 0;

            VkCommandBufferBeginInfo localBeginInfo = {};
            localBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            localBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            localBeginInfo.pInheritanceInfo = &inheritanceInfo;
            if (vkBeginCommandBuffer(commandBuffer, &localBeginInfo) != VK_SUCCESS) {
                aout << "Failed to begin recording command buffer!" << std::endl;
                return false;
            }
        }

        return true;
    }

    /* Set Image Layout */
    inline static void
    setImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspectMask,
                   VkImageLayout oldLayout, VkImageLayout newLayout,
                   VkImageSubresourceRange subresourceRange,
                   VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                   VkPipelineStageFlags dstStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        // Create an image barrier object
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.oldLayout = oldLayout;
        imageMemoryBarrier.newLayout = newLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (oldLayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                // Image layout is undefined (or does not matter)
                // Only valid as initial layout
                // No flags required, listed only for completeness
                imageMemoryBarrier.srcAccessMask = 0;
                break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                // Image is preinitialized
                // Only valid as initial layout for linear images, preserves memory contents
                // Make sure host writes have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image is a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image is a depth/stencil attachment
                // Make sure any writes to the depth/stencil buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image is a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image is a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image is read by a shader
                // Make sure any shader reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                break;
            default:
                throw std::invalid_argument("Unsupported layout transition!");
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (newLayout) {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image will be used as a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image will be used as a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image will be used as a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image layout will be used as a depth/stencil attachment
                // Make sure any writes to depth/stencil buffer have been finished
                imageMemoryBarrier.dstAccessMask =
                        imageMemoryBarrier.dstAccessMask |
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image will be read in a shader (sampler, input attachment)
                // Make sure any writes to the image have been finished
                if (imageMemoryBarrier.srcAccessMask == 0) {
                    imageMemoryBarrier.srcAccessMask =
                            VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                }
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                break;
            default:
                throw std::invalid_argument("Unsupported layout transition!");
        }

        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(
                commandBuffer,
                srcStageFlags,
                dstStageFlags,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
    }

    /*  */
    inline static bool findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                                      VkMemoryPropertyFlags properties, uint32_t &index) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                index = i;
                return true;
            }
        }

        return false;
    }

    /* */
    inline static bool
    createFrameBuffer(VkFormat format, int usage, int width, int height, GPUImage *frameBuffer) {
        VkDevice device = RENDER_DEVICE->getDevice();

        VkImageAspectFlags aspectMask = 0;
        VkImageLayout layout;

        switch (usage) {
            case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:
                aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            default:
            case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:
                aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                break;
        }

        /* Create FrameBuffer Image Object */
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &frameBuffer->image) != VK_SUCCESS) {
            aout << "Failed to create image!" << std::endl;
            return false;
        }

        /* Allocate Device Memory */
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, frameBuffer->image, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(RENDER_DEVICE->getPhysicalDevice(), &memProperties);

        uint32_t memoryTypeIndex;
        findMemoryType(RENDER_DEVICE->getPhysicalDevice(), memRequirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryTypeIndex);

        allocInfo.memoryTypeIndex = memoryTypeIndex;

        if (vkAllocateMemory(device, &allocInfo, nullptr, &frameBuffer->memory) != VK_SUCCESS) {
            aout << "Failed to allocate image memory!" << std::endl;
            return false;
        }

        /* Bind Image Memory */
        if (vkBindImageMemory(device, frameBuffer->image, frameBuffer->memory, 0) != VK_SUCCESS) {
            aout << "Failed to bind image memory!" << std::endl;
            return false;
        }

        /* Create FrameBuffer View Object */
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = frameBuffer->image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectMask;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &frameBuffer->imageView) != VK_SUCCESS) {
            aout << "Failed to create image view!" << std::endl;
            return false;
        }
        return true;
    }

    /* Create Vulkan Buffer */
    inline static bool
    createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                 VkDeviceSize *sizeOut, GPUBuffer *buffer) {
        VkDevice device = RENDER_DEVICE->getDevice();

        /* Create buffer object */
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer->buffer) != VK_SUCCESS) {
            aout << "Failed to create buffer!" << std::endl;
            return false;
        }

        /* Allocate Device Memory */
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer->buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(RENDER_DEVICE->getPhysicalDevice(), &memProperties);

        uint32_t memoryTypeIndex;
        findMemoryType(RENDER_DEVICE->getPhysicalDevice(), memRequirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryTypeIndex);

        allocInfo.memoryTypeIndex = memoryTypeIndex;

        if (vkAllocateMemory(device, &allocInfo, nullptr, &buffer->memory) != VK_SUCCESS) {
            aout << "Failed to allocate buffer memory!" << std::endl;
            return false;
        }

        /* Bind Buffer Memory */
        if (vkBindBufferMemory(device, buffer->buffer, buffer->memory, 0) != VK_SUCCESS) {
            aout << "Failed to bind buffer memory!" << std::endl;
            return false;
        }

        buffer->size = size;

        if (sizeOut != nullptr) {
            *sizeOut = size;
        }

        /* Create Buffer Info */
        buffer->descriptor.buffer = buffer->buffer;
        buffer->descriptor.offset = 0;
        buffer->descriptor.range = VK_WHOLE_SIZE;

        return true;
    }

    /* Create Staging Buffer */
    inline static bool
    createStagingBuffer(const void *data, uint32_t dataSize, VkBufferUsageFlagBits usage,
                        GPUBuffer *buffer) {
        VkDevice device = RENDER_DEVICE->getDevice();

        GPUBuffer stagingBuffer;
        if (!createBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, nullptr, &stagingBuffer)) {
            aout << "Failed to create staging buffer!" << std::endl;
            return false;
        }

        if (data != nullptr) {
            void *dataPtr;
            vkMapMemory(device, stagingBuffer.memory, 0, stagingBuffer.size, 0, &dataPtr);
            memcpy(dataPtr, data, dataSize);
            vkUnmapMemory(device, stagingBuffer.memory);
        }

        if (!createBuffer(dataSize, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr, buffer)) {
            aout << "Failed to create buffer!" << std::endl;
            return false;
        }

        /* Copy Staging Buffer to Device Local Buffer */
        VkCommandBuffer commandBuffer;
        allocateCommandBuffer(RENDER_DEVICE->getPrimaryCommandPool(), commandBuffer);
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            aout << "Failed to begin recording command buffer!" << std::endl;
            return false;
        }

        /* Define Copy parameters to copy from staging buffer to final buffer */
        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = dataSize;

        /* Copy Data from Staging Buffer to Final Buffer */
        vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, buffer->buffer, 1, &copyRegion);

        /* End Recording */
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            aout << "Failed to record command buffer!" << std::endl;
            return false;
        }

        /* Submit Command Buffer to Queue */
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        if (vkQueueSubmit(RENDER_DEVICE->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) !=
            VK_SUCCESS) {
            aout << "Failed to submit command buffer!" << std::endl;
            return false;
        }

        /* Wait for Queue to Finish */
        vkQueueWaitIdle(RENDER_DEVICE->getGraphicsQueue());

        /* Free Staging Buffer */
        vkFreeMemory(device, stagingBuffer.memory, nullptr);
        vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);

        return true;
    }

    inline static bool
    createGpuTexture(const void *data, uint32_t dataSize, VkFormat format, int width, int height,
                     std::shared_ptr<GPUTexture> texture) {
        VkDevice device = RENDER_DEVICE->getDevice();

        if (!texture) {
            texture = std::make_shared<GPUTexture>();
        }
        texture->image.format = format;
        texture->width = width;
        texture->height = height;

        auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(RENDER_DEVICE->getPhysicalDevice(), format,
                                            &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
            aout << "Texture image format does not support sampling!" << std::endl;
            return false;
        }

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)) {
            aout << "Texture image format does not support storage!" << std::endl;
            return false;
        }

        GPUBuffer stagingBuffer;
        VkDeviceSize bufferSize;

        if (!createBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &bufferSize, &stagingBuffer)) {
            aout << "Failed to create staging buffer!" << std::endl;
            return false;
        }

        /* Upload data to staging buffer */
        if (data != nullptr) {
            void *dataPtr;
            vkMapMemory(device, stagingBuffer.memory, 0, stagingBuffer.size, 0, &dataPtr);
            memcpy(dataPtr, data, dataSize);
            vkUnmapMemory(device, stagingBuffer.memory);
        }

        /* Create Optimal tiled target image */
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                          VK_IMAGE_USAGE_STORAGE_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &texture->image.image) != VK_SUCCESS) {
            aout << "Failed to create image!" << std::endl;
            return false;
        }

        /* Get Memory requirements */
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, texture->image.image, &memRequirements);

        /* Allocate Device Memory */
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;

        findMemoryType(RENDER_DEVICE->getPhysicalDevice(), memRequirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocInfo.memoryTypeIndex);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &texture->image.memory) != VK_SUCCESS) {
            aout << "Failed to allocate image memory!" << std::endl;
            return false;
        }

        /* Bind Image Memory */
        if (vkBindImageMemory(device, texture->image.image, texture->image.memory, 0) !=
            VK_SUCCESS) {
            aout << "Failed to bind image memory!" << std::endl;
            return false;
        }

        VkCommandBuffer copyCommandBuffer;
        allocateCommandBuffer(RENDER_DEVICE->getPrimaryCommandPool(), copyCommandBuffer);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(copyCommandBuffer, &beginInfo) != VK_SUCCESS) {
            aout << "Failed to begin recording command buffer!" << std::endl;
            return false;
        }

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = mipLevels;

        setImageLayout(copyCommandBuffer, texture->image.image, VK_IMAGE_ASPECT_COLOR_BIT,
                       VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       subresourceRange);

        /* Copy Data from Staging Buffer to Final Buffer */
        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.bufferOffset = 0;
        bufferCopyRegion.bufferRowLength = 0;
        bufferCopyRegion.bufferImageHeight = 0;
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = 0;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageOffset = {0, 0, 0};
        bufferCopyRegion.imageExtent = {texture->width, texture->height, 1};

        vkCmdCopyBufferToImage(copyCommandBuffer, stagingBuffer.buffer, texture->image.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

        setImageLayout(copyCommandBuffer, texture->image.image, VK_IMAGE_ASPECT_COLOR_BIT,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       subresourceRange, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        /* End Recording */
        if (vkEndCommandBuffer(copyCommandBuffer) != VK_SUCCESS) {
            aout << "Failed to record command buffer!" << std::endl;
            return false;
        }

        /* Submit Command Buffer to Queue */
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &copyCommandBuffer;

        if (vkQueueSubmit(RENDER_DEVICE->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) !=
            VK_SUCCESS) {
            aout << "Failed to submit command buffer!" << std::endl;
            return false;
        }

        /* */
        vkQueueWaitIdle(RENDER_DEVICE->getGraphicsQueue());

        vkFreeCommandBuffers(RENDER_DEVICE->getDevice(), RENDER_DEVICE->getPrimaryCommandPool(), 1,
                             &copyCommandBuffer);

        /* Generate Mipmap chain */
        VkCommandBuffer blitCommandBuffer;
        allocateCommandBuffer(RENDER_DEVICE->getPrimaryCommandPool(), blitCommandBuffer);
        if (!beginCommandBuffer(blitCommandBuffer, nullptr)) {
            aout << "Failed to begin recording command buffer!" << std::endl;
            return false;
        }

        for (uint32_t i = 1; i < mipLevels; i++) {
            VkImageBlit imageBlit = {};

            /* source */
            imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.srcSubresource.mipLevel = i - 1;
            imageBlit.srcSubresource.baseArrayLayer = 0;
            imageBlit.srcSubresource.layerCount = 1;
            imageBlit.srcOffsets[1].x = std::max(1u, texture->width >> i);
            imageBlit.srcOffsets[1].y = std::max(1u, texture->height >> i);
            imageBlit.srcOffsets[1].z = 1;

            /* Destination */
            imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.dstSubresource.mipLevel = i;
            imageBlit.dstSubresource.baseArrayLayer = 0;
            imageBlit.dstSubresource.layerCount = 1;
            imageBlit.dstOffsets[1].x = std::max(1u, texture->width >> (i - 1));
            imageBlit.dstOffsets[1].y = std::max(1u, texture->height >> (i - 1));
            imageBlit.dstOffsets[1].z = 1;

            VkImageSubresourceRange subresourceRangeMipmap = {};
            subresourceRangeMipmap.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRangeMipmap.baseMipLevel = i;
            subresourceRangeMipmap.levelCount = 1;
            subresourceRangeMipmap.layerCount = 1;

            setImageLayout(blitCommandBuffer, texture->image.image, VK_IMAGE_ASPECT_COLOR_BIT,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRangeMipmap,
                           VK_PIPELINE_STAGE_TRANSFER_BIT);

            vkCmdBlitImage(blitCommandBuffer, texture->image.image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->image.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

            setImageLayout(blitCommandBuffer, texture->image.image, VK_IMAGE_ASPECT_COLOR_BIT,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           subresourceRangeMipmap, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }

        subresourceRange.levelCount = mipLevels;

        setImageLayout(blitCommandBuffer, texture->image.image, 0,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       subresourceRange, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        vkEndCommandBuffer(blitCommandBuffer);

        /* Submit copy command buffer */

        submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &blitCommandBuffer;

        vkQueueSubmit(RENDER_DEVICE->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(RENDER_DEVICE->getGraphicsQueue());

        vkFreeCommandBuffers(RENDER_DEVICE->getDevice(), RENDER_DEVICE->getPrimaryCommandPool(), 1,
                             &blitCommandBuffer);

        /* Create Image view */
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.image = texture->image.image;
        viewInfo.format = texture->image.format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.flags = 0;

        viewInfo.image = texture->image.image;

        if (vkCreateImageView(device, &viewInfo, nullptr, &texture->image.imageView) != VK_SUCCESS) {
            aout << "Failed to create image view!" << std::endl;
            return false;
        }

        /* Create Sampler */
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(mipLevels);
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &texture->sampler) != VK_SUCCESS) {
            aout << "Failed to create sampler!" << std::endl;
            return false;
            }

        texture->descriptor.sampler = texture->sampler;
        texture->descriptor.imageView = texture->image.imageView;
        texture->descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        return true;
    }
};


#endif //OXYOUS_2026_RENDERFRAMEWORK_HPP
