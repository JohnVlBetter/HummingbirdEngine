#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

class Application {
public:
    void Run() {
        InitWindow();
        InitVulkan();
        Tick();
        Cleanup();
    }

private:
    GLFWwindow* window;
    int width = 1920,height = 1080;

    VkInstance instance;

    void InitWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, "Hummingbird Engine", nullptr, nullptr);
    }

    void InitVulkan() {
        CreateInstance();
    }

    void Tick() {
        while (!glfwWindowShouldClose(window)) {
            LogicTick();
            RenderTick();
            glfwPollEvents();
        }
    }
    void RenderTick(){}
    void LogicTick(){}

    void Cleanup() {
        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void CreateInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Hummingbird Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vk instance!");
        }
    }

};

int main() {
    Application app;

    try {
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
