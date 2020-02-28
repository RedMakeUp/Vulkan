#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>
#include <vector>


class Application
{
private:
    struct QueueFamilyIndices{
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool IsComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails{
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

public:
    // Call this function to run the program
    void Run();

private:
    // Main loop
    void MainLoop();

    // Initialize window creation
    void InitWindow();
    // Initialize Vulank staffs
    void InitVulkan();

    // Clean up all resources using by Vulkan and GLFW
    void Cleanup();
    // Clean up all objects related with swap chain
    void CleanupSwapChain();

    // Create a Vulkan instance
    void CreateInstance();

    // Look up all suitable pyhsical devices(GPUs) and pick up the first one
    void PickPhysicalDevice();
    bool IsPhysicalDeviceSuitable(VkPhysicalDevice device);

    // Loop up all queue fanilies of the @device and pick up the first suitable one
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

    // Setup debug messenger through validation layers
    void SetupDebugMassenger();

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
    void CreateLogicalDevice();
    void CreateSurface();
    VkSurfaceFormatKHR ChooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void CreateSwapChain();
    void CreateImageViews();
    void CreateDescriptorSetLayout();
    void CreateGraphicsPipeline();
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    void CreateRenderPass();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateUniformBuffers();
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void CreateSyncObjects();
    void DrawFrame();
    void RecreateSwapChain();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertices);
    void UpdateUniformBuffer(uint32_t currentImage);

private:
    // Check if the validation layers we need are supported
    static bool CheckValidationLayerSupport();
    // Check if the Vulkan extensions required by GLFW are supported by local Vulkan 
    static bool CheckGLFWExtensionSupport();
    // Check if the extensions we need for specific physical device are supported by that device 
    static bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    // Fill @createInfo with necessary debug messenger creation infomations
    static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    // Return all extensions that are actually needed for this application
    static std::vector<const char*> GetRequiredExtensions();
    // Callback function that handles messages from Validation layers
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData);

private:

    GLFWwindow* m_window;
    VkInstance m_vkInstance;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;// Describes how to access the image and which part image to access
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    VkRenderPass m_renderPass;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inflightFences;
    std::vector<VkFence> m_imagesInFlight;
    size_t m_currentFrame = 0;
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;
    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;

    bool m_frameBufferResized = false;
};