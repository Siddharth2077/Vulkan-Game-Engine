#pragma once

#include "vk_types.h"


/// @brief For double-buffering our commands.
constexpr unsigned int FRAME_OVERLAP {2};

/// @brief Represents the structures and commands that the engine will need to draw a given frame.
///
/// This is particularly useful when double or triple buffering the commands to keep the CPU busy.
struct FrameData {
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;
};


/// The main Vulkan Engine class.
///
/// Hold all the parameters and Vulkan handles to set up the render loop.
class VulkanEngine {
public:
	// Initializes everything in the engine
	void init();

	// Shuts down the engine
	void cleanup();

	// Draw loop
	void draw();

	// Run main loop
	void run();

	/// Getter for fetching the FrameData struct for the current frame.
	inline FrameData& get_current_frame() { return _frames.at(_frameNumber % FRAME_OVERLAP); }

private:
	bool _isInitialized{ false };
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 720 , 405 };
	int _frameNumber {0};
	std::array<FrameData, FRAME_OVERLAP> _frames{};

	struct SDL_Window* _window{ nullptr };

	// Returns the Singleton Instance of the engine.
	static VulkanEngine& Get();

	// Vulkan handles
	VkInstance _vulkanInstance{ nullptr };
	VkDebugUtilsMessengerEXT _debugMessenger;
	VkPhysicalDevice _physicalDevice{ nullptr };
	VkDevice _device{ nullptr };
	VkSurfaceKHR _surface{ nullptr };

	VkSwapchainKHR _swapchain{ nullptr };
	VkFormat _swapchainImageFormat;
	VkExtent2D _swapchainExtent;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	VkQueue _graphicsQueue{ nullptr };
	uint32_t _graphicsQueueFamilyIndex{ 0 };


	// Initialization helper methods
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();

};
