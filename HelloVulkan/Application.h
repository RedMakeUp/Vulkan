#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Application
{
public:
    void Run();

private:
    void InitWindow();
    void InitVulkan();
    void MainLoop();
    void Cleanup();
    void SetupDebugMassenger();
    void CreateInstance();

private:
    GLFWwindow* m_window;
    VkInstance m_vkInstance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
};