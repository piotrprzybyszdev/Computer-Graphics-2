#include <imgui.h>
#include <glm/glm.hpp>

#include <Core/Core.h>

#include <Vulkan/Application.h>
#include <Vulkan/Renderer/FrameGraphBuilder.h>

#include "ApplicationState.h"

using namespace ref;
using namespace ref::vulkan;

DuckUserInterface::DuckUserInterface(const UserInterfaceVulkanSpec& spec, Scene& scene) : UserInterface(spec), m_Scene(scene)
{
}

void DuckUserInterface::OnDefineUI(float /* timeStep */)
{
}

void DuckUserInterface::OnKeyEvent(Key key, KeyAction action, Mods mods)
{
    m_Scene.OnKeyEvent(key, action, mods);
}

void DuckUserInterface::OnMouseButtonEvent(ref::Button button, ref::ButtonAction action, ref::Mods mods)
{
    m_Scene.OnMouseButtonEvent(button, action, mods);
}

void DuckUserInterface::OnCursorMoveEvent(double xpos, double ypos)
{
    m_Scene.OnCursorMoveEvent(xpos, ypos);
}

struct CameraConstants
{
    glm::mat4x4 Projection;
    glm::mat4x4 View;
    glm::vec4 Origin;
};

DuckApplicationState::DuckApplicationState(const ApplicationStateSpec& spec)
    : m_MainQueue(spec.Queues.at(Application::MainQueueName))
{
    ShaderId waterHeightShader = spec.ShaderLibrary->GetShaderByPath("Shaders/waterHeight.comp");
    ShaderId waterNormalShader = spec.ShaderLibrary->GetShaderByPath("Shaders/waterNormal.comp");
    ShaderId meshVertexShader = spec.ShaderLibrary->GetShaderByPath("Shaders/mesh.vert");
    ShaderId waterFragmentShader = spec.ShaderLibrary->GetShaderByPath("Shaders/water.frag");
    ShaderId duckFragmentShader = spec.ShaderLibrary->GetShaderByPath("Shaders/duck.frag");
    ShaderId environmentFragmentShader = spec.ShaderLibrary->GetShaderByPath("Shaders/environment.frag");

    {
        ComputePipelineInfo pipelineInfo = {
            .Name = "Water Height Pipeline",
            .ComputeShader = waterHeightShader,
        };

        auto heightPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        pipelineInfo = {
            .Name = "Water Normal Pipeline",
            .ComputeShader = waterNormalShader,
        };

        auto normalPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        ComputePipelineInstanceInfo pipelineInstanceInfo = {
            .Name = "Water Height Pipeline Instance",
            .PipelineId = heightPipelineId,
        };

        m_WaterHeightPipeline = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);

        pipelineInstanceInfo = {
            .Name = "Water Normal Pipeline Instance",
            .PipelineId = normalPipelineId,
        };

        m_WaterNormalPipeline = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);
    }

    {
        GraphicsPipelineInfo pipelineInfo = {
            .Name = "Water Pipeline",
            .VertexShaderId = meshVertexShader,
            .FragmentShaderId = waterFragmentShader,
        };

        auto waterPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        GraphicsPipelineInstanceInfo pipelineInstanceInfo = {
            .Name = "Water Pipeline Instance",
            .PipelineId = waterPipelineId,
            .BindingDescriptions = { vk::VertexInputBindingDescription(0, sizeof(Vertex)) },
            .VertexInputs = { { 0, offsetof(Vertex, Position) }, { 0, offsetof(Vertex, Normal) }, { 0, offsetof(Vertex, TexCoord) } },
            .ColorAttachmentFormats = { vk::Format::eR8G8B8A8Unorm },
            .DepthAttachmentFormat = vk::Format::eD24UnormS8Uint,
            .StencilAttachmentFormat = vk::Format::eD24UnormS8Uint,
        };

        pipelineInstanceInfo.InputAssemblyState.setTopology(vk::PrimitiveTopology::eTriangleList);
        pipelineInstanceInfo.RasterizationState.setLineWidth(1.0f);
        pipelineInstanceInfo.RasterizationState.setCullMode(vk::CullModeFlagBits::eBack);
        pipelineInstanceInfo.DepthStencilState.setDepthTestEnable(vk::True);
        pipelineInstanceInfo.DepthStencilState.setDepthWriteEnable(vk::True);
        pipelineInstanceInfo.DepthStencilState.setDepthCompareOp(vk::CompareOp::eLess);
        pipelineInstanceInfo.AttachmentBlendStates.emplace_back().setColorWriteMask(vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags);
        m_WaterPipeline = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);

        pipelineInfo.Name = "Duck Pipeline";
        pipelineInfo.FragmentShaderId = duckFragmentShader;

        auto duckPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);
        pipelineInstanceInfo.Name = "Duck Pipeline Instance";
        pipelineInstanceInfo.PipelineId = duckPipelineId;
        m_DuckPipeline = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);

        pipelineInfo.Name = "Environment Pipeline";
        pipelineInfo.FragmentShaderId = environmentFragmentShader;

        auto environmentPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);
        pipelineInstanceInfo.Name = "Environment Pipeline Instance";
        pipelineInstanceInfo.PipelineId = environmentPipelineId;
        m_EnvironmentPipeline = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);
    }
}

DuckApplicationState::~DuckApplicationState()
{
}

void DuckApplicationState::OnEnter(vulkan::ApplicationState* /* previous */)
{
    const auto& spec = Application::GetInstance()->GetApplicationStateSpec();

    [[maybe_unused]] bool success = spec.PipelineLibrary->CompilePipelines();
    assert(success == true);

    ResourceManagerSpec resourceManagerSpec = {
        .ApiVersion = spec.ApiVersion,
        .Instance = spec.Instance,
        .PhysicalDevice = spec.PhysicalDevice,
        .LogicalDevice = spec.LogicalDevice,
    };

    m_ResourceAllocator = std::make_unique<ResourceAllocator>(resourceManagerSpec);

    const uint32_t imageCount = std::clamp(2u, spec.SwapchainBuilder->GetMinImageCount(), spec.SwapchainBuilder->GetMaxImageCount());
    UserInterfaceVulkanSpec userInterfaceSpec = {
        .Window = spec.ApplicationWindow->GetHandle(),
        .ApiVersion = spec.ApiVersion,
        .Instance = spec.Instance,
        .PhysicalDevice = spec.PhysicalDevice,
        .LogicalDevice = spec.LogicalDevice,
        .QueueFamilyIndex = m_MainQueue.FamilyIndex,
        .Queue = m_MainQueue.Handle,
        .ImageCount = imageCount,
        .ImageFormat = vk::Format::eR8G8B8A8Unorm,
    };

    m_UserInterface = std::make_unique<DuckUserInterface>(userInterfaceSpec, m_Scene);

    FrameGraphBuilder builder;

    builder.AddDeviceImage(
        "Image", vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(1280, 720, 1), 1, 1)
        .setUsage(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment), ResourceType::Transient, true
    );
    builder.AddDeviceImage(
        "Depth Stencil Image", vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eD24UnormS8Uint, vk::Extent3D(1280, 720, 1), 1, 1)
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment), ResourceType::Transient, true
    );
    builder.AddDeviceImage(
        "Water Normal Image", vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(256, 256, 1), 1, 1)
        .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled), ResourceType::Transient, true
    );

    const auto& duckTexture = m_Scene.GetDuckTexture();
    builder.AddDeviceImage(
        "Duck Color Texture", vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(duckTexture.Width, duckTexture.Height, 1), 1, 1)
        .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled), ResourceType::Persistent, false
    );

    const auto& environmentTextures = m_Scene.GetEnvironmentTextures();
    builder.AddDeviceImage(
        "Environment Cube Texture", vk::ImageCreateInfo(vk::ImageCreateFlagBits::eCubeCompatible, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(environmentTextures.front().Width, environmentTextures.front().Height, 1), 1, 6)
        .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled),
        vk::ImageViewCreateInfo().setFormat(vk::Format::eR8G8B8A8Unorm).setViewType(vk::ImageViewType::eCube).setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6)), ResourceType::Persistent, false
    );

    builder.AddDeviceBuffer("Water Height Buffer 0", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer).setSize(256 * 256 * sizeof(float)), ResourceType::Temporal, false);
    builder.AddDeviceBuffer("Water Height Buffer 1", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer).setSize(256 * 256 * sizeof(float)), ResourceType::Temporal, false);
    builder.AddDeviceBuffer("Vertex Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer).setSize(m_Scene.GetVertices().size_bytes()), ResourceType::Persistent, false);
    builder.AddDeviceBuffer("Index Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer).setSize(m_Scene.GetIndices().size_bytes()), ResourceType::Persistent, false);

    builder.AddHostBuffer("Transform Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer).setSize(m_Scene.GetTransforms().size_bytes()), ResourceType::Persistent, true);
    builder.AddHostBuffer("Camera Uniform Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eUniformBuffer).setSize(sizeof(CameraConstants)), ResourceType::Persistent, true);

    m_TextureSampler = spec.LogicalDevice.createSampler(vk::SamplerCreateInfo().setMinFilter(vk::Filter::eLinear).setMagFilter(vk::Filter::eLinear));
    
    {
        ComputePassSpec passSpec = {
            .Pipeline = m_WaterHeightPipeline,
            .BufferBindings = {
                { "Water Height Buffer 0", 0, true, true },
                { "Water Height Buffer 1", 1, true, true },
            },
            .Dispatches = { 
                {
                    .Command = { 256 / 8, 256 / 8, 1 },
                    .PushConstantData = std::as_bytes(std::span(&m_SimulationData, 1)),
                }
            },
        };

        builder.AddComputePass("Water Height Pass", passSpec);
    }

    {
        ComputePassSpec passSpec = {
            .Pipeline = m_WaterNormalPipeline,
            .BufferBindings = {
                { "Water Height Buffer 0", 0, true, true },
                { "Water Height Buffer 1", 1, true, true },
            },
            .ImageBindings = {
                { "Water Normal Image", 2, nullptr, false, true },
            },
            .Dispatches = {
                {
                    .Command = { 256 / 8, 256 / 8, 1 },
                    .PushConstantData = std::as_bytes(std::span(&m_SimulationData.InputBufferIndex, 1)),
                }
            },
        };

        builder.AddComputePass("Water Normal Pass", passSpec);
    }

    auto getInstanceDraw = [this](uint32_t instanceIndex) {
        const auto& instance = m_Scene.GetInstances()[instanceIndex];
        const auto& mesh = m_Scene.GetMeshes()[instance.MeshIndex];
        return IndexedGraphicsPassSpec::DrawSpec {
            .Command = {
                .IndexCount = mesh.IndexCount,
                .InstanceCount = 1,
                .FirstIndex = mesh.IndexOffset,
                .VertexOffset = mesh.VertexOffset,
                .FirstInstance = 0,
            },
            .PushConstantData = std::as_bytes(std::span(&instance.TransformIndex, 1)),
        };
    };

    {
        IndexedGraphicsPassSpec passSpec = {
            .Pipeline = m_WaterPipeline,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
                { "Transform Buffer", 1, true, false },
            },
            .ImageBindings = {
                { "Water Normal Image", 2, m_TextureSampler, true, false },
            },
            .VertexBuffers = {
                .VertexBuffers = { { "Vertex Buffer" } },
            },
            .IndexBuffer = { "Index Buffer", 0, vk::IndexType::eUint32 },
            .ColorAttachments = {
                {
                    .ImageResource = "Image",
                    .LoadOp = vk::AttachmentLoadOp::eClear,
                    .ClearValue = vk::ClearColorValue(0.2f, 0.2f, 0.2f, 1.0f),
                },
            },
            .DepthAttachment = { {
                .ImageResource = "Depth Stencil Image",
                .LoadOp = vk::AttachmentLoadOp::eClear,
                .ClearValue = vk::ClearDepthStencilValue(1.0f, 0),
            } },
            .Draws = { getInstanceDraw(m_Scene.GetWaterInstanceIndex()) },
        };
        builder.AddIndexedGraphicsPass("Water Pass", passSpec);
    }

    {
        IndexedGraphicsPassSpec passSpec = {
            .Pipeline = m_DuckPipeline,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
                { "Transform Buffer", 1, true, false },
            },
            .ImageBindings = {
                { "Duck Color Texture", 2, m_TextureSampler, true, false },
            },
            .VertexBuffers = {
                .VertexBuffers = { { "Vertex Buffer" } },
            },
            .IndexBuffer = { "Index Buffer", 0, vk::IndexType::eUint32 },
            .ColorAttachments = {
                {
                    .ImageResource = "Image",
                },
            },
            .DepthAttachment = { {
                .ImageResource = "Depth Stencil Image",
            } },
            .Draws = { getInstanceDraw(m_Scene.GetDuckInstanceIndex()) },
        };
        builder.AddIndexedGraphicsPass("Duck Pass", passSpec);
    }

    {
        IndexedGraphicsPassSpec passSpec = {
            .Pipeline = m_EnvironmentPipeline,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
                { "Transform Buffer", 1, true, false },
            },
            .ImageBindings = {
                { "Environment Cube Texture", 2, m_TextureSampler, true, false },
            },
            .VertexBuffers = {
                .VertexBuffers = { { "Vertex Buffer" } },
            },
            .IndexBuffer = { "Index Buffer", 0, vk::IndexType::eUint32 },
            .ColorAttachments = {
                {
                    .ImageResource = "Image",
                },
            },
            .DepthAttachment = { {
                .ImageResource = "Depth Stencil Image",
            } },
            .Draws = { getInstanceDraw(m_Scene.GetEnvironmentInstanceIndex()) },
        };
        builder.AddIndexedGraphicsPass("Environment Pass", passSpec);
    }

    {
        CustomGraphicsPassSpec passSpec = {
            .OnRender = [this](vk::CommandBuffer cmd) { m_UserInterface->OnRenderVulkan(cmd); },
            .ColorAttachments = {
                {
                    .ImageResource = "Image",
                },
            },
        };

        builder.AddCustomGraphicsPass("UI Pass", passSpec);
    }

    {
        vk::ImageSubresourceLayers layers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);

        BlitPassSpec blitSpec = {
            .SrcImageResource = "Image",
            .DstImageResource = FrameGraph::g_SwapchainImageResourceName,
            .Regions = { vk::ImageBlit2(layers, {}, layers, {}) },
        };
        builder.AddBlitPass("Blit Pass", blitSpec);
    }

    m_FrameGraph = builder.CreateUnique(spec.PipelineLibrary, m_ResourceAllocator.get());

    RendererSpec rendererSpec = {
        .LogicalDevice = spec.LogicalDevice,
        .MainQueue = m_MainQueue,
        .FrameGraph = m_FrameGraph.get(),
        .ResourceAllocator = m_ResourceAllocator.get(),
        .StagingBufferSize = 16 * 1024 * 1024,
    };

    m_Renderer = std::make_unique<Renderer>(rendererSpec);

    m_UserInterface->OnEnter();

    auto uploadBuffer = [&](const std::string& name, std::span<const std::byte> data) {
        assert(m_FrameGraph->GetBuffer(name).size() == 1);
        auto bufferId = m_FrameGraph->GetBuffer(name).front();
        if (m_ResourceAllocator->GetBufferResource(bufferId).IsDevice)
            m_Renderer->UploadWithStaging(bufferId, data);
        else
            m_ResourceAllocator->UploadToBuffer(bufferId, data.data(), data.size());
    };

    uploadBuffer("Vertex Buffer", std::as_bytes(m_Scene.GetVertices()));
    uploadBuffer("Index Buffer", std::as_bytes(m_Scene.GetIndices()));

    auto clearBuffer = [&](const std::string& name) {
        assert(m_FrameGraph->GetBuffer(name).size() == 1);
        auto bufferId = m_FrameGraph->GetBuffer(name).front();
        m_Renderer->FillBuffer(bufferId, 0, vk::WholeSize, 0);
    };

    clearBuffer("Water Height Buffer 0");
    clearBuffer("Water Height Buffer 1");

    m_Renderer->UploadWithStaging(
        m_FrameGraph->GetImage("Duck Color Texture").front().first, duckTexture.Content, vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1)
    );
    for (int i = 0; i < 6; i++)
        m_Renderer->UploadWithStaging(
            m_FrameGraph->GetImage("Environment Cube Texture").front().first, environmentTextures[i].Content, vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, i, 1)
        );
}

void DuckApplicationState::OnExit(vulkan::ApplicationState* /* next */)
{
    const auto& spec = Application::GetInstance()->GetApplicationStateSpec();
    spec.LogicalDevice.destroySampler(m_TextureSampler);

    m_UserInterface->OnExit();
    m_Renderer.reset();
    m_FrameGraph.reset();
    m_UserInterface.reset();
    m_ResourceAllocator.reset();
}

void DuckApplicationState::OnResize(const Swapchain* swapchain)
{
    m_Renderer->OnResize(swapchain);

    const vk::Extent2D extent = swapchain->GetExtent();
    m_Scene.OnResize(extent.width, extent.height);

    auto resizeImage = [&](const std::string& name) {
        m_FrameGraph->ModifyImage(name).Info.setExtent(vk::Extent3D(extent, 1));
        m_FrameGraph->UpdateImage(name);
    };

    resizeImage("Image");
    resizeImage("Depth Stencil Image");

    auto resizeGraphicsPass = [&](auto config) {
        config.GetScissors() = { vk::Rect2D(vk::Offset2D(0, 0), extent) };
        config.GetViewports() = { vk::Viewport(0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0, 1) };
        config.GetRenderArea().extent = extent;
    };

    resizeGraphicsPass(m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Water Pass"));
    resizeGraphicsPass(m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Duck Pass"));
    resizeGraphicsPass(m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Environment Pass"));

    std::array<vk::Offset3D, 2> offsets = { vk::Offset3D(), vk::Offset3D(extent.width, extent.height, 1) };

    m_FrameGraph->GetBlitPassDynamicConfig("Blit Pass").GetSrcOffsets().front() = offsets;
    m_FrameGraph->GetBlitPassDynamicConfig("Blit Pass").GetDstOffsets().front() = offsets;

    m_FrameGraph->GetCustomGraphicsPassDynamicConfig("UI Pass").GetRenderArea().extent = swapchain->GetExtent();
}

void DuckApplicationState::OnUpdate(float timeStep)
{
    m_UserInterface->OnUpdate(timeStep);

    m_Scene.OnUpdate(timeStep);

    m_SimulationData.InputBufferIndex = m_SimulationData.InputBufferIndex == 0 ? 1 : 0;
    m_SimulationData.HasDisturb = m_Scene.GetDisturbance().has_value();
    if (m_Scene.GetDisturbance().has_value())
        m_SimulationData.Disturb = m_Scene.GetDisturbance().value();
    m_SimulationData.DuckDisturb = m_Scene.GetDuckDisturbance();
}

void DuckApplicationState::OnRender()
{
    m_Renderer->BeginFrame();

    {
        CameraConstants camera = {
            .Projection = m_Scene.GetCameraProjection(),
            .View = m_Scene.GetCameraView(),
            .Origin = m_Scene.GetCameraOrigin(),
        };
        auto bufferId = m_FrameGraph->GetCurrentBuffer("Camera Uniform Buffer");
        m_ResourceAllocator->UploadToBuffer(bufferId, &camera, sizeof(CameraConstants));
    }

    {
        const auto& transforms = m_Scene.GetTransforms();
        auto bufferId = m_FrameGraph->GetCurrentBuffer("Transform Buffer");
        m_ResourceAllocator->UploadToBuffer(bufferId, transforms.data(), transforms.size_bytes(), 0);
    }

    m_Renderer->EndFrame();
}
