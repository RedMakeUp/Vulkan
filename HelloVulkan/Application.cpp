#include "Application.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <string>
#include <cstring>
#include <set>
#include <cstdint>
#include <fstream>

#ifdef NDEBUG
#define ENABLE_VALIDATION_LAYERS false
#else
#define ENABLE_VALIDATION_LAYERS true
#endif

// If @result is not VK_SUCCESS, throw a std::runtime_error with description @text
#define ThrowIfFailed(result, text) if(result != VK_SUCCESS){throw std::runtime_error(text);}

struct Vertex{
    glm::vec2 Pos;
    glm::vec3 Color;

    static VkVertexInputBindingDescription GetBindingDescription(){
        VkVertexInputBindingDescription bindingDesc = {};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(Vertex);
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDesc;
    }

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescription(){
        std::array<VkVertexInputAttributeDescription, 2> attributeDescs = {};
        // Position
        attributeDescs[0].binding = 0;
        attributeDescs[0].location = 0;
        attributeDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescs[0].offset = offsetof(Vertex, Pos);
        // Color
        attributeDescs[1].binding = 0;
        attributeDescs[1].location = 1;
        attributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[1].offset = offsetof(Vertex, Color);

        return attributeDescs;
    }
};

struct UniformBufferObject{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;
const std::vector<Vertex> g_vertices = {
    {{-0.5f,-0.5f}, {1.0f,0.0f,0.0f}},
    {{0.5f,-0.5f}, {0.0f,1.0f,0.0f}},
    {{0.5f,0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,0.5f}, {0.0f,1.0f,1.0f}},
};
const std::vector<uint16_t> g_indices = {
    0, 1, 2,
    2, 3, 0
};


const std::vector<const char*> g_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> g_deviceEntensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

/////////////////////////////////////////////////////////////////////////////////
// Static global functions //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

// Be careful the file path which is relative to path of the execuable when you just run it,
// and is relative to path of CMakeLists.txt when you debug using CMake Tool in Vs code. 
std::vector<char> ReadFile(const std::string& filename){
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if(!file.is_open()){
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

template<typename T>
T Clamp(T value, T minValue, T maxValue){
    if(value > maxValue){
        return maxValue;
    }else if(value < minValue){
        return minValue;
    }else{
        return value;
    }
}


/////////////////////////////////////////////////////////////////////////////////
// Static member functions //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
bool Application::CheckValidationLayerSupport()
{
    // Get all avaliable layers in local Vulkan
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> avabliableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, avabliableLayers.data());

    // Check the inclusion between avabliableLayers we got above and g_validationLayers which we actually need
    for (const char *layerName : g_validationLayers)
    {
        bool isFound = false;

        for (const auto &layerProperties : avabliableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                isFound = true;
                break;
            }
        }

        if (!isFound)
            return false;
    }

    return true;
}

bool Application::CheckGLFWExtensionSupport()
{
    // Require extensions through GLFW
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Require extensions through Vulkan
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    // Check the inclusion between two extension list required above
    if (glfwExtensionCount > extensionCount)
    {
        return false;
    }
    else
    {
        for (uint32_t i = 0; i < glfwExtensionCount; i++)
        {
            bool isIncluded = false;
            for (const auto &ext : extensions)
            {
                if (strcmp(ext.extensionName, glfwExtensions[i]) == 0)
                {
                    isIncluded = true;
                    break;
                }
            }

            if (!isIncluded)
                return false;
        }
    }

    return true;
}

bool Application::CheckDeviceExtensionSupport(VkPhysicalDevice device){
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device,nullptr,&extensionCount,nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device,nullptr,&extensionCount,availableExtensions.data());

    std::set<std::string> requiredExtensions(g_deviceEntensions.begin(),g_deviceEntensions.end());

    for(const auto& extension: availableExtensions){
        requiredExtensions.erase(extension.extensionName);
    }
    
    return requiredExtensions.empty();
}

std::vector<const char*> Application::GetRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    #if ENABLE_VALIDATION_LAYERS
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

    return extensions;
}

void Application::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo){
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
    createInfo.pUserData = nullptr;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    std::string severityStr = "Unknown"; 
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        severityStr = "[ Diagnostic ]";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        severityStr = "[ Warning ]";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        severityStr = "[ ERROR ]";
        break;
    default:
        break;
    }
    std::cerr << "Validation layer" << severityStr << ": " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

// Output number and name of extensions required by GLFW to console
void DebugVulkanExtensionsByGLFW()
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::cout << "GLFW Extension Count: " << glfwExtensionCount << std::endl;
    for (uint32_t i = 0; i < glfwExtensionCount; i++)
    {
        std::cout << glfwExtensions[i] << std::endl;
    }
}

// Output number and name of extensions supported to console
void DebugVulkanExtensios()
{
    // Require extensions through VK
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    std::cout << "VK Extension Count: " << extensionCount << std::endl;
    for (uint32_t i = 0; i < extensionCount; i++)
    {
        std::cout << extensions[i].extensionName << std::endl;
    }
}


/////////////////////////////////////////////////////////////////////////////////
// Non-static member functions //////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
void Application::Run()
{
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();
}

void Application::InitWindow()
{
    // Initialize GLFW library
    glfwInit();

    // Check if all extensions required bt GLFW are supported
    if(!CheckGLFWExtensionSupport()) { throw std::runtime_error("Extensions required by GLFW is not validated"); }

    // This is a Vulkan application, so we don't want OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // The window can be resized
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Create main window
    m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
    // Set user pointer for the window in order to access other class members from GLFW custom callback functions
    glfwSetWindowUserPointer(m_window, this);

    // Set callback function for window resizing
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height){
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

        app->m_frameBufferResized = true;
    });
}

void Application::InitVulkan()
{
    #if ENABLE_VALIDATION_LAYERS
        if(!CheckValidationLayerSupport()) { throw std::runtime_error("Validation layers requested, but not available!"); }
    #endif

    CreateInstance();
    SetupDebugMassenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();// Using images as 2D textures
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateCommandBuffers();
    CreateSyncObjects();
}

void Application::MainLoop()
{
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
        DrawFrame();
    }

    vkDeviceWaitIdle(m_device);
}

void Application::Cleanup()
{
    CleanupSwapChain();

    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

    vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    vkFreeMemory(m_device, m_indexBufferMemory, nullptr);

    vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_device, m_inflightFences[i], nullptr);
    }

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vkDestroyDevice(m_device,nullptr);

    #if ENABLE_VALIDATION_LAYERS
        DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);
    #endif

    vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);
    vkDestroyInstance(m_vkInstance, nullptr);

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Application::CleanupSwapChain(){
    for(auto& framebuffer: m_swapChainFramebuffers) vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    for(auto& imageView: m_swapChainImageViews) vkDestroyImageView(m_device, imageView, nullptr);
    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);

    for(size_t i = 0;i < m_swapChainImages.size(); i++){
        vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
    }
}

void Application::SetupDebugMassenger()
{
    #if ENABLE_VALIDATION_LAYERS
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        PopulateDebugMessengerCreateInfo(createInfo);

        ThrowIfFailed(CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_debugMessenger),
            "Failed to set up debug messenger!");
    #endif
}

void Application::CreateInstance()
{
    // Informations about this application
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    // Informations about global extensions and validation layers we want to use
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    #if ENABLE_VALIDATION_LAYERS
        createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();

        // Create an additional debug messenger for vkCreateInstance and vkDestroyInstance and clean up after that
        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
    #endif
    auto extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Create Vulkan instance
    ThrowIfFailed(vkCreateInstance(&createInfo, nullptr, &m_vkInstance), "Failed to create instance!");
}

void Application::PickPhysicalDevice(){
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);
    if(deviceCount == 0){
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());

    for(const auto& device:devices){
        if(IsPhysicalDeviceSuitable(device)){
            m_physicalDevice = device;
            break;
        }
    }

    if(m_physicalDevice == VK_NULL_HANDLE){
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

bool Application ::IsPhysicalDeviceSuitable(VkPhysicalDevice device){
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    bool extensionSupported = CheckDeviceExtensionSupport(device);

    bool swapchainAdequate = false;
    if(extensionSupported){
        auto swapChainDetails = QuerySwapChainSupport(device);
        swapchainAdequate = !swapChainDetails.formats.empty() && !swapChainDetails.presentModes.empty();
    }

    bool result = 
            deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            deviceFeatures.geometryShader &&
            FindQueueFamilies(device).IsComplete() &&
            extensionSupported &&
            swapchainAdequate;

    if(result){
        std::cout << "Choose GPU: " << deviceProperties.deviceName << std::endl;
    }

    return result;
}

Application::QueueFamilyIndices Application::FindQueueFamilies(VkPhysicalDevice device){
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

        if(presentSupport){
            indices.presentFamily = i;
        }

        if(indices.IsComplete()) break;
        i++;
    }

    return indices;
}

void Application::CreateLogicalDevice(){
    // Specify the queue information we actually need
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    float queuePriority = 1.0;
    for(uint32_t queueFamily: uniqueQueueFamilies){
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    // Require queue families
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    // Require features
    VkPhysicalDeviceFeatures deviceFeatures = {};
    createInfo.pEnabledFeatures = &deviceFeatures;
    // Require extensions
    createInfo.enabledExtensionCount = static_cast<uint32_t>(g_deviceEntensions.size());
    createInfo.ppEnabledExtensionNames = g_deviceEntensions.data();
    // Require validation layers
    // Note: enabledLayerCount and ppEnabledLayerNames are deprecated by up-to-date implementations
    createInfo.enabledLayerCount = 0;
    #if ENABLE_VALIDATION_LAYERS
        createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();
    #endif

    // Create logical device
    ThrowIfFailed(vkCreateDevice(m_physicalDevice,&createInfo,nullptr,&m_device), 
        "Failed to create logical device!");

    // Retrieve queue handles
    // Note: For each queue family, we only need one queue
    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
}

void Application::CreateSurface(){
    ThrowIfFailed(glfwCreateWindowSurface(m_vkInstance, m_window, nullptr, &m_surface),
        "Failed to create window surface!");
}

Application::SwapChainSupportDetails Application::QuerySwapChainSupport(VkPhysicalDevice device){
    SwapChainSupportDetails details;

    // This function wiil take the specified VkPhysicalDevice and VkSurfaceKHR window surface into account when 
    // determining the supported capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    // Query the supported surface formats
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    if(formatCount != 0){
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    // Query the supported presentation mode
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    if(presentModeCount != 0){
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Application::ChooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats){
    for(const auto& availableFormat: availableFormats){
        if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Application::ChooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes){
    for(const auto& availablePresentMode: availablePresentModes){
        if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR){
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::ChooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities){
    if(capabilities.currentExtent.width != UINT32_MAX){
        return capabilities.currentExtent;
    }else{
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = Clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = Clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void Application::CreateSwapChain(){
    auto swapChainDetails = QuerySwapChainSupport(m_physicalDevice);

    auto surfaceFormat = ChooseSwapChainSurfaceFormat(swapChainDetails.formats);
    auto presentMode = ChooseSwapChainPresentMode(swapChainDetails.presentModes);
    auto extent = ChooseSwapChainExtent(swapChainDetails.capabilities);

    // Use two images at least and not exceed the maximum supported number
    uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;
    if(swapChainDetails.capabilities.maxImageCount > 0 && imageCount > swapChainDetails.capabilities.maxImageCount){
        imageCount = swapChainDetails.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;// Always 1 unless stereoscopic 3D application
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;// Directly render to these images

    // We'll be drawing on the images in the swap chain from the graphics queue and then submitting 
    // them on the presentation queue
    auto indices = FindQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyindices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    if(indices.graphicsFamily != indices.presentFamily){
        // Images can be used across multiple queue families without explicit ownership transfers
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyindices;
    }else{
        // The ownership of an image must be explicitly transfered before using it in another queue family
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    // We do not want any transformation to be applied th images in the swap chain
    createInfo.preTransform = swapChainDetails.capabilities.currentTransform;
    // We do not want the alpha channel to be used for blending with other window in the window system
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    // We don't care about the color of pixels that are obscured, for example, because another window is in front of them
    createInfo.clipped = VK_TRUE;
    // Stop all rendering before a new swap chain is created
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    ThrowIfFailed(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain),
        "Failed to create swap chain!");

    // Access these images in the swap chain
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void Application::CreateImageViews(){
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for(size_t i = 0; i < m_swapChainImages.size(); i++){
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;// Treat images as 2D textures
        createInfo.format = m_swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;// Use images as color targets
        createInfo.subresourceRange.baseMipLevel = 0;// Without any mipmapping levels
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;// Without any multiple layers
        createInfo.subresourceRange.layerCount = 1;

        ThrowIfFailed(vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]),
            "Failed to create image views!");
    }
}

void Application::CreateRenderPass(){
    // A render pass could be considerd as a wrapper of resources and operations, where resources are attachments 
    // and operations are subpass
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;// Clear framebuffer to a constant color
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subPass = {};
    subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPass.colorAttachmentCount = 1;
    subPass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subPass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    ThrowIfFailed(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass),
        "Failed to create render pass!");
}

void Application::CreateDescriptorSetLayout(){
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    ThrowIfFailed(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout),
        "Failed to create descriptor set layout!");
}

void Application::CreateGraphicsPipeline(){
    // Programmable shader stages
    auto vertShaderCode = ReadFile("shaders/vert.spv");
    auto fragShaderCode = ReadFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Fixed functions
    // Vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    auto bindingDesc = Vertex::GetBindingDescription();
    auto attributeDescs = Vertex::GetAttributeDescription();
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissors
    VkViewport viewport = {};// Scale after everything is projected
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChainExtent.width);
    viewport.height = static_cast<float>(m_swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor = {};// Clip a rectangle(pixels) inside the viewport
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainExtent;
    VkPipelineViewportStateCreateInfo viewportInfo = {};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizerInfo = {};
    rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo.depthClampEnable = VK_FALSE;
    rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerInfo.lineWidth = 1.0f;
    rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizerInfo.depthBiasEnable = VK_FALSE;
    rasterizerInfo.depthBiasConstantFactor = 0.0f;
    rasterizerInfo.depthBiasClamp = 0.0f;
    rasterizerInfo.depthBiasSlopeFactor = 0.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisamplingInfo = {};
    multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingInfo.sampleShadingEnable = VK_FALSE;
    multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisamplingInfo.minSampleShading = 1.0f;
    multisamplingInfo.pSampleMask = nullptr;
    multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
    multisamplingInfo.alphaToOneEnable = VK_FALSE;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachment;
    colorBlendInfo.blendConstants[0] = 0.0f; 
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;

    // Pipeline layout(Uniforms and push values)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    ThrowIfFailed(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout),
        "Failed to create pipeline layout!");

    // Finally, the graphics pipeline itself
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterizerInfo;
    pipelineInfo.pMultisampleState = &multisamplingInfo;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlendInfo;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    ThrowIfFailed(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline),
        "Failed to create graphics pipelines!");

    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
}

VkShaderModule Application::CreateShaderModule(const std::vector<char>& code){
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    ThrowIfFailed(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule),
        "Failed to create shader module!");

    return shaderModule;
}

void Application::CreateFramebuffers(){
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

    for(size_t i = 0; i < m_swapChainImageViews.size(); i++){
        VkImageView attachments[] = { m_swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        ThrowIfFailed(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]),
            "Failed to create framebuffer!");
    }
}

void Application::CreateCommandPool(){
    auto queueFamilyIndices = FindQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = 0;

    ThrowIfFailed(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool),
        "Failed to create command pool!");
}

uint32_t Application::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertices){
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    // typeFilter is a bitfield of which a bit represents a MemoryType that is supported by this resource
    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++){
        if((typeFilter & (1 << i)) && // So we first check if the MemoryType is supported
            (memProperties.memoryTypes[i].propertyFlags & propertices) == propertices){// And then check there are our wanted properties
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

void Application::CreateVertexBuffer(){
    VkDeviceSize bufferSize = sizeof(g_vertices[0]) * g_vertices.size();
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, g_vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_device, stagingBufferMemory);

    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_vertexBuffer, m_vertexBufferMemory);
    
    CopyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void Application::CreateIndexBuffer(){
    VkDeviceSize bufferSize = sizeof(g_indices[0]) * g_indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, g_indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_device, stagingBufferMemory);

    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_indexBuffer, m_indexBufferMemory);
    
    CopyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void Application::CreateUniformBuffers(){
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(m_swapChainImages.size());
    m_uniformBuffersMemory.resize(m_swapChainImages.size());

    for(size_t i = 0; i < m_swapChainImages.size(); i++){
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_uniformBuffers[i], m_uniformBuffersMemory[i]);
    }
}

void Application::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory){
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    ThrowIfFailed(vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer),
        "Failed to create buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    ThrowIfFailed(vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory),
        "Failed to allocate buffer memory!");

    vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

void Application::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void Application::CreateCommandBuffers(){
    m_commandBuffers.resize(m_swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    ThrowIfFailed(vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()),
        "Failed to allocate command buffers!");

    for (size_t i = 0; i < m_commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;
        ThrowIfFailed(vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo), 
            "Failed to begin recording command buffer!");

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_swapChainExtent;
        VkClearValue clearColor = {0.2f, 0.3f, 0.4f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
        
        VkBuffer vertexBuffer[] = {m_vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(m_commandBuffers[i],0,1,vertexBuffer,offsets);
        
        vkCmdBindIndexBuffer(m_commandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(m_commandBuffers[i], static_cast<uint32_t>(g_indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(m_commandBuffers[i]);

        ThrowIfFailed(vkEndCommandBuffer(m_commandBuffers[i]), 
            "Failed to record command buffer!");
    }

}

void Application::CreateSyncObjects(){
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inflightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        if(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inflightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
}

void Application::DrawFrame(){
    // Wait for the n-th frame(specified by m_currentFrame) finishing
    vkWaitForFences(m_device, 1, &m_inflightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    // Acquire an image from the swap chain
    uint32_t imageIndex;
    auto result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
    if(result == VK_ERROR_OUT_OF_DATE_KHR){
        RecreateSwapChain();
        return;
    }else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    // Check if a previous frame is using this image 
    if(m_imagesInFlight[imageIndex] != VK_NULL_HANDLE){
        vkWaitForFences(m_device, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    m_imagesInFlight[imageIndex] = m_inflightFences[m_currentFrame];

    UpdateUniformBuffer(imageIndex);

    // Submitting the command buffer
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;// Specify which semaphores to wait on before execution
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;// And in which stages of the pipeline to wait
    submitInfo.commandBufferCount = 1;// Specify which command buffers to actually submit for execution
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;// Specify which semaphores to signal once the command buffers have finished execution
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    // We manually need to restore the fence to the unsignaled state before actually using the fance
    vkResetFences(m_device, 1, &m_inflightFences[m_currentFrame]);
    
    // Submit the command buffer to the graphics queue and the fence will be signaled once the command buffer finished executing
    ThrowIfFailed(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inflightFences[m_currentFrame]),
        "Failed to submit draw command buffer!");

    // Presentation
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;// Specify which semaphores to wait on before presentation can happen
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {m_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_frameBufferResized){
        m_frameBufferResized = false;
        RecreateSwapChain();
    }else if(result != VK_SUCCESS){
        throw std::runtime_error("Failed to present swap chain image!");
    }

    // Advance to the next frame
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::UpdateUniformBuffer(uint32_t currentImage){
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f,0.0f,1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f,2.0f,2.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,0.0f,1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(m_swapChainExtent.width) / m_swapChainExtent.height, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1;

    void* data;
    vkMapMemory(m_device, m_uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(m_device, m_uniformBuffersMemory[currentImage]);
}

void Application::RecreateSwapChain(){
    // Pause the app when minimized
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width ==0 || height == 0)
    {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }
    

    // Wait untill the resources are not in use
    vkDeviceWaitIdle(m_device);

    // Make sure the old version of these objects are cleaned up before recreating them
    CleanupSwapChain();

    // Recreate the swapchain itself
    CreateSwapChain();
    // Recreate image views and render pass because they are based on the format of the swapchain images
    CreateImageViews();
    CreateRenderPass();
    // Recreate graphics pipeline because viewport and scissor retangle size are specified during its creation
    CreateGraphicsPipeline();
    // Recreate frame buffers and command buffers because they directly depend on the swap chain images
    CreateFramebuffers();
    CreateUniformBuffers();
    CreateCommandBuffers();
}
