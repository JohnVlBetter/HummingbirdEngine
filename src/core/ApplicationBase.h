#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <windows.h>
#include <fcntl.h>
#include <io.h>

#include <iostream>
#include <chrono>
#include <sys/stat.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <string>
#include <sstream>
#include <array>
#include <numeric>

#include "Macros.h"
#include "Camera.hpp"
#include "Keycodes.hpp"

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"

#include "imgui/imgui.h"

class ApplicationBase
{
private:	
	float fpsTimer = 0.0f;
	uint32_t frameCounter = 0;
	uint32_t destWidth;
	uint32_t destHeight;
	bool resizing = false;
	inline bool checkKeyPress(int key);
	inline bool checkKeyRelease(int key);
	inline bool checkMouseButtonPress(int key);
	inline bool checkMouseButtonRelease(int key);
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
	vks::VulkanDevice *vulkanDevice;
	VkQueue queue;
	VkFormat depthFormat;
	VkCommandPool cmdPool;
	VkRenderPass renderPass;
	std::vector<VkFramebuffer>frameBuffers;
	uint32_t currentBuffer = 0;
	VkDescriptorPool descriptorPool;
	VkPipelineCache pipelineCache;
	VulkanSwapChain swapChain;
	std::string title = "Vulkan Example";
	std::string name = "ApplicationExample";
	void windowResize();
public: 
	static std::vector<const char*> args;
	bool prepared = false;
	uint32_t width = 1280;
	uint32_t height = 720;
	float frameTimer = 1.0f;
	Camera camera;
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

	HWND window;
	HINSTANCE windowInstance;

	HWND setupWindow(HINSTANCE hinstance, WNDPROC wndproc);
	void handleMouseMove(int32_t x, int32_t y);
	void handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
	virtual void fileDropped(std::string filename);

	void initSwapchain();
	void setupSwapChain();

	void renderLoop();
	void mainLoop();
	void processInput();
	void renderFrame();
};
