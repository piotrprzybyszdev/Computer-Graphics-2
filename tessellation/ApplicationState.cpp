#include <imgui.h>
#include <glm/glm.hpp>

#include <Core/Core.h>

#include <Vulkan/Application.h>
#include <Vulkan/Renderer/FrameGraphBuilder.h>

#include "ApplicationState.h"

using namespace ref;
using namespace ref::vulkan;

TessellationUserInterface::TessellationUserInterface(const UserInterfaceVulkanSpec& spec, Scene& scene) : UserInterface(spec), m_Scene(scene)
{
}

void TessellationUserInterface::OnDefineUI(float /* timeStep */)
{
}

void TessellationUserInterface::OnKeyEvent(Key key, KeyAction action, Mods mods)
{
    m_Scene.OnKeyEvent(key, action, mods);

    if (key == Key::H)
    {
        Application::GetInstance()->GetApplicationStateSpec().Queues.at(Application::MainQueueName).Handle.waitIdle();
        ErrorApplicationState::ReloadShaders("Duck State");
    }
}

void TessellationUserInterface::OnMouseButtonEvent(ref::Button button, ref::ButtonAction action, ref::Mods mods)
{
    m_Scene.OnMouseButtonEvent(button, action, mods);
}

void TessellationUserInterface::OnCursorMoveEvent(double xpos, double ypos)
{
    m_Scene.OnCursorMoveEvent(xpos, ypos);
}

struct CameraConstants
{
    glm::mat4x4 Projection;
    glm::mat4x4 View;
    glm::vec4 Origin;
    glm::vec4 Color0;
    glm::vec4 Color1;
    uint32_t InsideTessFactor;
    uint32_t OutsideTessFactor;
    glm::uvec2 pad0;
};

TessellationApplicationState::TessellationApplicationState(const ApplicationStateSpec& spec)
    : m_MainQueue(spec.Queues.at(Application::MainQueueName))
{
    ShaderId meshVertexShader = spec.ShaderLibrary->GetShaderByPath("Shaders/mesh.vert");
    ShaderId quadTessellationControlShader = spec.ShaderLibrary->GetShaderByPath("Shaders/quad.tesc");
    ShaderId quadTessellationEvaluationShader = spec.ShaderLibrary->GetShaderByPath("Shaders/quad.tese");
    ShaderId colorFragmentShader = spec.ShaderLibrary->GetShaderByPath("Shaders/color.frag");

    {
        GraphicsPipelineInfo pipelineInfo = {
            .Name = "Line Pipeline",
            .VertexShaderId = meshVertexShader,
            .FragmentShaderId = colorFragmentShader,
        };

        auto linePipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        pipelineInfo = {
            .Name = "Patch Pipeline",
            .VertexShaderId = meshVertexShader,
            .TessellationControlShaderId = quadTessellationControlShader,
            .TessellationEvaluationShaderId = quadTessellationEvaluationShader,
            .FragmentShaderId = colorFragmentShader,
        };

        auto patchPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        GraphicsPipelineInstanceInfo pipelineInstanceInfo = {
            .Name = "Line Pipeline Instance",
            .PipelineId = linePipelineId,
            .BindingDescriptions = { vk::VertexInputBindingDescription(0, sizeof(Vertex)) },
            .VertexInputs = { { 0, offsetof(Vertex, Position) } },
            .ColorAttachmentFormats = { vk::Format::eR8G8B8A8Unorm },
            .DepthAttachmentFormat = vk::Format::eD24UnormS8Uint,
            .StencilAttachmentFormat = vk::Format::eD24UnormS8Uint,
        };

        pipelineInstanceInfo.InputAssemblyState.setTopology(vk::PrimitiveTopology::eLineList);
        pipelineInstanceInfo.RasterizationState.setLineWidth(1.0f);
        pipelineInstanceInfo.DepthStencilState.setDepthTestEnable(vk::True);
        pipelineInstanceInfo.DepthStencilState.setDepthWriteEnable(vk::True);
        pipelineInstanceInfo.DepthStencilState.setDepthCompareOp(vk::CompareOp::eLess);
        pipelineInstanceInfo.AttachmentBlendStates.emplace_back().setColorWriteMask(vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags);
        m_LinePipeline = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);

        pipelineInstanceInfo.Name = "Patch Pipeline Instance";
        pipelineInstanceInfo.PipelineId = patchPipelineId;
        pipelineInstanceInfo.InputAssemblyState.setTopology(vk::PrimitiveTopology::ePatchList);
        pipelineInstanceInfo.RasterizationState.setPolygonMode(vk::PolygonMode::eLine);
        pipelineInstanceInfo.TessellationState.setPatchControlPoints(16);
        m_PatchPipeline = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);
    }
}

TessellationApplicationState::~TessellationApplicationState()
{
}

void TessellationApplicationState::OnEnter(vulkan::ApplicationState* /* previous */)
{
    const auto& spec = Application::GetInstance()->GetApplicationStateSpec();

    [[maybe_unused]] bool success = spec.PipelineLibrary->CompilePipelines();
    assert(success == true);

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

    m_TextureSampler = spec.LogicalDevice.createSampler(vk::SamplerCreateInfo().setMinFilter(vk::Filter::eLinear).setMagFilter(vk::Filter::eLinear));

    m_UserInterface = std::make_unique<TessellationUserInterface>(userInterfaceSpec, m_Scene);
    m_UserInterface->OnEnter();

    RebuildFrameGraph();
}

void TessellationApplicationState::OnExit(vulkan::ApplicationState* /* next */)
{
    const auto& spec = Application::GetInstance()->GetApplicationStateSpec();
    spec.LogicalDevice.destroySampler(m_TextureSampler);

    m_UserInterface->OnExit();
    m_Renderer.reset();
    m_FrameGraph.reset();
    m_UserInterface.reset();
    m_ResourceAllocator.reset();
}

void TessellationApplicationState::OnResize(const Swapchain* swapchain)
{
    m_Swapchain = swapchain;
    m_Renderer->OnResize(swapchain);

    const vk::Extent2D extent = swapchain->GetExtent();
    m_Scene.OnResize(extent.width, extent.height);

    auto resizeImage = [&](const std::string& name) {
        m_FrameGraph->ModifyImage(name).Info.setExtent(vk::Extent3D(extent, 1));
        m_FrameGraph->UpdateImage(name);
        m_FrameGraph->UpdateImageView(std::format("{} View", name));
    };

    resizeImage("Image");
    resizeImage("Depth Stencil Image");

    auto resizeGraphicsPass = [&](auto config) {
        config.GetScissors() = { vk::Rect2D(vk::Offset2D(0, 0), extent) };
        config.GetViewports() = { vk::Viewport(0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0, 1) };
        config.GetRenderArea().extent = extent;
    };

    resizeGraphicsPass(m_FrameGraph->GetGraphicsPassDynamicConfig("Patch Pass"));
    if (m_ShowControlLines)
        resizeGraphicsPass(m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Line Pass"));

    std::array<vk::Offset3D, 2> offsets = { vk::Offset3D(), vk::Offset3D(extent.width, extent.height, 1) };

    m_FrameGraph->GetBlitPassDynamicConfig("Blit Pass").GetSrcOffsets().front() = offsets;
    m_FrameGraph->GetBlitPassDynamicConfig("Blit Pass").GetDstOffsets().front() = offsets;
}

void TessellationApplicationState::OnUpdate(float timeStep)
{
    m_UserInterface->OnUpdate(timeStep);

    m_Scene.OnUpdate(timeStep);

    if (m_ShowControlLines != m_Scene.GetTessellationControls().ShowControlLines)
    {
        m_MainQueue.Handle.waitIdle();
        RebuildFrameGraph();
        m_ShowControlLines = m_Scene.GetTessellationControls().ShowControlLines;
        OnResize(m_Swapchain);
    }

    {
        const auto &patch = m_Scene.GetPatches()[m_Scene.GetCurrentPatchIndex()];
        m_FrameGraph->GetGraphicsPassDynamicConfig("Patch Pass").GetDrawCommand().front().Command.FirstVertex = patch.VertexOffset;
        if (m_ShowControlLines)
        {
            m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Line Pass").GetIndexedDrawCommand().front().Command.FirstIndex = patch.IndexOffset;
            m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Line Pass").GetIndexedDrawCommand().front().Command.VertexOffset = patch.VertexOffset;
        }
    }
}

void TessellationApplicationState::OnRender()
{
    m_Renderer->BeginFrame();

    {
        CameraConstants camera = {
            .Projection = m_Scene.GetCameraProjection(),
            .View = m_Scene.GetCameraView(),
            .Origin = m_Scene.GetCameraOrigin(),
            .Color0 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
            .Color1 = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
            .InsideTessFactor = m_Scene.GetTessellationControls().InsideTessFactor,
            .OutsideTessFactor = m_Scene.GetTessellationControls().OutsideTessFactor,
        };
        auto bufferId = m_FrameGraph->GetCurrentBuffer("Camera Uniform Buffer");
        m_ResourceAllocator->UploadToBuffer(bufferId, &camera, sizeof(CameraConstants));
    }

    m_Renderer->EndFrame();
}

void TessellationApplicationState::RebuildFrameGraph()
{
    const auto& spec = Application::GetInstance()->GetApplicationStateSpec();

    ResourceManagerSpec resourceManagerSpec = {
        .ApiVersion = spec.ApiVersion,
        .Instance = spec.Instance,
        .PhysicalDevice = spec.PhysicalDevice,
        .LogicalDevice = spec.LogicalDevice,
    };

    m_ResourceAllocator = std::make_unique<ResourceAllocator>(resourceManagerSpec);

    FrameGraphBuilder builder;

    builder.AddDeviceImageWithView(
        "Image", vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(1280, 720, 1), 1, 1)
        .setUsage(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment), ResourceType::Transient, true
    );
    builder.AddDeviceImageWithView(
        "Depth Stencil Image", vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eD24UnormS8Uint, vk::Extent3D(1280, 720, 1), 1, 1)
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment), ResourceType::Transient, true
    );

    builder.AddDeviceBuffer("Vertex Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer).setSize(m_Scene.GetVertices().size_bytes()), ResourceType::Persistent, false);
    builder.AddDeviceBuffer("Index Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer).setSize(m_Scene.GetIndices().size_bytes()), ResourceType::Persistent, false);

    builder.AddHostBuffer("Camera Uniform Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eUniformBuffer).setSize(sizeof(CameraConstants)), ResourceType::Persistent, true);

    {
        const auto& patch = m_Scene.GetPatches()[m_Scene.GetCurrentPatchIndex()];
        GraphicsPassSpec passSpec = {
            .Pipeline = m_PatchPipeline,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
            },
            .VertexBuffers = {
                .VertexBuffers = { { "Vertex Buffer" } },
            },
            .ColorAttachments = {
                {
                    .ImageViewResource = "Image View",
                    .LoadOp = vk::AttachmentLoadOp::eClear,
                    .ClearValue = vk::ClearColorValue(0.2f, 0.2f, 0.2f, 1.0f),
                },
            },
            .DepthAttachment = { {
                .ImageViewResource = "Depth Stencil Image View",
                .LoadOp = vk::AttachmentLoadOp::eClear,
                .ClearValue = vk::ClearDepthStencilValue(1.0f, 0),
            } },
            .Draws = {
                {
                    .Command = {
                        .VertexCount = patch.VertexCount,
                        .InstanceCount = 1,
                        .FirstVertex = patch.VertexOffset,
                        .FirstInstance = 0,
                    },
                    .PushConstantData = std::as_bytes(std::span(&m_PatchColorIndex, 1)),
                }
            },
        };
        builder.AddGraphicsPass("Patch Pass", passSpec);
    }

    if (m_Scene.GetTessellationControls().ShowControlLines)
    {
        const auto& patch = m_Scene.GetPatches()[m_Scene.GetCurrentPatchIndex()];
        IndexedGraphicsPassSpec passSpec = {
            .Pipeline = m_LinePipeline,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
            },
            .VertexBuffers = {
                .VertexBuffers = { { "Vertex Buffer" } },
            },
            .IndexBuffer = { "Index Buffer", 0, vk::IndexType::eUint32 },
            .ColorAttachments = {
                {
                    .ImageViewResource = "Image View",
                },
            },
            .DepthAttachment = { {
                .ImageViewResource = "Depth Stencil Image View",
            } },
            .Draws = {
                {
                    .Command = {
                        .IndexCount = patch.IndexCount,
                        .InstanceCount = 1,
                        .FirstIndex = patch.IndexOffset,
                        .VertexOffset = patch.VertexOffset,
                        .FirstInstance = 0,
                    },
                    .PushConstantData = std::as_bytes(std::span(&m_LineColorIndex, 1)),
                }
            },
        };
        builder.AddIndexedGraphicsPass("Line Pass", passSpec);
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
    };

    m_Renderer = std::make_unique<Renderer>(rendererSpec);

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
}
