#include "Renderer.h"
#include "Mesh.h"
#include "Util.h"
#include "BehPipelines.h"

#include <chrono>
#include <set>
#include <map>
#include <stdexcept>
#include <iostream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define VMA_IMPLEMENTATION

#include "vk_mem_alloc.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "BehFrameInfo.h"

Renderer::Renderer(std::shared_ptr<WindowWrapper> window, std::shared_ptr<BehDevice> device, AssetManager &assetManager)
        : window(window), device{device}, assetManager(assetManager) {
    try {
        descriptorAllocator.init(device->device());
        descriptorLayoutCache.init(device->device());
        mainDeletionQueue.push_function([&]() {
            descriptorLayoutCache.cleanup();
            descriptorAllocator.cleanup();
        });
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGlobalDescriptorSetLayout();
        createMaterialDescriptorSetLayout();
        createComputeDescriptorSetLayout();
        createGraphicsPipelineLayout();
        createBillboardPipelineLayout();
        createComputePipelineLayout();
        createPipelines();
        createCommandPool();
        initImgui();
        createColorResources();
        createDepthResources();
        createFramebuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }
    catch (const std::exception &e) {
        cleanup();
        std::cerr << "Renderer failed to initialize!" << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        throw e;
    }
}

Renderer::~Renderer() {
    cleanup();
}

Material Renderer::createMaterial(std::vector<std::string> &texturePaths) {
    std::vector<std::shared_ptr<UploadedTexture>> textures;
    std::map<std::string, std::shared_ptr<UploadedTexture>> uploadedTextures;
    for (const auto &filename: texturePaths) {
        textures.push_back(assetManager.getTexture(filename));
    }


    std::vector<vk::DescriptorImageInfo> imageInfos;
    for (const auto &texture: textures) {
        vk::DescriptorImageInfo imageInfo{
                .sampler = texture->textureSampler,
                .imageView = texture->textureImageView,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        };
        imageInfos.push_back(imageInfo);
    }

    Material material{};

    auto textureResult = DescriptorSetBuilder::begin(materialDescriptorSetLayout, &descriptorAllocator)
            .bindImages(0, imageInfos, vk::DescriptorType::eCombinedImageSampler)
            .build(material.textureSet);

    if (!textureResult) {
        throw std::runtime_error("Failed to create Material");
    }

    return material;
}

void Renderer::createSwapChain() {
    const SwapChainSupportDetails swapChainSupport = device->swapChainSupport();

    const vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    const vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    const vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    swapChainImageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        swapChainImageCount > swapChainSupport.capabilities.maxImageCount) {
        swapChainImageCount = swapChainSupport.capabilities.maxImageCount;
    }

	swapChainSupportedFlags = swapChainSupport.capabilities.supportedUsageFlags;
    vk::SwapchainCreateInfoKHR createInfo{
            .surface = device->surface(),
            .minImageCount = swapChainImageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | (vk::ImageUsageFlagBits::eTransferSrc & swapChainSupportedFlags)
    };


    QueueFamilyIndices indices = device->queueFamilies();
    uint32_t queueFamilyIndices[] = {indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsAndComputeFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = oldSwapChain;

    try {
        swapChain = device->device().createSwapchainKHR(createInfo);
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to create swap chain!") + err.what());
    }

    swapChainImages = device->device().getSwapchainImagesKHR(swapChain);

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void Renderer::recreateSwapchain() {
    auto timeStart = std::chrono::high_resolution_clock::now();

    oldSwapChain = swapChain;
    createSwapChain();
    device->device().destroySwapchainKHR(oldSwapChain);
    oldSwapChain = vk::SwapchainKHR{VK_NULL_HANDLE};

    oldDeleteLaterQueues.push_back(std::move(deleteLaterQueue));
    deleteLaterQueue = DeletionQueue();

    createImageViews();
    createColorResources();
    createDepthResources();
    createFramebuffers();

    imagesInFlight.resize(swapChainImages.size(), nullptr);
    awaitingClean = true;
    awaitingSwapchainImageUsed = 0;
    for (int i = 0; i < swapChainImages.size(); i++) {
        awaitingSwapchainImageUsed |= 1UL << i;
    }
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> delta = now - timeStart;
    std::cout << "Recreated swapchain in " << delta.count() << "ms" << std::endl;
}

vk::SurfaceFormatKHR
Renderer::chooseSwapSurfaceFormat(const std::vector<struct vk::SurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat: availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace ==
                                                                   vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    // Just return the first one if we cant find the specified one, it should be okay.
    return availableFormats[0];
}

vk::PresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<enum vk::PresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode: availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            // Vertical sync with less latency, by discarding old frames.
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo; // Standard Vsync, Guaranteed by Vulkan to be available.
}

vk::Extent2D Renderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        const auto[width, height] = window->getFramebufferSize();

        vk::Extent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void Renderer::initImgui() {
    //1: create descriptor pool for IMGUI
    // the size of the pool is very oversize, but it's copied from imgui demo itself.
    vk::DescriptorPoolSize pool_sizes[] =
            {
                    {vk::DescriptorType::eSampler,              1000},
                    {vk::DescriptorType::eCombinedImageSampler, 1000},
                    {vk::DescriptorType::eSampledImage,         1000},
                    {vk::DescriptorType::eStorageImage,         1000},
                    {vk::DescriptorType::eUniformTexelBuffer,   1000},
                    {vk::DescriptorType::eStorageTexelBuffer,   1000},
                    {vk::DescriptorType::eUniformBuffer,        1000},
                    {vk::DescriptorType::eStorageBuffer,        1000},
                    {vk::DescriptorType::eUniformBufferDynamic, 1000},
                    {vk::DescriptorType::eStorageBufferDynamic, 1000},
                    {vk::DescriptorType::eInputAttachment,      1000}
            };

    vk::DescriptorPoolCreateInfo pool_info{
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = 1000,
            .poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes)),
            .pPoolSizes = pool_sizes
    };

    imguiPool = device->device().createDescriptorPool(pool_info);

    //this initializes the core structures of imgui
    ImGui::CreateContext();

    //this initializes imgui for Glfw
    ImGui_ImplGlfw_InitForVulkan(window->getGLFWwindow(), true);

    //this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = device->instance();
    init_info.PhysicalDevice = device->physicalDevice();
    init_info.Device = device->device();
    init_info.Queue = device->graphicsQueue();
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = device->swapChainSupport().capabilities.minImageCount;
    init_info.ImageCount = swapChainImageCount;
    init_info.MSAASamples = static_cast<VkSampleCountFlagBits>(device->msaaSamples());

    ImGui_ImplVulkan_Init(&init_info, renderPass);

    //execute a gpu command to upload imgui font textures
    device->immediateSubmit([&](auto cmd) {
        ImGui_ImplVulkan_CreateFontsTexture(cmd);
    });

    //clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();
    mainDeletionQueue.push_function([&]() {
        device->device().destroyDescriptorPool(imguiPool);
        ImGui_ImplVulkan_Shutdown();
    });
}

void Renderer::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(device->device(), swapChainImages[i], swapChainImageFormat,
                                                 vk::ImageAspectFlagBits::eColor, 1);
        deleteLaterQueue.push_function([&, imageView = swapChainImageViews[i]]() {
            device->device().destroyImageView(imageView);
        });
    }
}

void Renderer::createRenderPass() {
    vk::AttachmentDescription colorAttachment{
            .format = swapChainImageFormat,
            .samples = device->msaaSamples(),
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::eColorAttachmentOptimal
    };

    vk::AttachmentReference colorAttachmentRef{
            .attachment = 0,
            .layout = vk::ImageLayout::eColorAttachmentOptimal
    };

    vk::AttachmentDescription depthAttachment{
            .format = findDepthFormat(),
            .samples = device->msaaSamples(),
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eDontCare,
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    vk::AttachmentReference depthAttachmentRef{
            .attachment = 1,
            .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    vk::AttachmentDescription colorAttachmentResolve{
            .format = swapChainImageFormat,
            .samples = vk::SampleCountFlagBits::e1,
            .loadOp = vk::AttachmentLoadOp::eDontCare,
            .storeOp = vk::AttachmentStoreOp::eDontCare,
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::ePresentSrcKHR
    };

    vk::AttachmentReference colorAttachmentResolveRef{
            .attachment = 2,
            .layout = vk::ImageLayout::eColorAttachmentOptimal
    };

    vk::SubpassDescription subpass{
            .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
            .pResolveAttachments = &colorAttachmentResolveRef,
            .pDepthStencilAttachment = &depthAttachmentRef
    };

    vk::SubpassDependency dependency{
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests |
                            vk::PipelineStageFlagBits::eLateFragmentTests,
            .dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests |
                            vk::PipelineStageFlagBits::eLateFragmentTests,
            .srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            .dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                             vk::AccessFlagBits::eDepthStencilAttachmentRead
    };

    std::array<vk::AttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};

    const vk::RenderPassCreateInfo renderPassInfo{
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency
    };

    try {
        renderPass = device->device().createRenderPass(renderPassInfo);
        mainDeletionQueue.push_function([&]() {
            device->device().destroyRenderPass(renderPass);
        });
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to create render pass!") + err.what());
    }

}

void Renderer::createGlobalDescriptorSetLayout() {
    auto success = DescriptorSetLayoutBuilder::begin(&descriptorLayoutCache)
            .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAllGraphics)
            .build(uboDescriptorSetLayout);

    if (!success) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}


void Renderer::createMaterialDescriptorSetLayout() {
    auto success = DescriptorSetLayoutBuilder::begin(&descriptorLayoutCache)
            .addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 256,
                        vk::DescriptorBindingFlagBits::ePartiallyBound)
            .build(materialDescriptorSetLayout);

    if (!success) {
        throw std::runtime_error("Failed to create material descriptor set layout");
    }
}

void Renderer::createComputeDescriptorSetLayout() {
    auto success = DescriptorSetLayoutBuilder::begin(&descriptorLayoutCache)
        .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
        .addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
        .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
        .build(computeDescriptorSetLayout);

    if (!success) {
        throw std::runtime_error("Failed to create compute descriptor set layout");
    }
}


void Renderer::createGraphicsPipelineLayout() {
    vk::PushConstantRange pushConstantRange{
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
            .offset = 0,
            .size = sizeof(MeshPushConstants)
    };

    std::array<vk::DescriptorSetLayout, 2> descriptorSetLayouts = {uboDescriptorSetLayout, materialDescriptorSetLayout};

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
            .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
            .pSetLayouts = descriptorSetLayouts.data(),
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange
    };

    try {
        pipelineLayout = device->device().createPipelineLayout(pipelineLayoutInfo);
        mainDeletionQueue.push_function([&]() {
            device->device().destroyPipelineLayout(pipelineLayout);
        });
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to create pipeline layout!") + err.what());
    }
}

void Renderer::createBillboardPipelineLayout() {
    std::array<vk::DescriptorSetLayout, 1> descriptorSetLayouts = {uboDescriptorSetLayout};

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
            .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
            .pSetLayouts = descriptorSetLayouts.data()
    };

    try {
        billboardPipelineLayout = device->device().createPipelineLayout(pipelineLayoutInfo);
        mainDeletionQueue.push_function([&]() {
            device->device().destroyPipelineLayout(billboardPipelineLayout);
        });
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to create billboard pipeline layout!") + err.what());
    }
}

void Renderer::createComputePipelineLayout() {
    std::array<vk::DescriptorSetLayout, 2> descriptorSetLayouts = {computeDescriptorSetLayout, uboDescriptorSetLayout};

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
        .pSetLayouts = descriptorSetLayouts.data()
    };

    try {
        computePipelineLayout = device->device().createPipelineLayout(pipelineLayoutInfo);
        mainDeletionQueue.push_function([&]() {
            device->device().destroyPipelineLayout(computePipelineLayout);
        });
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to create compute pipeline layout!") + err.what());
    }
}

void Renderer::createPipelines() {
    createGraphicsPipeline();
    createBillboardPipeline();
    createWireframePipeline();
    createComputePipeline();
}

void Renderer::createGraphicsPipeline() {
    PipelineConfigurationInfo pipelineConfig{};
    BehPipeline::defaultPipelineConfiguration(pipelineConfig);
    pipelineConfig.addShader("shaders/shader.vert.spv", vk::ShaderStageFlagBits::eVertex);
    pipelineConfig.addShader("shaders/shader.frag.spv", vk::ShaderStageFlagBits::eFragment);
    pipelineConfig.pipelineLayout = pipelineLayout;
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.extent = swapChainExtent;
    graphicsPipeline = std::make_unique<BehPipeline>(device, pipelineConfig);
}

void Renderer::createBillboardPipeline() {
    PipelineConfigurationInfo pipelineConfig{};
    BehPipeline::defaultPipelineConfiguration(pipelineConfig);
    pipelineConfig.attributeDescriptions.clear();
    pipelineConfig.bindingDescriptions.clear();
    pipelineConfig.addShader("shaders/point_light.vert.spv", vk::ShaderStageFlagBits::eVertex);
    pipelineConfig.addShader("shaders/point_light.frag.spv", vk::ShaderStageFlagBits::eFragment);
    pipelineConfig.pipelineLayout = billboardPipelineLayout;
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.extent = swapChainExtent;
    billboardPipeline = std::make_unique<BehPipeline>(device, pipelineConfig);
}

void Renderer::createWireframePipeline() {
    PipelineConfigurationInfo pipelineConfig{};
    BehPipeline::defaultPipelineConfiguration(pipelineConfig);
    pipelineConfig.addShader("shaders/shader_unlit.vert.spv", vk::ShaderStageFlagBits::eVertex);
    pipelineConfig.addShader("shaders/shader_unlit.frag.spv", vk::ShaderStageFlagBits::eFragment);
    pipelineConfig.pipelineLayout = pipelineLayout;
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.extent = swapChainExtent;

    pipelineConfig.polygonMode = vk::PolygonMode::eLine;

    wireframePipeline = std::make_unique<BehPipeline>(device, pipelineConfig);
}

void Renderer::createComputePipeline() {
    PipelineConfigurationInfo pipelineConfig{};
    pipelineConfig.pipelineLayout;

    pipelineConfig.addShader("shaders/particles.comp.spv", vk::ShaderStageFlagBits::eCompute);
}

vk::ShaderModule Renderer::createShaderModule(const std::vector<char> &code) {
    const vk::ShaderModuleCreateInfo createInfo{
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t *>(code.data())
    };

    vk::ShaderModule shaderModule;
    try {
        shaderModule = device->device().createShaderModule(createInfo);
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("Failed to create shader module!") + err.what());
    }

    return shaderModule;
}

void Renderer::createFramebuffers() {
    vk::FramebufferAttachmentImageInfo depthAttach{
            .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
            .width = swapChainExtent.width,
            .height = swapChainExtent.height,
            .layerCount = 1,
            .viewFormatCount = 1,
            .pViewFormats = &depthFormat,
    };


    vk::FramebufferAttachmentImageInfo colorAttach{
            .usage = vk::ImageUsageFlagBits::eColorAttachment |
                     vk::ImageUsageFlagBits::eTransientAttachment,
            .width = swapChainExtent.width,
            .height = swapChainExtent.height,
            .layerCount = 1,
            .viewFormatCount = 1,
            .pViewFormats = &swapChainImageFormat,
    };

    vk::FramebufferAttachmentImageInfo colorAttachResolve{
            .usage = vk::ImageUsageFlagBits::eColorAttachment | (vk::ImageUsageFlagBits::eTransferSrc & swapChainSupportedFlags),
            .width = swapChainExtent.width,
            .height = swapChainExtent.height,
            .layerCount = 1,
            .viewFormatCount = 1,
            .pViewFormats = &swapChainImageFormat,
    };

    std::vector<vk::FramebufferAttachmentImageInfo> attachments = {
            colorAttach,
            depthAttach,
            colorAttachResolve
    };

    vk::FramebufferAttachmentsCreateInfo attachCreateInfo{
            .attachmentImageInfoCount   = static_cast<uint32_t>(attachments.size()),
            .pAttachmentImageInfos      = attachments.data(),
    };

    vk::FramebufferCreateInfo framebufferInfo{
            .pNext = &attachCreateInfo,
            .flags = vk::FramebufferCreateFlagBits::eImageless,
            .renderPass = renderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = nullptr,
            .width = swapChainExtent.width,
            .height = swapChainExtent.height,
            .layers = 1,
    };

    try {
        swapChainFramebuffer = device->device().createFramebuffer(framebufferInfo);
        deleteLaterQueue.push_function([&, cachedBuffer = swapChainFramebuffer]() {
            device->device().destroyFramebuffer(cachedBuffer);
        });
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("Failed to create framebuffer") + err.what());
    }
}

void Renderer::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = device->queueFamilies();
    const vk::CommandPoolCreateInfo poolInfo{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value()
    };

    const vk::CommandPoolCreateInfo transferPoolInfo{
            .queueFamilyIndex = queueFamilyIndices.transferFamily.value(),
    };

    try {
        commandPool = device->device().createCommandPool(poolInfo);
        transferCommandPool = device->device().createCommandPool(transferPoolInfo);
        mainDeletionQueue.push_function([&]() {
            device->device().destroyCommandPool(commandPool);
            device->device().destroyCommandPool(transferCommandPool);
        });
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("Failed to create graphics command pools!") + err.what());
    }
}

void Renderer::createColorResources() {
    auto colorFormat = swapChainImageFormat;

    colorImage = assetManager.createImage(
            swapChainExtent.width,
            swapChainExtent.height,
            1,
            device->msaaSamples(),
            colorFormat,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment
    );
    colorImageView = createImageView(device->device(), colorImage._image, colorFormat, vk::ImageAspectFlagBits::eColor,
                                     1);
    deleteLaterQueue.push_function([&, cachedImage = colorImage, cachedView = colorImageView]() {
        device->device().destroyImageView(cachedView);
        vmaDestroyImage(device->allocator(), cachedImage._image, cachedImage._allocation);
    });
}

void Renderer::createDepthResources() {
    depthFormat = findDepthFormat();
    depthImage = assetManager.createImage(
            swapChainExtent.width,
            swapChainExtent.height,
            1,
            device->msaaSamples(),
            depthFormat,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment
    );
    depthImageView = createImageView(device->device(), depthImage._image, depthFormat, vk::ImageAspectFlagBits::eDepth,
                                     1);

    transitionImageLayout(depthImage._image, depthFormat, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);
    deleteLaterQueue.push_function([&, cachedImage = depthImage, cachedView = depthImageView]() {
        device->device().destroyImageView(cachedView);
        vmaDestroyImage(device->allocator(), cachedImage._image, cachedImage._allocation);
    });
}

vk::Format Renderer::findDepthFormat() {
    return findSupportedFormat(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

bool Renderer::hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

vk::Format Renderer::findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling,
                                         vk::FormatFeatureFlags features) {
    for (auto &format: candidates) {
        auto props = device->physicalDevice().getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format!");
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const {
    vk::PhysicalDeviceMemoryProperties memoryProperties;
    device->physicalDevice().getMemoryProperties(&memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void Renderer::uploadMeshes(const std::vector<std::shared_ptr<RenderObject>> &objects) {
    for (const auto &model: objects) {
        model->mesh->_vertexBuffer = uploadBuffer(model->mesh->_vertices,
                                                  VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        model->mesh->_indexBuffer = uploadBuffer(model->mesh->_indices,
                                                 VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        mainDeletionQueue.push_function([&, model]() {
            vmaDestroyBuffer(device->allocator(), model->mesh->_vertexBuffer._buffer,
                             model->mesh->_vertexBuffer._allocation);
            vmaDestroyBuffer(device->allocator(), model->mesh->_indexBuffer._buffer,
                             model->mesh->_indexBuffer._allocation);
        });
        model->material = createMaterial(model->mesh->_texturePaths);
    }
}

template<typename T>
AllocatedBuffer Renderer::uploadBuffer(std::vector<T> &meshData, VkBufferUsageFlags usage) {
    VkBufferCreateInfo stagingCreate{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = meshData.size() * sizeof(T),
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    };

    VmaAllocationCreateInfo stagingAlloc{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
    };

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;

    if (vmaCreateBuffer(device->allocator(), &stagingCreate, &stagingAlloc, &stagingBuffer, &stagingAllocation,
                        nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to uploadBuffer mesh vertices!");
    }

    void *data;
    vmaMapMemory(device->allocator(), stagingAllocation, &data);
    {
        memcpy(data, meshData.data(), meshData.size() * sizeof(T));
    }
    vmaUnmapMemory(device->allocator(), stagingAllocation);

    VkBufferCreateInfo bufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = meshData.size() * sizeof(T),
            .usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    };

    VmaAllocationCreateInfo vmaAllocInfo{
            .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };

    VkBuffer buffer;

    AllocatedBuffer allocatedBuffer{};

    if (vmaCreateBuffer(device->allocator(), &bufferCreateInfo, &vmaAllocInfo, &buffer, &allocatedBuffer._allocation,
                        nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to upload buffer!");
    }
    allocatedBuffer._buffer = buffer;

    copyBuffer(stagingBuffer, buffer, meshData.size() * sizeof(T));

    vmaDestroyBuffer(device->allocator(), stagingBuffer, stagingAllocation);
    return allocatedBuffer;
}

void Renderer::createUniformBuffers() {
    uniformBuffers.resize(swapChainImages.size());
    VkBufferCreateInfo create{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(GlobalUbo),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    };

    VmaAllocationCreateInfo allocCreate{
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkBuffer buffer;
        VmaAllocation allocation{};
        if (vmaCreateBuffer(device->allocator(), &create, &allocCreate, &buffer, &allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to upload buffer!");
        }
        uniformBuffers[i]._buffer = buffer;
        uniformBuffers[i]._allocation = allocation;
        mainDeletionQueue.push_function([&, i]() {
            vmaDestroyBuffer(device->allocator(), uniformBuffers[i]._buffer, uniformBuffers[i]._allocation);
        });
    }
}

void Renderer::createDescriptorPool() {
    std::array<vk::DescriptorPoolSize, 2> poolSizes{
            vk::DescriptorPoolSize{
                    .type = vk::DescriptorType::eUniformBuffer,
                    .descriptorCount = static_cast<uint32_t>(swapChainImages.size())
            },
            vk::DescriptorPoolSize{
                    .type = vk::DescriptorType::eCombinedImageSampler,
                    .descriptorCount = static_cast<uint32_t>(swapChainImages.size())
            }
    };

    vk::DescriptorPoolCreateInfo poolInfo{
            .maxSets = static_cast<uint32_t>(swapChainImages.size()) +
                       100, // This is an extremely bad way to do this. TODO Fix it
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data(),
    };

    if (device->device().createDescriptorPool(&poolInfo, nullptr, &descriptorPool) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create descriptor pool");
    }
    mainDeletionQueue.push_function([&]() {
        device->device().destroyDescriptorPool(descriptorPool, nullptr);
    });
}

void Renderer::createDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), uboDescriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = descriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(swapChainImages.size()),
            .pSetLayouts = layouts.data()
    };

    descriptorSets.resize(swapChainImages.size());
    if (device->device().allocateDescriptorSets(&allocInfo, descriptorSets.data()) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to create Descriptor sets!");
    }


    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vk::DescriptorBufferInfo bufferInfo{
                .buffer = uniformBuffers[i]._buffer,
                .offset = 0,
                .range = sizeof(GlobalUbo)
        };

        std::array<vk::WriteDescriptorSet, 1> descriptorWrites{
                vk::WriteDescriptorSet{
                        .dstSet = descriptorSets[i],
                        .dstBinding = 0,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = vk::DescriptorType::eUniformBuffer,
                        .pBufferInfo = &bufferInfo,
                }
        };

        device->device().updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
                                              0, nullptr);
    }
}

void Renderer::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
    device->immediateSubmit([&](vk::CommandBuffer cmd) {
        vk::BufferCopy copyRegion{
                .size = size
        };
        cmd.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
    });
}

void Renderer::createCommandBuffers() {
    commandBuffers.resize(swapChainImages.size());

    vk::CommandBufferAllocateInfo allocateInfo{
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = static_cast<uint32_t>(commandBuffers.size()),
    };

    try {
        commandBuffers = device->device().allocateCommandBuffers(allocateInfo);
        mainDeletionQueue.push_function([&]() {
            device->device().freeCommandBuffers(commandPool, commandBuffers);
        });
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to allocate command buffers") + err.what());
    }
}

void Renderer::recordCommandBuffer(uint32_t index, FrameInfo &frameInfo) {
    vk::CommandBufferBeginInfo beginInfo{};

    try {
        commandBuffers[index].begin(beginInfo);
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to begin recording command buffer!") + err.what());
    }

    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0].color = {std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

    vk::Rect2D scissor{
            .offset = {0, 0},
            .extent = swapChainExtent
    };

    commandBuffers[index].setScissor(0, 1, &scissor);

    vk::Viewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(swapChainExtent.width),
            .height = static_cast<float>(swapChainExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
    };

    commandBuffers[index].setViewport(0, 1, &viewport);

    std::vector<vk::ImageView> attachments {
            colorImageView,
            depthImageView,
            swapChainImageViews[index],
    };

    vk::RenderPassAttachmentBeginInfo attachBeginInfo{
            .attachmentCount    = static_cast<uint32_t>(attachments.size()),
            .pAttachments       = attachments.data(),
    };

    vk::RenderPassBeginInfo renderPassInfo{
            .pNext = &attachBeginInfo,
            .renderPass = renderPass,
            .framebuffer = swapChainFramebuffer,
            .renderArea = {
                    .offset = {0, 0},
                    .extent = swapChainExtent,
            },
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues = clearValues.data()
    };

    commandBuffers[index].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    {
        //Add commands to buffer
        if (rendererMode == NORMAL) {
            graphicsPipeline->bind(commandBuffers[index]);
        } else if (rendererMode == WIREFRAME) {
            wireframePipeline->bind(commandBuffers[index]);
        }
        commandBuffers[index].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1,
                                                 &descriptorSets[index], 0, nullptr);

        for (const auto &model: frameInfo.objects) {
            vk::Buffer vertexBuffers[] = {model->mesh->_vertexBuffer._buffer};
            vk::DeviceSize offsets[] = {0};
            commandBuffers[index].bindVertexBuffers(0, 1, vertexBuffers, offsets);

            commandBuffers[index].bindIndexBuffer(model->mesh->_indexBuffer._buffer, 0, vk::IndexType::eUint32);

            commandBuffers[index].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, 1,
                                                     &model->material.textureSet, 0, nullptr);

            MeshPushConstants constants = model->transformMatrix;
            commandBuffers[index].pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0,
                                                sizeof(MeshPushConstants), &constants);

            commandBuffers[index].drawIndexed(static_cast<uint32_t>(model->mesh->_indices.size()), 1, 0, 0, 0);
        }

        // Point lights
        billboardPipeline->bind(commandBuffers[index]);
        commandBuffers[index].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, billboardPipelineLayout, 0, 1,
                                                 &descriptorSets[index], 0, nullptr);
        commandBuffers[index].draw(6, 1, 0, 0);
    }

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[index]);

    commandBuffers[index].endRenderPass();

    try {
        commandBuffers[index].end();
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to record command buffer!") + err.what());
    }
}

void Renderer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainImages.size(), nullptr);

    vk::SemaphoreCreateInfo semaphoreInfo{};

    vk::FenceCreateInfo fenceInfo{.flags = vk::FenceCreateFlagBits::eSignaled};

    try {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            imageAvailableSemaphores[i] = device->device().createSemaphore(semaphoreInfo);
            renderFinishedSemaphores[i] = device->device().createSemaphore(semaphoreInfo);
            inFlightFences[i] = device->device().createFence(fenceInfo);
            mainDeletionQueue.push_function([&, i]() {
                device->device().destroySemaphore(renderFinishedSemaphores[i]);
                device->device().destroySemaphore(imageAvailableSemaphores[i]);
                device->device().destroyFence(inFlightFences[i]);
            });
        }
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("Failed to create synchronization objects for a frame!") + err.what());
    }
}

void Renderer::updateUniformBuffer(uint32_t currentImage, FrameInfo &frameInfo) {
    GlobalUbo ubo{
            .view = frameInfo.camera.viewMatrix(),
            .proj = frameInfo.camera.getCameraProjection(static_cast<float>(swapChainExtent.width),
                                                         static_cast<float>(swapChainExtent.height))
    };
    ubo.projView = ubo.proj * ubo.view;

    void *data;
    vmaMapMemory(device->allocator(), uniformBuffers[currentImage]._allocation, &data);
    {
        memcpy(data, &ubo, sizeof(ubo));
    }
    vmaUnmapMemory(device->allocator(), uniformBuffers[currentImage]._allocation);
}

int Renderer::drawFrame(FrameInfo &frameInfo) {

    while (device->device().waitForFences(inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max())
           == vk::Result::eTimeout) {
        // Wait
    }

    uint32_t imageIndex;
    try {
        const auto acquired =
                device->device().acquireNextImageKHR(swapChain,
                                                     std::numeric_limits<uint64_t>::max(),
                                                     imageAvailableSemaphores[currentFrame],
                                                     nullptr);

        switch (acquired.result) {
            case vk::Result::eSuccess:
            case vk::Result::eSuboptimalKHR: // Swap chain is still valid, but surface properties are a miss match
                imageIndex = acquired.value;
                break;
            case vk::Result::eErrorOutOfDateKHR:
                return 1;
            default:
                throw std::runtime_error("failed to acquire swap chain image!");
        }


    }
    catch (vk::OutOfDateKHRError err) { // https://github.com/KhronosGroup/Vulkan-Hpp/issues/599
        // recreateSwapchain();
        return 1;
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to acquire swap chain image!") + err.what());
    }

    if (imagesInFlight[imageIndex]) {
        while (device->device().waitForFences(imagesInFlight[imageIndex], VK_TRUE, std::numeric_limits<uint64_t>::max())
               == vk::Result::eTimeout) {
            // Wait
        }
    }

    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    ImGui::Render();

    vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

    updateUniformBuffer(imageIndex, frameInfo);

    recordCommandBuffer(imageIndex, frameInfo);

    vk::SubmitInfo submitInfo{

            .waitSemaphoreCount = 1,
            .pWaitSemaphores = waitSemaphores,
            .pWaitDstStageMask = waitStages,

            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffers[imageIndex],

            .signalSemaphoreCount = 1,
            .pSignalSemaphores = signalSemaphores,

    };

    device->device().resetFences(inFlightFences[currentFrame]);

    try {
        device->graphicsQueue().submit(submitInfo, inFlightFences[currentFrame]);
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to submit draw command buffer") + err.what());

    }

    vk::SwapchainKHR swapChains[] = {swapChain};
    const vk::PresentInfoKHR presentInfo{

            .waitSemaphoreCount = 1,
            .pWaitSemaphores = signalSemaphores,

            .swapchainCount = 1,
            .pSwapchains = swapChains,
            .pImageIndices = &imageIndex,
    };

    try {
        switch (device->presentQueue().presentKHR(presentInfo)) {
            case vk::Result::eSuccess:
                break;
            case vk::Result::eSuboptimalKHR:
            case vk::Result::eErrorOutOfDateKHR: // What we would like to do :) But it's actually an exception
                return 1;
            default:
                throw std::runtime_error("failed to present swap chain image!");  // an unexpected result is returned!
        }
    } catch (vk::OutOfDateKHRError &err) {
        return 1;
    }

    awaitingSwapchainImageUsed &= ~(1UL << imageIndex);
    // TODO: Check this on another thread
    if (!awaitingSwapchainImageUsed && awaitingClean) {
        while (!oldDeleteLaterQueues.empty()) {
			oldDeleteLaterQueues.back().flush();
			oldDeleteLaterQueues.pop_back();
        }
        awaitingClean = false;
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return 0;
}

void Renderer::cleanup() {
    for (auto& deletionQueue : oldDeleteLaterQueues) {
        deletionQueue.flush();
    }
    deleteLaterQueue.flush();
    device->device().destroySwapchainKHR(swapChain);
    mainDeletionQueue.flush();
}

void Renderer::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
                                     vk::ImageLayout newLayout, uint32_t mipLevels) {
    vk::ImageMemoryBarrier barrier{
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = vk::ImageSubresourceRange{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = mipLevels,
                    .baseArrayLayer = 0,
                    .layerCount = 1
            }
    };

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    } else {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
               newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (oldLayout == vk::ImageLayout::eUndefined &&
               newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.dstAccessMask =
                vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        throw std::invalid_argument("Unsupported layout transition!");
    }

    device->immediateSubmit([&](auto cmd) {
        cmd.pipelineBarrier(
                sourceStage, destinationStage,
                vk::DependencyFlags(),
                0, nullptr,
                0, nullptr,
                1, &barrier
        );
    });
}
