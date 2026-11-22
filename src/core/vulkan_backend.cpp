#include "vulkan_backend.h"
#include <iostream>
#include <fstream>
#include <cstring>

namespace openhle {
namespace core {

VkShaderModule VulkanBackend::create_shader_module(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) return VK_NULL_HANDLE;
    size_t size = (size_t)file.tellg();
    std::vector<char> code(size);
    file.seekg(0);
    file.read(code.data(), size);

    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule mod = VK_NULL_HANDLE;
    vkCreateShaderModule(device, &info, nullptr, &mod);
    return mod;
}

VulkanBackend::VulkanBackend() {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) return;

    window = SDL_CreateWindow("PCps5 3D Hardware Pipeline (Vulkan / Metal)",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              640, 480, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
    if (!window) return;

    uint32_t ext_count = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &ext_count, nullptr);
    std::vector<const char*> extensions(ext_count);
    SDL_Vulkan_GetInstanceExtensions(window, &ext_count, extensions.data());
#ifdef __APPLE__
    extensions.push_back("VK_KHR_portability_enumeration");
    extensions.push_back("VK_KHR_get_physical_device_properties2");
#endif

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "PCps5 3D";
    app_info.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo inst_info{};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledExtensionCount = (uint32_t)extensions.size();
    inst_info.ppEnabledExtensionNames = extensions.data();
#ifdef __APPLE__
    inst_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    if (vkCreateInstance(&inst_info, nullptr, &instance) != VK_SUCCESS) return;
    if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) return;

    uint32_t dev_count = 0;
    vkEnumeratePhysicalDevices(instance, &dev_count, nullptr);
    if (dev_count == 0) return;
    std::vector<VkPhysicalDevice> devs(dev_count);
    vkEnumeratePhysicalDevices(instance, &dev_count, devs.data());
    physical_device = *(devs.data() + 0);

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physical_device, &props);
    std::cout << "[Vulkan 3D] GPU Pipeline Active: " << props.deviceName << "\n";

    float priority = 1.0f;
    VkDeviceQueueCreateInfo q_info{};
    q_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_info.queueFamilyIndex = 0;
    q_info.queueCount = 1;
    q_info.pQueuePriorities = &priority;

    std::vector<const char*> dev_exts = {"VK_KHR_swapchain"};
#ifdef __APPLE__
    dev_exts.push_back("VK_KHR_portability_subset");
#endif

    VkDeviceCreateInfo dev_info{};
    dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_info.queueCreateInfoCount = 1;
    dev_info.pQueueCreateInfos = &q_info;
    dev_info.enabledExtensionCount = (uint32_t)dev_exts.size();
    dev_info.ppEnabledExtensionNames = dev_exts.data();
    if (vkCreateDevice(physical_device, &dev_info, nullptr, &device) != VK_SUCCESS) return;
    vkGetDeviceQueue(device, 0, 0, &graphics_queue);

    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &caps);
    extent = caps.currentExtent;

    VkSwapchainCreateInfoKHR sc_info{};
    sc_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sc_info.surface = surface;
    sc_info.minImageCount = 2;
    sc_info.imageFormat = swapchain_format;
    sc_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sc_info.imageExtent = extent;
    sc_info.imageArrayLayers = 1;
    sc_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sc_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sc_info.preTransform = caps.currentTransform;
    sc_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sc_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sc_info.clipped = VK_TRUE;
    if (vkCreateSwapchainKHR(device, &sc_info, nullptr, &swapchain) != VK_SUCCESS) return;

    uint32_t img_count = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &img_count, nullptr);
    swapchain_images.resize(img_count);
    vkGetSwapchainImagesKHR(device, swapchain, &img_count, swapchain_images.data());

    swapchain_views.resize(img_count);
    for (uint32_t i = 0; i < img_count; ++i) {
        VkImageViewCreateInfo iv{};
        iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv.image = *(swapchain_images.data() + i);
        iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv.format = swapchain_format;
        iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv.subresourceRange.levelCount = 1;
        iv.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &iv, nullptr, swapchain_views.data() + i);
    }

    VkAttachmentDescription color_att{};
    color_att.format = swapchain_format;
    color_att.samples = VK_SAMPLE_COUNT_1_BIT;
    color_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;

    VkRenderPassCreateInfo rp_info{};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.attachmentCount = 1;
    rp_info.pAttachments = &color_att;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    vkCreateRenderPass(device, &rp_info, nullptr, &render_pass);

    VkShaderModule vert_mod = create_shader_module("shaders/vert.spv");
    VkShaderModule frag_mod = create_shader_module("shaders/frag.spv");
    if (!vert_mod || !frag_mod) {
        std::cerr << "[Vulkan 3D] Failed to load SPIR-V shaders from shaders/\n";
        return;
    }

    std::vector<VkPipelineShaderStageCreateInfo> stages(2);
    stages.data()->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages.data()->stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages.data()->module = vert_mod;
    stages.data()->pName = "main";
    (stages.data() + 1)->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    (stages.data() + 1)->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    (stages.data() + 1)->module = frag_mod;
    (stages.data() + 1)->pName = "main";

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport vp{0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f};
    VkRect2D scissor{{0, 0}, extent};
    VkPipelineViewportStateCreateInfo vps{};
    vps.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vps.viewportCount = 1;
    vps.pViewports = &vp;
    vps.scissorCount = 1;
    vps.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rast{};
    rast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast.polygonMode = VK_POLYGON_MODE_FILL;
    rast.lineWidth = 1.0f;
    rast.cullMode = VK_CULL_MODE_NONE;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState cba{};
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments = &cba;

    VkPipelineLayoutCreateInfo pl_info{};
    pl_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vkCreatePipelineLayout(device, &pl_info, nullptr, &pipeline_layout);

    VkGraphicsPipelineCreateInfo gp_info{};
    gp_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp_info.stageCount = 2;
    gp_info.pStages = stages.data();
    gp_info.pVertexInputState = &vi;
    gp_info.pInputAssemblyState = &ia;
    gp_info.pViewportState = &vps;
    gp_info.pRasterizationState = &rast;
    gp_info.pMultisampleState = &ms;
    gp_info.pColorBlendState = &cb;
    gp_info.layout = pipeline_layout;
    gp_info.renderPass = render_pass;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gp_info, nullptr, &graphics_pipeline);

    vkDestroyShaderModule(device, frag_mod, nullptr);
    vkDestroyShaderModule(device, vert_mod, nullptr);

    framebuffers.resize(img_count);
    for (uint32_t i = 0; i < img_count; ++i) {
        VkImageView att = *(swapchain_views.data() + i);
        VkFramebufferCreateInfo fb_info{};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.renderPass = render_pass;
        fb_info.attachmentCount = 1;
        fb_info.pAttachments = &att;
        fb_info.width = extent.width;
        fb_info.height = extent.height;
        fb_info.layers = 1;
        vkCreateFramebuffer(device, &fb_info, nullptr, framebuffers.data() + i);
    }

    VkCommandPoolCreateInfo cp_info{};
    cp_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cp_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(device, &cp_info, nullptr, &command_pool);
VkCommandBufferAllocateInfo cb_alloc{};
cb_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
cb_alloc.commandPool = command_pool;
cb_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
cb_alloc.commandBufferCount = 1;
vkAllocateCommandBuffers(device, &cb_alloc, &command_buffer);
VkSemaphoreCreateInfo sem_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
vkCreateSemaphore(device, &sem_info, nullptr, ℑ_available_sem);
vkCreateSemaphore(device, &sem_info, nullptr, &render_finished_sem);
vkCreateFence(device, &fence_info, nullptr, &in_flight_fence);
ready = true;
}
void VulkanBackend::draw_3d_frame() {
if (!ready) return;
vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
vkResetFences(device, 1, &in_flight_fence);
uint32_t img_idx = 0;
vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_sem, VK_NULL_HANDLE, &img_idx);
vkResetCommandBuffer(command_buffer, 0);
VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
vkBeginCommandBuffer(command_buffer, &begin_info);
VkClearValue clear_color{{{0.08f, 0.05f, 0.14f, 1.0f}}};
VkRenderPassBeginInfo rp_begin{};
rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
rp_begin.renderPass = render_pass;
rp_begin.framebuffer = *(framebuffers.data() + img_idx);
rp_begin.renderArea = {{0, 0}, extent};
rp_begin.clearValueCount = 1;
rp_begin.pClearValues = &clear_color;
vkCmdBeginRenderPass(command_buffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
vkCmdDraw(command_buffer, 3, 1, 0, 0);
vkCmdEndRenderPass(command_buffer);
vkEndCommandBuffer(command_buffer);
VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
VkSubmitInfo submit{};
submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
submit.waitSemaphoreCount = 1;
submit.pWaitSemaphores = ℑ_available_sem;
submit.pWaitDstStageMask = &wait_stage;
submit.commandBufferCount = 1;
submit.pCommandBuffers = &command_buffer;
submit.signalSemaphoreCount = 1;
submit.pSignalSemaphores = &render_finished_sem;
vkQueueSubmit(graphics_queue, 1, &submit, in_flight_fence);
VkPresentInfoKHR present{};
present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
present.waitSemaphoreCount = 1;
present.pWaitSemaphores = &render_finished_sem;
present.swapchainCount = 1;
present.pSwapchains = &swapchain;
present.pImageIndices = &img_idx;
vkQueuePresentKHR(graphics_queue, &present);
}
VulkanBackend::~VulkanBackend() {
if (device != VK_NULL_HANDLE) {
vkDeviceWaitIdle(device);
vkDestroySemaphore(device, render_finished_sem, nullptr);
vkDestroySemaphore(device, image_available_sem, nullptr);
vkDestroyFence(device, in_flight_fence, nullptr);
vkDestroyCommandPool(device, command_pool, nullptr);
for (auto fb : framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
vkDestroyPipeline(device, graphics_pipeline, nullptr);
vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
vkDestroyRenderPass(device, render_pass, nullptr);
for (auto iv : swapchain_views) vkDestroyImageView(device, iv, nullptr);
vkDestroySwapchainKHR(device, swapchain, nullptr);
vkDestroyDevice(device, nullptr);
}
if (surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance, surface, nullptr);
if (instance != VK_NULL_HANDLE) vkDestroyInstance(instance, nullptr);
if (window) SDL_DestroyWindow(window);
}
} // namespace core
} // namespace openhle
