#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>

class Application
{
private:
    struct QueueFamilyIndices{
        std::optional<uint32_t> graphicsFamily;

        bool isComplete() {
            return graphicsFamily.has_value();
        }
    };

public:
    void Run();

private:
    void InitWindow();
    void InitVulkan();
    void MainLoop();
    void Cleanup();
    void SetupDebugMassenger();
    void CreateInstance();
    void PickPhysicalDevice();
    bool IsDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

private:

    GLFWwindow* m_window;
    VkInstance m_vkInstance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
};