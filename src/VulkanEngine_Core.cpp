#include "VulkanEngine.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <array>
#include <memory>
#include <filesystem>
#include <algorithm>

using json = nlohmann::json;

#define VK_CHECK(x)                                                 \
    do {                                                            \
        VkResult err = x;                                           \
        if (err) {                                                  \
            std::cerr << "Detected Vulkan error: " << err << "\n";  \
            abort();                                                \
        }                                                           \
    } while (0)

// Updates the particle type definitions buffer on the GPU
void VulkanEngine::updateTypeBuffer() {
    if (typeBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, typeBuffer, typeBufferAllocation);
    }
    
    size_t typeCount = particleTypes.empty() ? 1 : particleTypes.size();
    VkBufferCreateInfo typeBufferInfo{};
    typeBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    typeBufferInfo.size = sizeof(ParticleTypeGPU) * typeCount;
    typeBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo typeAllocInfo{};
    typeAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    typeAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VK_CHECK(vmaCreateBuffer(allocator, &typeBufferInfo, &typeAllocInfo, &typeBuffer, &typeBufferAllocation, nullptr));
    
    VmaAllocationInfo vmaAllocInfo;
    vmaGetAllocationInfo(allocator, typeBufferAllocation, &vmaAllocInfo);
    
    std::vector<ParticleTypeGPU> gpuTypes(typeCount);
    if (!particleTypes.empty()) {
        for (size_t i = 0; i < particleTypes.size(); ++i) {
            gpuTypes[i].color = particleTypes[i].color;
            gpuTypes[i].size = particleTypes[i].size;
            gpuTypes[i].opacity = particleTypes[i].opacity;
            gpuTypes[i].lifeDuration = particleTypes[i].lifeDuration;
            gpuTypes[i].shape = particleTypes[i].shape;
            gpuTypes[i].fadeType = particleTypes[i].fadeType;
            gpuTypes[i].velocityProfile = particleTypes[i].velocityProfile;
        }
    } else {
        gpuTypes[0].color = glm::vec4(1.0f);
        gpuTypes[0].size = 5.0f;
        gpuTypes[0].opacity = 1.0f;
        gpuTypes[0].lifeDuration = 5.0f;
    }
    memcpy(vmaAllocInfo.pMappedData, gpuTypes.data(), sizeof(ParticleTypeGPU) * typeCount);
}

// Main application loop
void VulkanEngine::run() {
    auto lastTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        auto currentTime = glfwGetTime();
        if (isExporting) {
            int framesPerTick = 1; // You can increase this to speed up export
            for (int i = 0; i < framesPerTick && isExporting; ++i) {
                pushConstants.deltaTime = 1.0f / 60.0f;
                pushConstants.time += pushConstants.deltaTime;
                currentAnimationTime += pushConstants.deltaTime;
                
                exportVideoFrameOffscreen();
                
                if (currentAnimationTime > animationDuration) {
                    isExporting = false;
                    if (ffmpegPipe) {
                        pclose(ffmpegPipe);
                        ffmpegPipe = nullptr;
                        std::cout << "Video Export Completed!" << std::endl;
                    }
                    currentAnimationTime = 0.0f;
                    pushConstants.time = 0.0f;
                    pushConstants.reset = 1;
                }
            }
            lastTime = glfwGetTime();
        } else {
            pushConstants.deltaTime = (float)(currentTime - lastTime);
            pushConstants.time += pushConstants.deltaTime;
            lastTime = currentTime;
            
            if (!infiniteLoop && currentAnimationTime >= animationDuration) {
                pushConstants.deltaTime = 0.0f;
                currentAnimationTime = animationDuration;
            } else {
                currentAnimationTime += pushConstants.deltaTime;
                if (currentAnimationTime > animationDuration) {
                    currentAnimationTime = 0.0f;
                    pushConstants.time = 0.0f;
                    pushConstants.reset = 1;
                }
            }
        }
        
        if (framebufferResized) {
            recreateSwapchain();
            framebufferResized = false;
        }

        if (needsBufferReallocation) {
            vkDeviceWaitIdle(device);
            if (particleBuffer != VK_NULL_HANDLE) vmaDestroyBuffer(allocator, particleBuffer, particleBufferAllocation);
            createParticleBuffers();
            pushConstants.reset = 1;
            needsBufferReallocation = false;
        } else if (needsTypeBufferUpdate) {
            vkDeviceWaitIdle(device);
            updateTypeBuffer();
            needsTypeBufferUpdate = false;
        }

        renderUI();
        draw();
        
        if (pushConstants.reset == 1) {
            pushConstants.reset = 0;
            currentAnimationTime = 0.0f;
        }
    }
}

// Copies the current frame to a readback buffer and pipes it to FFmpeg
void VulkanEngine::exportVideoFrameOffscreen() {
    if (!isExporting || !ffmpegPipe) return;
    
    vkQueueWaitIdle(graphicsQueue); // Ensure previous frame is finished before resetting command buffer
    
    vkResetCommandBuffer(mainCommandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(mainCommandBuffer, &beginInfo);
    
    if (isPreviewing) {
        vkCmdBindPipeline(mainCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdBindDescriptorSets(mainCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, nullptr);
        vkCmdPushConstants(mainCommandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ComputePushConstants), &pushConstants);
        vkCmdDispatch(mainCommandBuffer, (currentParticleCount + 255) / 256, 1, 1);
        
        VkBufferMemoryBarrier bufferBarrier{};
        bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferBarrier.buffer = particleBuffer;
        bufferBarrier.offset = 0;
        bufferBarrier.size = VK_WHOLE_SIZE;
        vkCmdPipelineBarrier(mainCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
    }

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = renderPass;
    rpInfo.framebuffer = offscreenFramebuffer;
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = swapchainExtent;
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(mainCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    VkViewport viewport{};
    viewport.width = (float)swapchainExtent.width;
    viewport.height = (float)swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(mainCommandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(mainCommandBuffer, 0, 1, &scissor);
    
    if (isPreviewing) {
        vkCmdBindPipeline(mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        vkCmdBindDescriptorSets(mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &computeDescriptorSet, 0, nullptr);
        vkCmdPushConstants(mainCommandBuffer, graphicsPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ComputePushConstants), &pushConstants);
        vkCmdDraw(mainCommandBuffer, currentParticleCount, 1, 0, 0);
    }
    
    vkCmdEndRenderPass(mainCommandBuffer);
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = offscreenImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    
    vkCmdPipelineBarrier(mainCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {swapchainExtent.width, swapchainExtent.height, 1};
    
    vkCmdCopyImageToBuffer(mainCommandBuffer, offscreenImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readbackBuffer, 1, &region);
    
    VK_CHECK(vkEndCommandBuffer(mainCommandBuffer));
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mainCommandBuffer;
    
    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
    vkQueueWaitIdle(graphicsQueue);
    
    VmaAllocationInfo allocInfo;
    vmaGetAllocationInfo(allocator, readbackBufferAllocation, &allocInfo);
    if (allocInfo.pMappedData) {
        fwrite(allocInfo.pMappedData, 1, swapchainExtent.width * swapchainExtent.height * 4, ffmpegPipe);
    }
}

// Records and submits the command buffer for a single frame
void VulkanEngine::draw() {
    VK_CHECK(vkWaitForFences(device, 1, &renderFence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(device, 1, &renderFence));
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        framebufferResized = true;
        return;
    }
    
    vkResetCommandBuffer(mainCommandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(mainCommandBuffer, &beginInfo);
    
    if (isPreviewing && !isExporting) {
        vkCmdBindPipeline(mainCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdBindDescriptorSets(mainCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, nullptr);
        vkCmdPushConstants(mainCommandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ComputePushConstants), &pushConstants);
        vkCmdDispatch(mainCommandBuffer, (currentParticleCount + 255) / 256, 1, 1);
        
        VkBufferMemoryBarrier bufferBarrier{};
        bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferBarrier.buffer = particleBuffer;
        bufferBarrier.offset = 0;
        bufferBarrier.size = VK_WHOLE_SIZE;
        vkCmdPipelineBarrier(mainCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
    }

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = renderPass;
    rpInfo.framebuffer = framebuffers[imageIndex];
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = swapchainExtent;
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(mainCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    VkViewport viewport{};
    viewport.width = (float)swapchainExtent.width;
    viewport.height = (float)swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(mainCommandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(mainCommandBuffer, 0, 1, &scissor);
    
    if (isPreviewing) {
        vkCmdBindPipeline(mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        vkCmdBindDescriptorSets(mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &computeDescriptorSet, 0, nullptr);
        vkCmdPushConstants(mainCommandBuffer, graphicsPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ComputePushConstants), &pushConstants);
        vkCmdDraw(mainCommandBuffer, currentParticleCount, 1, 0, 0);
    }
    
    vkCmdEndRenderPass(mainCommandBuffer);
    
    // (Removed export video frame here as it's handled separately)
    
    VkRenderPassBeginInfo uiRpInfo{};
    uiRpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    uiRpInfo.renderPass = uiRenderPass;
    uiRpInfo.framebuffer = uiFramebuffers[imageIndex];
    uiRpInfo.renderArea.offset = {0, 0};
    uiRpInfo.renderArea.extent = swapchainExtent;
    
    vkCmdBeginRenderPass(mainCommandBuffer, &uiRpInfo, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), mainCommandBuffer);
    vkCmdEndRenderPass(mainCommandBuffer);
    
    VK_CHECK(vkEndCommandBuffer(mainCommandBuffer));
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &presentSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderSemaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mainCommandBuffer;
    
    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, renderFence));
    
    // (ffmpeg write handled in exportVideoFrameOffscreen)
    
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        framebufferResized = true;
    }
}

