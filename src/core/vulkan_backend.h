#ifndef SRC_CORE_VULKAN_BACKEND_H
#define SRC_CORE_VULKAN_BACKEND_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace openhle {
namespace core {

class VulkanBackend {
public:
    VulkanBackend();
    ~VulkanBackend();
    bool is_ready() const { return instance != VK_NULL_HANDLE; }
    std::string get_gpu_name() const { return gpu_name; }

private:
    SDL_Window* window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    std::string gpu_name = "Unknown GPU";
};

} // namespace core
} // namespace openhle

#endif
