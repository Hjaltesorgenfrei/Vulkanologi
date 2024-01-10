#include "Renderer.hpp"
#include "Mesh.hpp"
#include "Util.hpp"
#include "BehPipelines.hpp"

#include <chrono>
#include <set>
#include <map>
#include <stdexcept>
#include <iostream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

#define VMA_IMPLEMENTATION

int PARTICLE_COUNT = 256 * 4;

#include <vk_mem_alloc.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include "Particle.hpp"
#include "GlobalUbo.hpp"
#include "BehVkInit.hpp"
#include "Cube.hpp"

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
        createPipelines();
        createCommandPool();
        initImgui();
        createColorResources();
        createDepthResources();
        createFramebuffers();
        createUniformBuffers();
        createComputeShaderBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createComputeDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
        skyBox = std::make_shared<RenderObject>();
        skyBox->mesh = createCubeMesh();
        skyBox->mesh->_texturePaths = {"resources/cubemap_yokohama_rgba.ktx"};
        std::vector<std::shared_ptr<RenderObject>> reee = {skyBox};
        uploadMeshes(reee);
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

uint64_t Renderer::getMemoryUsage()
{
    VmaTotalStatistics vmaStats;
    vmaCalculateStatistics(device->allocator(), &vmaStats);
    uint64_t total = 0, used = 0;

    return vmaStats.total.statistics.allocationBytes;
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
        ImGui_ImplVulkan_CreateFontsTexture();
    });

    //clear font textures from cpu data
    mainDeletionQueue.push_function([&]() {
        ImGui_ImplVulkan_DestroyFontsTexture();
        device->device().destroyDescriptorPool(imguiPool);
        ImGui_ImplVulkan_Shutdown();
    });

    // Enable docking
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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


void Renderer::createPipelineLayout(vk::PipelineLayout& pipelineLayout, std::vector<vk::DescriptorSetLayout> descriptorSetLayouts, std::vector<vk::PushConstantRange> pushConstantranges) {
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
            .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
            .pSetLayouts = descriptorSetLayouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(pushConstantranges.size()),
            .pPushConstantRanges = pushConstantranges.data()
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

void Renderer::createPipelines() {
    createPipelineLayout(graphicsPipelineLayout, {uboDescriptorSetLayout, materialDescriptorSetLayout}, {{
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = sizeof(MeshPushConstants)
    }}); 
    createPipelineLayout(billboardPipelineLayout, {uboDescriptorSetLayout},{});
    createPipelineLayout(computePipelineLayout, {computeDescriptorSetLayout, uboDescriptorSetLayout},{});
    createPipelineLayout(skyboxPipelineLayout, {uboDescriptorSetLayout, materialDescriptorSetLayout},{});

    createGraphicsPipeline();
    createBillboardPipeline();
    createParticlePipeline();
    createWireframePipeline();
    createLinePipeline();
    createComputePipeline();
    createSkyboxPipeline();
}

void Renderer::createGraphicsPipeline() {
    PipelineConfigurationInfo pipelineConfig{};
    BehPipeline::defaultPipelineConfiguration(pipelineConfig);
    pipelineConfig.addShader("shaders/shader.vert.spv", vk::ShaderStageFlagBits::eVertex);
    pipelineConfig.addShader("shaders/shader.frag.spv", vk::ShaderStageFlagBits::eFragment);
    pipelineConfig.pipelineLayout = graphicsPipelineLayout;
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

void Renderer::createSkyboxPipeline() {
    PipelineConfigurationInfo pipelineConfig{};
    BehPipeline::defaultPipelineConfiguration(pipelineConfig);
    
    pipelineConfig.attributeDescriptions = {
            vk::VertexInputAttributeDescription{
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32B32A32Sfloat,
                .offset = 0
            }
    };
    pipelineConfig.bindingDescriptions = {
        vk::VertexInputBindingDescription{
            .binding = 0,
            .stride = sizeof(glm::vec3),
            .inputRate = vk::VertexInputRate::eVertex
        }
    };
    pipelineConfig.cullMode = vk::CullModeFlagBits::eFront; // Done instead of swapping the cube inside out.
    pipelineConfig.addShader("shaders/skybox.vert.spv", vk::ShaderStageFlagBits::eVertex);
    pipelineConfig.addShader("shaders/skybox.frag.spv", vk::ShaderStageFlagBits::eFragment);
    pipelineConfig.pipelineLayout = skyboxPipelineLayout;
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.extent = swapChainExtent;
    skyboxPipeline = std::make_unique<BehPipeline>(device, pipelineConfig);
}

void Renderer::createParticlePipeline() {
    PipelineConfigurationInfo pipelineConfig{};
    BehPipeline::defaultPipelineConfiguration(pipelineConfig);
    pipelineConfig.attributeDescriptions = Particle::getAttributeDescriptions();
    pipelineConfig.bindingDescriptions = Particle::getBindingDescriptions();
    pipelineConfig.addShader("shaders/particle.vert.spv", vk::ShaderStageFlagBits::eVertex);
    pipelineConfig.addShader("shaders/particle.frag.spv", vk::ShaderStageFlagBits::eFragment);
    pipelineConfig.topology = vk::PrimitiveTopology::ePointList;
    pipelineConfig.colorBlendAttachment = {
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
            .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .dstAlphaBlendFactor = vk::BlendFactor::eZero,
            .alphaBlendOp = vk::BlendOp::eSubtract,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };
    pipelineConfig.pipelineLayout = billboardPipelineLayout;
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.extent = swapChainExtent;
    particlePipeline = std::make_unique<BehPipeline>(device, pipelineConfig);
}

void Renderer::createWireframePipeline() {
    PipelineConfigurationInfo pipelineConfig{};
    BehPipeline::defaultPipelineConfiguration(pipelineConfig);
    pipelineConfig.addShader("shaders/shader_unlit.vert.spv", vk::ShaderStageFlagBits::eVertex);
    pipelineConfig.addShader("shaders/shader_unlit.frag.spv", vk::ShaderStageFlagBits::eFragment);
    pipelineConfig.pipelineLayout = graphicsPipelineLayout;
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.extent = swapChainExtent;

    pipelineConfig.polygonMode = vk::PolygonMode::eLine;

    wireframePipeline = std::make_unique<BehPipeline>(device, pipelineConfig);
}

void Renderer::createLinePipeline() {
    lineVertexBuffer = assetManager.allocatePersistentBuffer<Point>(1000000, vk::BufferUsageFlagBits::eVertexBuffer);
    lineIndexBuffer = assetManager.allocatePersistentBuffer<uint32_t>(1000000 * 2, vk::BufferUsageFlagBits::eIndexBuffer);

    PipelineConfigurationInfo pipelineConfig{};
    BehPipeline::defaultPipelineConfiguration(pipelineConfig);
    pipelineConfig.attributeDescriptions = Point::getAttributeDescriptions();
    pipelineConfig.bindingDescriptions = Point::getBindingDescriptions();
    pipelineConfig.addShader("shaders/shader_unlit_line.vert.spv", vk::ShaderStageFlagBits::eVertex);
    pipelineConfig.addShader("shaders/shader_unlit_line.frag.spv", vk::ShaderStageFlagBits::eFragment);
    pipelineConfig.pipelineLayout = billboardPipelineLayout;
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.extent = swapChainExtent;
    pipelineConfig.lineWidth = 2.0f;

    pipelineConfig.polygonMode = vk::PolygonMode::eLine;
    pipelineConfig.topology = vk::PrimitiveTopology::eLineList;

    linePipeline = std::make_unique<BehPipeline>(device, pipelineConfig);
}

void Renderer::createComputePipeline() {
    PipelineConfigurationInfo pipelineConfig{};
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = computePipelineLayout;
    pipelineConfig.extent = swapChainExtent;

    pipelineConfig.addShader("shaders/particles.comp.spv", vk::ShaderStageFlagBits::eCompute);
    computePipeline = std::make_unique<BehPipeline>(device, pipelineConfig);
}

void Renderer::createComputeShaderBuffers() {
    // Initialize particles
    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initial particle positions on a circle
    std::vector<Particle> particles(PARTICLE_COUNT);
    for (auto& particle : particles) {
        float r = 0.25f * sqrt(rndDist(rndEngine));
        float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846f;
        float x = r * cos(theta) * swapChainExtent.height / swapChainExtent.width;
        float y = r * sin(theta);
        particle.position = glm::vec2(x, y);
        particle.velocity = glm::normalize(glm::vec2(x,y)) * 0.00025f;
        particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
    }

    auto usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    shaderStorageBuffers = assetManager.createBuffers<Particle>(particles, usage, swapChainImages.size());
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
        if (model->mesh->_texturePaths.empty())
        {
            // Use the default texture
            model->mesh->_texturePaths.emplace_back("resources/no_texture.png");
            for (auto& vertice : model->mesh->_vertices) {
                vertice.materialIndex = 0; // We are only uploading one texture, so we dont want to index out of it
            }
        }
        model->mesh->_vertexBuffer = assetManager.createBuffer<Vertex>(model->mesh->_vertices, vk::BufferUsageFlagBits::eVertexBuffer);
        model->mesh->_indexBuffer = assetManager.createBuffer<uint32_t>(model->mesh->_indices, vk::BufferUsageFlagBits::eIndexBuffer);
        model->material = createMaterial(model->mesh->_texturePaths);
    }  
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
        uniformBuffers[i] = assetManager.allocatePersistentBuffer<GlobalUbo>(1, vk::BufferUsageFlagBits::eUniformBuffer);
    }
}

void Renderer::createDescriptorPool() {
    std::array<vk::DescriptorPoolSize, 3> poolSizes{
            vk::DescriptorPoolSize{
                    .type = vk::DescriptorType::eUniformBuffer,
                    .descriptorCount = static_cast<uint32_t>(swapChainImages.size())
            },
            vk::DescriptorPoolSize{
                    .type = vk::DescriptorType::eCombinedImageSampler,
                    .descriptorCount = static_cast<uint32_t>(swapChainImages.size())
            },
            vk::DescriptorPoolSize{
                .type = vk::DescriptorType::eStorageBuffer,
                .descriptorCount = static_cast<uint32_t>(swapChainImages.size() * 2)
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
    descriptorSets.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vk::DescriptorBufferInfo bufferInfo{
                .buffer = uniformBuffers[i]->_buffer,
                .offset = 0,
                .range = sizeof(GlobalUbo)
        };

        auto result = DescriptorSetBuilder::begin(uboDescriptorSetLayout, &descriptorAllocator)
            .bindBuffer(0, &bufferInfo, vk::DescriptorType::eUniformBuffer)
            .build(descriptorSets[i]);

        if (!result) {
            throw std::runtime_error("Failed to create descriptor set!");
        }
    }
}

void Renderer::createComputeDescriptorSets() {
    computeDescriptorSets.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vk::DescriptorBufferInfo bufferInfo {
            .buffer = uniformBuffers[i]->_buffer,
            .offset = 0,
            .range = sizeof(GlobalUbo)
        };

        vk::DescriptorBufferInfo lastFrameStorageBuffer {
            .buffer = shaderStorageBuffers[(i - 1) % swapChainImages.size()]->_buffer,
            .offset = 0,
            .range = sizeof(Particle) * PARTICLE_COUNT
        };

        vk::DescriptorBufferInfo thisFrameStorageBuffer {
            .buffer = shaderStorageBuffers[i]->_buffer,
            .offset = 0,
            .range = sizeof(Particle) * PARTICLE_COUNT
        };

        auto result = DescriptorSetBuilder::begin(computeDescriptorSetLayout, &descriptorAllocator)
            .bindBuffer(0, &bufferInfo, vk::DescriptorType::eUniformBuffer)
            .bindBuffer(1, &lastFrameStorageBuffer, vk::DescriptorType::eStorageBuffer)
            .bindBuffer(2, &thisFrameStorageBuffer, vk::DescriptorType::eStorageBuffer)
            .build(computeDescriptorSets[i]);

        if (!result) {
            throw std::runtime_error("Failed to create descriptor set!");
        }
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
    computeCommandBuffers.resize(swapChainImages.size());

    vk::CommandBufferAllocateInfo allocateInfo{
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = static_cast<uint32_t>(commandBuffers.size()),
    };

    try {
        commandBuffers = device->device().allocateCommandBuffers(allocateInfo);
        computeCommandBuffers = device->device().allocateCommandBuffers(allocateInfo);
        mainDeletionQueue.push_function([&]() {
            device->device().freeCommandBuffers(commandPool, commandBuffers);
            device->device().freeCommandBuffers(commandPool, computeCommandBuffers);
        });
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to allocate command buffers") + err.what());
    }
}

void Renderer::recordComputeCommandBuffer(vk::CommandBuffer &commandBuffer, FrameInfo &frameInfo) {
    vk::CommandBufferBeginInfo beginInfo{};

    try {
        commandBuffer.begin(beginInfo);
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to begin recording compute command buffer!") + err.what());
    }

    computePipeline->bind(commandBuffer, vk::PipelineBindPoint::eCompute);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, computePipelineLayout, 0, 1, &computeDescriptorSets[currentFrame], 0, nullptr);

    commandBuffer.dispatch(PARTICLE_COUNT / 256, 1, 1);

    try {
        commandBuffer.end();
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to record compute command buffer!") + err.what());
    }
}

void Renderer::recordCommandBuffer(vk::CommandBuffer &commandBuffer, size_t index, FrameInfo &frameInfo) {
    vk::CommandBufferBeginInfo beginInfo{};

    try {
        commandBuffer.begin(beginInfo);
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

    commandBuffer.setScissor(0, 1, &scissor);

    vk::Viewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(swapChainExtent.width),
            .height = static_cast<float>(swapChainExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
    };

    commandBuffer.setViewport(0, 1, &viewport);

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

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    {
        //Add commands to buffer
        if (displaySkybox) {
            skyboxPipeline->bind(commandBuffer);
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skyboxPipelineLayout, 0, 1,
                                                &descriptorSets[currentFrame], 0, nullptr);
            
            vk::Buffer vertexBuffers[] = {skyBox->mesh->_vertexBuffer->_buffer};
            vk::DeviceSize offsets[] = {0};
            commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

            commandBuffer.bindIndexBuffer(skyBox->mesh->_indexBuffer->_buffer, 0, vk::IndexType::eUint32);

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skyboxPipelineLayout, 1, 1,
                                                     &skyBox->material.textureSet, 0, nullptr);


            commandBuffer.drawIndexed(static_cast<uint32_t>(skyBox->mesh->_indices.size()), 1, 0, 0, 0);
        }
        
        if (rendererMode == NORMAL) {
            graphicsPipeline->bind(commandBuffer);
        } else if (rendererMode == WIREFRAME) {
            wireframePipeline->bind(commandBuffer);
        }
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphicsPipelineLayout, 0, 1,
                                                 &descriptorSets[currentFrame], 0, nullptr);

        for (const auto &model: frameInfo.objects) {
            vk::Buffer vertexBuffers[] = {model->mesh->_vertexBuffer->_buffer};
            vk::DeviceSize offsets[] = {0};
            commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

            commandBuffer.bindIndexBuffer(model->mesh->_indexBuffer->_buffer, 0, vk::IndexType::eUint32);

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphicsPipelineLayout, 1, 1,
                                                     &model->material.textureSet, 0, nullptr);

            MeshPushConstants constants = model->transformMatrix;
            commandBuffer.pushConstants(graphicsPipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0,
                                                sizeof(MeshPushConstants), &constants);

            commandBuffer.drawIndexed(static_cast<uint32_t>(model->mesh->_indices.size()), 1, 0, 0, 0);
        }
        
        // Point lights
        billboardPipeline->bind(commandBuffer);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, billboardPipelineLayout, 0, 1,
                                                 &descriptorSets[currentFrame], 0, nullptr);
        commandBuffer.draw(6, 1, 0, 0);

        if (shouldDrawComputeParticles) {
            particlePipeline->bind(commandBuffer);
            
            vk::DeviceSize offsets[] = {0};
            commandBuffer.bindVertexBuffers(0, 1, &shaderStorageBuffers[currentFrame]->_buffer, offsets);
            commandBuffer.draw(PARTICLE_COUNT, 1, 0, 0);
        }

        //  Draw Lines
        linePipeline->bind(commandBuffer);
        
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, billboardPipelineLayout, 0, 1,
                                                 &descriptorSets[currentFrame], 0, nullptr);
        
        vk::DeviceSize offsets[] = {0};

        commandBuffer.bindVertexBuffers(0, 1, &lineVertexBuffer->_buffer, offsets);

        commandBuffer.bindIndexBuffer(lineIndexBuffer->_buffer, 0, vk::IndexType::eUint32);

        size_t vertexOffset = 0, indexOffset = 0;
        for (const auto &path : frameInfo.paths) {
            for (const auto &point : path.getPoints()) {
                lineVertexBuffer->at(vertexOffset++) = point;
            }
            for (const auto &index : path.getIndices()) {
                lineIndexBuffer->at(indexOffset++) = index;
            }

            auto indexBackCount = static_cast<uint32_t>(indexOffset - path.getIndices().size());
            auto vertexBackCount = static_cast<int32_t>(vertexOffset - path.getPoints().size());


            commandBuffer.drawIndexed(
                static_cast<uint32_t>(path.getIndices().size()), 
                1, 
                indexBackCount, 
                vertexBackCount, 
                0
            );
        }

    }

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    commandBuffer.endRenderPass();

    try {
        commandBuffer.end();
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
    computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo semaphoreInfo{};

    vk::FenceCreateInfo fenceInfo{.flags = vk::FenceCreateFlagBits::eSignaled};

    try {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            imageAvailableSemaphores[i] = device->device().createSemaphore(semaphoreInfo);
            renderFinishedSemaphores[i] = device->device().createSemaphore(semaphoreInfo);
            computeFinishedSemaphores[i] = device->device().createSemaphore(semaphoreInfo);
            inFlightFences[i] = device->device().createFence(fenceInfo);
            computeInFlightFences[i] = device->device().createFence(fenceInfo);
            mainDeletionQueue.push_function([&, i]() {
                device->device().destroySemaphore(renderFinishedSemaphores[i]);
                device->device().destroySemaphore(imageAvailableSemaphores[i]);
                device->device().destroySemaphore(computeFinishedSemaphores[i]);
                device->device().destroyFence(inFlightFences[i]);
                device->device().destroyFence(computeInFlightFences[i]);
            });
        }
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("Failed to create synchronization objects for a frame!") + err.what());
    }
}

void Renderer::updateUniformBuffer(size_t currentImage, FrameInfo &frameInfo) {
    GlobalUbo ubo{
            .view = frameInfo.camera.viewMatrix(),
            .proj = frameInfo.camera.getCameraProjection(static_cast<float>(swapChainExtent.width),
                                                         static_cast<float>(swapChainExtent.height))
    };
    ubo.projView = ubo.proj * ubo.view;
    ubo.deltaTime = frameInfo.deltaTime;
    auto light = frameInfo.lights.front();
    ubo.lightPosition = glm::vec4(light.position, light.isDirectional ? 0.0f : 1.0f);
    ubo.lightColor = glm::vec4(light.color, light.intensity);

    uniformBuffers[currentImage]->at(0) = ubo;
}

int Renderer::drawFrame(FrameInfo &frameInfo) {

    while (device->device().waitForFences(computeInFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max())
           == vk::Result::eTimeout) {
        // Wait
    }

    updateUniformBuffer(currentFrame, frameInfo);

    device->device().resetFences(computeInFlightFences[currentFrame]);

    computeCommandBuffers[currentFrame].reset();
    recordComputeCommandBuffer(computeCommandBuffers[currentFrame], frameInfo);

    vk::SubmitInfo computeSubmitInfo{
            .commandBufferCount = 1,
            .pCommandBuffers = &computeCommandBuffers[currentFrame],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &computeFinishedSemaphores[currentFrame],
    };

    try {
        device->computeQueue().submit(computeSubmitInfo, computeInFlightFences[currentFrame]);
    }
    catch (vk::SystemError &err) {
        throw std::runtime_error(std::string("failed to submit compute command buffer") + err.what());
    }

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

    while (device->device().waitForFences(inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max())
            == vk::Result::eTimeout) {
        // Wait
    }

    ImGui::Render();

    std::vector<vk::Semaphore> waitSemaphores = {computeFinishedSemaphores[currentFrame], imageAvailableSemaphores[currentFrame] };
    std::vector<vk::PipelineStageFlags> waitStages = {vk::PipelineStageFlagBits::eVertexInput, vk::PipelineStageFlagBits::eColorAttachmentOutput};
    std::vector<vk::Semaphore> signalSemaphores = {renderFinishedSemaphores[currentFrame]};

    commandBuffers[currentFrame].reset();
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex, frameInfo);

    vk::SubmitInfo submitInfo{

            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphores = waitSemaphores.data(),
            .pWaitDstStageMask = waitStages.data(),

            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffers[currentFrame],

            .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
            .pSignalSemaphores = signalSemaphores.data(),

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

            .waitSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
            .pWaitSemaphores = signalSemaphores.data(),

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
    } catch (vk::OutOfDateKHRError) {
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
