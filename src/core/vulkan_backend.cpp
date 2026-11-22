#include "vulkan_backend.h"
#include <iostream>

namespace openhle {
namespace core {

VulkanBackend::VulkanBackend() {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        std::cerr << "[Vulkan] SDL_INIT_VIDEO failed: " << SDL_GetError() << "\n";
        return;
    }

    window = SDL_CreateWindow("PCps5 3D Hardware Renderer (Vulkan / Metal)",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              640, 480, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "[Vulkan] Failed to create SDL Vulkan window: " << SDL_GetError() << "\n";
        return;
    }

    uint32_t ext_count = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &ext_count, nullptr)) {
        std::cerr << "[Vulkan] Failed to query SDL Vulkan extensions\n";
        return;
    }

    std::vector<const char*> extensions(ext_count);
    SDL_Vulkan_GetInstanceExtensions(window, &ext_count, extensions.data());

#ifdef __APPLE__
    extensions.push_back("VK_KHR_portability_enumeration");
#endif

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "PCps5 Emulator";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "PCps5 MoltenVK Backend";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

#ifdef __APPLE__
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VkResult res = vkCreateInstance(&create_info, nullptr, &instance);
    if (res != VK_SUCCESS) {
        std::cerr << "[Vulkan] vkCreateInstance failed with code: " << res << "\n";
        return;
    }

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if (device_count == 0) {
        std::cerr << "[Vulkan] No Vulkan-compatible GPU found\n";
        return;
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(*(devices.data() + 0), &props);
    gpu_name = props.deviceName;

    std::cout << "[Vulkan 3D] Hardware GPU Initialized: " << gpu_name
              << " (Vulkan " << VK_VERSION_MAJOR(props.apiVersion) << "."
              << VK_VERSION_MINOR(props.apiVersion) << "."
              << VK_VERSION_PATCH(props.apiVersion) << " via MoltenVK)\n";
}

VulkanBackend::~VulkanBackend() {
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
}

} // namespace core
} // namespace openhle
