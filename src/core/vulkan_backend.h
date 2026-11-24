#ifndef SRC_CORE_VULKAN_BACKEND_H
#define SRC_CORE_VULKAN_BACKEND_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace openhle {
namespace core {

struct Vertex3D {
    float x, y, z;
    float r, g, b;
};

class VulkanBackend {
public:
    VulkanBackend();
    ~VulkanBackend();
    bool is_ready() const { return ready; }
    void draw_3d_frame(const float* mvp16);

private:
    bool ready = false;
    SDL_Window* window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D extent{640, 480};
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_views;

    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline graphics_pipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;

    VkBuffer vertex_buffer = VK_NULL_HANDLE;
    VkDeviceMemory vertex_memory = VK_NULL_HANDLE;
    uint32_t vertex_count = 0;

    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkSemaphore avail_sem = VK_NULL_HANDLE;
    VkSemaphore render_finished_sem = VK_NULL_HANDLE;
    VkFence in_flight_fence = VK_NULL_HANDLE;

    VkShaderModule create_shader_module(const std::string& path);
    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
};

} // namespace core
} // namespace openhle

#endif
