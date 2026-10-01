#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include <array>

struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
    float life;
    int typeIndex;
    float seed;
    float padding;
};

struct ParticleTypeGPU {
    glm::vec4 color;
    float size;
    float opacity;
    float lifeDuration;
    int shape;
    int fadeType;
    int velocityProfile;
    int padding1;
    int padding2;
};

struct ParticleTypeDef {
    std::string name;
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    float size{5.0f};
    float opacity{1.0f};
    float lifeDuration{5.0f};
    int shape{0};
    int fadeType{1};
    int velocityProfile{0};
    int count{10000};
};

struct ComputePushConstants {
    float deltaTime;
    float timeSpeed;
    glm::vec2 gravity;
    int globalMovement;
    int particleCount;
    int reset;
    float time;
};

class VulkanEngine {
public:
    void init();
    void run();
    void cleanup();

    bool framebufferResized = false;

private:
    void initWindow();
    void initVulkan();
    void initImGui();
    void createPipelines();
    void createParticleBuffers();
    void updateTypeBuffer();
    void draw();
    void renderUI();
    void recreateSwapchain();
    void cleanupSwapchain();
    void applyGlobalPreset(int presetIndex);
    void savePreset(const std::string& filename);
    void loadPreset(const std::string& filename);
    void saveParticleType(const std::string& filename, const ParticleTypeDef& def);
    void loadParticleType(const std::string& filename);
    void exportVideoFrame(uint32_t imageIndex);

    GLFWwindow* window{nullptr};
    bool isInitialized{false};

    // Vulkan Core
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice chosenGPU;
    VkDevice device;
    VkSurfaceKHR surface;
    
    // Swapchain
    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    // Queues
    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamily;

    // VMA
    VmaAllocator allocator;

    // Sync objects
    VkSemaphore presentSemaphore, renderSemaphore;
    VkFence renderFence;
    
    // Command pools and buffers
    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;
    
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;
    
    // Offscreen Export
    VkImage offscreenImage = VK_NULL_HANDLE;
    VmaAllocation offscreenImageAllocation = VK_NULL_HANDLE;
    VkImageView offscreenImageView = VK_NULL_HANDLE;
    VkFramebuffer offscreenFramebuffer = VK_NULL_HANDLE;
    
    void createOffscreenExportResources();
    void cleanupOffscreenExportResources();
    void exportVideoFrameOffscreen();
    
    VkRenderPass uiRenderPass;
    std::vector<VkFramebuffer> uiFramebuffers;
    
    // Pipelines
    VkPipelineLayout graphicsPipelineLayout;
    VkPipeline graphicsPipeline;
    VkPipelineLayout computePipelineLayout;
    VkPipeline computePipeline;
    
    // Descriptors
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout computeDescriptorSetLayout;
    VkDescriptorSet computeDescriptorSet = VK_NULL_HANDLE;
    
    // Buffers
    VkBuffer particleBuffer = VK_NULL_HANDLE;
    VmaAllocation particleBufferAllocation = VK_NULL_HANDLE;
    
    VkBuffer typeBuffer = VK_NULL_HANDLE;
    VmaAllocation typeBufferAllocation = VK_NULL_HANDLE;
    
    size_t currentParticleCount = 0;
    
    // UI State
    ComputePushConstants pushConstants = {};
    std::vector<ParticleTypeDef> particleTypes;
    
    bool isPreviewing = true;
    bool needsBufferReallocation = false;
    bool needsTypeBufferUpdate = false;
    
    // Animation & Loop
    bool infiniteLoop = true;
    float animationDuration = 10.0f;
    float currentAnimationTime = 0.0f;
    
    // Video Export
    bool isExporting = false;
    FILE* ffmpegPipe = nullptr;
    VkBuffer readbackBuffer = VK_NULL_HANDLE;
    VmaAllocation readbackBufferAllocation = VK_NULL_HANDLE;
    
    // Utils
    uint32_t frameNumber = 0;
    
    VkShaderModule loadShaderModule(const char* filePath);
};
