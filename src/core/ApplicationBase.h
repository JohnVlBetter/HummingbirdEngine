#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <chrono>

#include <string>
#include <sstream>
#include <array>
#include <numeric>

#include "Macros.h"
#include "Camera.hpp"
#include "Keycodes.hpp"
#include "Log.hpp"
#include "Transform.hpp"

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"

#include "imgui/imgui.h"

class ApplicationBase
{
private:	
	float fpsTimer = 0.0f;
	uint32_t frameCounter = 0;
	bool resizing = false;

	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallback;
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallback;
	VkDebugReportCallbackEXT debugReportCallback;

	struct MultisampleTarget {
		struct {
			VkImage image;
			VkImageView view;
			VkDeviceMemory memory;
		} color;
		struct {
			VkImage image;
			VkImageView view;
			VkDeviceMemory memory;
		} depth;
	} multisampleTarget;

protected:
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	VkDevice device;
	hbvk::VulkanDevice *vulkanDevice;
	VkQueue queue;
	VkFormat depthFormat;
	VkCommandPool cmdPool;
	VkRenderPass renderPass;
	std::vector<VkFramebuffer>frameBuffers;
	uint32_t currentBuffer = 0;
	VkDescriptorPool descriptorPool;
	VkPipelineCache pipelineCache;
	VulkanSwapChain swapChain;

	bool screenshotSaved = false;

	std::string title = "Hummingbird Engine";
	std::string name = "Hummingbird Engine";
public: 
	static std::vector<const char*> args;
	bool prepared = false;
	uint32_t width = 1920;
	uint32_t height = 1080; 
	std::string iconPath;
	float frameTimer = 1.0f;
	std::shared_ptr<Camera> camera;
	glm::vec2 mousePos;
	bool paused = false;
	uint32_t lastFPS = 0;

	struct Settings {
		bool validation = false;
		bool fullscreen = false;
		bool vsync = false;
		bool multiSampling = true;
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_4_BIT;
	} settings;
	
	struct DepthStencil {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	} depthStencil;

	struct GamePadState {
		glm::vec2 axisLeft = glm::vec2(0.0f);
		glm::vec2 axisRight = glm::vec2(0.0f);
	} gamePadState;

	struct MouseButtons {
		bool left = false;
		bool right = false;
		bool middle = false;
	} mouseButtons;

	void handleMouseMove(float x, float y);
	void handleMouseScroll(float delta);
	void handleDropFile(int count, const char** paths);
	void windowResize();
	uint32_t destWidth;
	uint32_t destHeight;

	GLFWwindow* glfwWindow;
	bool framebufferResized = false;
	void initWindow();

	ApplicationBase();
	virtual ~ApplicationBase();
	
	void initVulkan();

	virtual VkResult createInstance(bool enableValidation);
	virtual void render() = 0;
	virtual void windowResized();
	virtual void setupFrameBuffer();
	virtual void prepare();
	virtual void processInput();
	virtual void fileDropped(std::string filename);

	void initSwapchain();
	void setupSwapChain();

	void mainLoop();
	void renderFrame();

	void write2JPG(char const* filename, int x, int y, int comp, const void* data, int quality);
	void saveScreenshot(std::string filename);
};

#define HB_MAIN(APP_NAME)					\
APP_NAME *application;						\
int main() {								\
try {										\
	application = new APP_NAME();			\
	application->initWindow();				\
	application->initVulkan();				\
	application->prepare();					\
	application->mainLoop();				\
	delete(application);					\
}											\
catch (const std::exception& e) {			\
	LOG_ERROR(e.what());					\
	return EXIT_FAILURE;					\
}											\
return EXIT_SUCCESS;						\
}