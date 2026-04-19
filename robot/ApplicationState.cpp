#include <imgui.h>
#include <glm/glm.hpp>

#include <Core/Core.h>

#include <Vulkan/Application.h>
#include <Vulkan/Renderer/FrameGraphBuilder.h>

#include "ApplicationState.h"

#include <numeric>

using namespace ref;
using namespace ref::vulkan;

SwapchainUserInterfaceState::SwapchainUserInterfaceState(Scene &scene) : m_Scene(scene)
{
}

void SwapchainUserInterfaceState::OnInit()
{
}

void SwapchainUserInterfaceState::OnShutdown()
{
}

void SwapchainUserInterfaceState::OnUpdate(float /* timeStep */)
{
    ImGui::Begin("Bonjur");
    ImGui::End();
}

void SwapchainUserInterfaceState::OnKeyRelease(Key key)
{
    m_Scene.OnKeyRelease(key);
}

struct CameraConstants
{
    glm::mat4x4 Projection;
    glm::mat4x4 View;
    glm::vec4 Origin;
};

RobotApplicationState::RobotApplicationState(const ApplicationStateSpec& spec)
    : m_MainQueue(spec.Queues.at(Application::MainQueueName))
{
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

    {
        auto ptr = std::make_unique<SwapchainUserInterfaceState>(m_Scene);
        m_UserInterfaceState = ptr.get();
        m_UserInterface = std::make_unique<UserInterface>(userInterfaceSpec, std::move(ptr));
    }

    FrameGraphBuilder builder;

    builder.AddDeviceImage(
        "Image", vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(1280, 720, 1), 1, 1)
        .setUsage(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment), true
    );
    builder.AddDeviceImage(
        "Depth Stencil Image", vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eD24UnormS8Uint, vk::Extent3D(1280, 720, 1), 1, 1)
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment), true
    );
    builder.AddHostBuffer("Mesh Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetMeshes().size_bytes()), false);
    builder.AddHostBuffer("Vertex Position Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetPositions().size_bytes()), false);
    builder.AddHostBuffer("Vertex Position Index Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetVertexIndices().size_bytes()), false);
    builder.AddHostBuffer("Vertex Normal Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eVertexBuffer).setSize(m_Scene.GetVertexNormals().size_bytes()), false);
    builder.AddHostBuffer("Index Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetTriangles().size_bytes()), false);
    builder.AddHostBuffer("Edge Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eVertexBuffer).setSize(m_Scene.GetEdges().size_bytes()), false);
    builder.AddHostBuffer("Transform Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eUniformBuffer).setSize(m_Scene.GetTransforms().size_bytes()), true);
    
    builder.AddHostBuffer("Camera Uniform Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eUniformBuffer).setSize(sizeof(CameraConstants)), true);
    builder.AddHostBuffer("Light Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetLights().size_bytes()), false);

    ShaderId robotVertexShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/mesh.vert", "main", vk::ShaderStageFlagBits::eVertex));
    ShaderId phongFragmentShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/phong.frag", "main", vk::ShaderStageFlagBits::eFragment));
    ShaderId shadowVertexShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/shadow.vert", "main", vk::ShaderStageFlagBits::eVertex));
    ShaderId shadowGeometryShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/shadow.geom", "main", vk::ShaderStageFlagBits::eGeometry));
    ShaderId shadowFragmentShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/shadow.frag", "main", vk::ShaderStageFlagBits::eFragment));
    ShaderId ambientFragmentShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/ambient.frag", "main", vk::ShaderStageFlagBits::eFragment));

    spec.ShaderLibrary->LoadShader(robotVertexShader);
    spec.ShaderLibrary->LoadShader(ambientFragmentShader);
    spec.ShaderLibrary->LoadShader(phongFragmentShader);
    spec.ShaderLibrary->LoadShader(shadowVertexShader);
    spec.ShaderLibrary->LoadShader(shadowGeometryShader);
    spec.ShaderLibrary->LoadShader(shadowFragmentShader);

    GraphicsPipelineId lightingPipelineId, shadowPipelineId, ambientPipelineId;
    {
        GraphicsPipelineInfo pipelineInfo = {
            .Name = "Ambient Pipeline",
            .VertexShaderId = robotVertexShader,
            .FragmentShaderId = ambientFragmentShader,
            .BindingDescriptions = { vk::VertexInputBindingDescription(0, sizeof(uint32_t)), vk::VertexInputBindingDescription(1, sizeof(glm::vec4)) },
            .VertexInputs = { { 0, 0 }, { 1, 0 } },            
            .ColorAttachmentFormats = { vk::Format::eR8G8B8A8Unorm },
            .DepthAttachmentFormat = vk::Format::eD24UnormS8Uint,
            .StencilAttachmentFormat = vk::Format::eD24UnormS8Uint,
        };

        pipelineInfo.InputAssemblyState.setTopology(vk::PrimitiveTopology::eTriangleList);
        pipelineInfo.RasterizationState.setLineWidth(1.0f);
        pipelineInfo.DepthStencilState.setDepthTestEnable(vk::True);
        pipelineInfo.DepthStencilState.setDepthWriteEnable(vk::True);
        pipelineInfo.DepthStencilState.setDepthCompareOp(vk::CompareOp::eLess);
        pipelineInfo.AttachmentBlendStates.emplace_back().setColorWriteMask(vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags);
        ambientPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        pipelineInfo.Name = "Lighting Pipeline";
        pipelineInfo.FragmentShaderId = phongFragmentShader;
        pipelineInfo.DepthStencilState.setStencilTestEnable(vk::True);
        pipelineInfo.DepthStencilState.setDepthCompareOp(vk::CompareOp::eEqual);
        pipelineInfo.DepthStencilState.setBack(vk::StencilOpState().setCompareMask(0xff).setCompareOp(vk::CompareOp::eEqual).setReference(0));
        pipelineInfo.DepthStencilState.setFront(vk::StencilOpState().setCompareMask(0xff).setCompareOp(vk::CompareOp::eEqual).setReference(0));
        lightingPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        pipelineInfo.Name = "Shadow Pipeline";
        pipelineInfo.InputAssemblyState.setTopology(vk::PrimitiveTopology::ePointList);
        pipelineInfo.DepthStencilState.setDepthCompareOp(vk::CompareOp::eLess);
        pipelineInfo.DepthStencilState.setDepthWriteEnable(vk::False);
        pipelineInfo.AttachmentBlendStates.front().setColorWriteMask(vk::ColorComponentFlags());
        pipelineInfo.DepthStencilState.setBack(vk::StencilOpState().setWriteMask(0xff).setCompareOp(vk::CompareOp::eAlways).setDepthFailOp(vk::StencilOp::eDecrementAndWrap));
        pipelineInfo.DepthStencilState.setFront(vk::StencilOpState().setWriteMask(0xff).setCompareOp(vk::CompareOp::eAlways).setDepthFailOp(vk::StencilOp::eIncrementAndWrap));
        
        pipelineInfo.VertexShaderId = shadowVertexShader;
        pipelineInfo.GeometryShaderId = shadowGeometryShader;
        pipelineInfo.FragmentShaderId = shadowFragmentShader;
        pipelineInfo.BindingDescriptions = { vk::VertexInputBindingDescription(0, sizeof(glm::uvec4)) };
        pipelineInfo.VertexInputs = { { 0, 0 } };
        
        shadowPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);
    }

    [[maybe_unused]] bool success = spec.PipelineLibrary->CompilePipeline(lightingPipelineId);
    assert(success == true);
    success = spec.PipelineLibrary->CompilePipeline(shadowPipelineId);
    assert(success == true);
    success = spec.PipelineLibrary->CompilePipeline(ambientPipelineId);
    assert(success == true);

    m_MeshIndices.resize(m_Scene.GetMeshes().size());
    std::ranges::iota(m_MeshIndices, 0);

    std::vector<IndexedGraphicsPassSpec::DrawSpec> meshDraws = std::ranges::iota_view(0ull, m_Scene.GetMeshes().size()) | std::views::transform([this](size_t index) {
        const auto& mesh = m_Scene.GetMeshes()[index];
        return  IndexedGraphicsPassSpec::DrawSpec{
            .Command = {
                .IndexCount = mesh.TriangleCount * 3,
                .InstanceCount = 1,
                .FirstIndex = mesh.TriangleOffset * 3,
                .VertexOffset = mesh.VertexOffset,
                .FirstInstance = 0,
            },
            .PushConstantData = std::as_bytes(std::span(m_MeshIndices).subspan(index, 1)),
        };
    }) | std::ranges::to<std::vector>();

    {
        IndexedGraphicsPassSpec passSpec = {
            .Pipeline = ambientPipelineId,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
                { "Vertex Position Buffer", 1, true, false },
                { "Mesh Buffer", 2, true, false },
                { "Transform Buffer", 3, true, false },
            },
            .VertexBuffers = {
                .VertexBuffers = { { "Vertex Position Index Buffer" }, { "Vertex Normal Buffer" } },
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
            .StencilAttachment = { {
                .ImageResource = "Depth Stencil Image",
                .LoadOp = vk::AttachmentLoadOp::eClear,
                .ClearValue = vk::ClearDepthStencilValue(1.0f, 0),
            } },
            .Draws = meshDraws,
        };
        builder.AddIndexedGraphicsPass("Ambient Pass", passSpec);
    }

    {
        std::vector<GraphicsPassSpec::DrawSpec> draws = std::ranges::iota_view(0ull, m_Scene.GetRobotMeshes().size()) | std::views::transform([this](size_t index) {
            const auto& mesh = m_Scene.GetMeshes()[index];
            return  GraphicsPassSpec::DrawSpec{
                .Command = {
                    .VertexCount = mesh.EdgeCount,
                    .InstanceCount = 1,
                    .FirstVertex = mesh.EdgeOffset,
                    .FirstInstance = 0,
                },
                .PushConstantData = std::as_bytes(std::span(m_MeshIndices).subspan(index, 1)),
            };
        }) | std::ranges::to<std::vector>();

        GraphicsPassSpec passSpec = {
            .Pipeline = shadowPipelineId,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
                { "Vertex Position Buffer", 1, true, false },
                { "Mesh Buffer", 2, true, false },
                { "Transform Buffer", 3, true, false },
                { "Light Buffer", 4, true, false },
                { "Index Buffer", 5, true, false },
                { "Vertex Position Index Buffer", 6, true, false },
            },
            .VertexBuffers = {
                .VertexBuffers = { { "Edge Buffer" } },
            },
            .ColorAttachments = {
                {
                    .ImageResource = "Image",
                },
            },
            .DepthAttachment = { {
                .ImageResource = "Depth Stencil Image",
            } },
            .StencilAttachment = { {
                .ImageResource = "Depth Stencil Image",
            } },
            .Draws = std::move(draws),
        };
        builder.AddGraphicsPass("Shadow Pass", passSpec);
    }
    
    {
        IndexedGraphicsPassSpec passSpec = {
            .Pipeline = lightingPipelineId,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
                { "Vertex Position Buffer", 1, true, false },
                { "Mesh Buffer", 2, true, false },
                { "Transform Buffer", 3, true, false },
                { "Light Buffer", 4, true, false },
            },
            .VertexBuffers = {
                .VertexBuffers = { { "Vertex Position Index Buffer" }, { "Vertex Normal Buffer" } },
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
            .StencilAttachment = { {
                .ImageResource = "Depth Stencil Image",
            } },
            .Draws = meshDraws,
        };
        builder.AddIndexedGraphicsPass("Lighting Pass", passSpec);
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
    };

    m_Renderer = std::make_unique<Renderer>(rendererSpec);

    auto uploadBuffer = [&](const std::string &name, std::span<const std::byte> data) {
        auto bufferId = m_FrameGraph->GetCurrentBuffer(name);
        m_ResourceAllocator->UploadToBuffer(bufferId, data.data(), data.size());
    };

    uploadBuffer("Mesh Buffer", std::as_bytes(m_Scene.GetMeshes()));
    uploadBuffer("Vertex Position Buffer", std::as_bytes(m_Scene.GetPositions()));
    uploadBuffer("Vertex Position Index Buffer", std::as_bytes(m_Scene.GetVertexIndices()));
    uploadBuffer("Vertex Normal Buffer", std::as_bytes(m_Scene.GetVertexNormals()));
    uploadBuffer("Index Buffer", std::as_bytes(m_Scene.GetTriangles()));
    uploadBuffer("Edge Buffer", std::as_bytes(m_Scene.GetEdges()));
    uploadBuffer("Light Buffer", std::as_bytes(m_Scene.GetLights()));
}

RobotApplicationState::~RobotApplicationState()
{
}

void RobotApplicationState::OnEnter(ApplicationState* /* previous */)
{
}

void RobotApplicationState::OnExit(ApplicationState* /* next */)
{
}

void RobotApplicationState::OnResize(const Swapchain* swapchain)
{
    m_Renderer->OnResize(swapchain);

    const vk::Extent2D extent = swapchain->GetExtent();
    m_Scene.OnResize(extent.width, extent.height);

    m_FrameGraph->ModifyImage("Image").Info.setExtent(vk::Extent3D(extent, 1));
    m_FrameGraph->UpdateImage("Image");

    m_FrameGraph->ModifyImage("Depth Stencil Image").Info.setExtent(vk::Extent3D(extent, 1));
    m_FrameGraph->UpdateImage("Depth Stencil Image");

    auto resizeGraphicsPass = [&](auto config) {
        config.GetScissors() = { vk::Rect2D(vk::Offset2D(0, 0), extent) };
        config.GetViewports() = { vk::Viewport(0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0, 1) };
        config.GetRenderArea().extent = extent;
    };

    resizeGraphicsPass(m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Lighting Pass"));
    resizeGraphicsPass(m_FrameGraph->GetGraphicsPassDynamicConfig("Shadow Pass"));
    resizeGraphicsPass(m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Ambient Pass"));

    std::array<vk::Offset3D, 2> offsets = { vk::Offset3D(), vk::Offset3D(extent.width, extent.height, 1) };

    m_FrameGraph->GetBlitPassDynamicConfig("Blit Pass").GetSrcOffsets().front() = offsets;
    m_FrameGraph->GetBlitPassDynamicConfig("Blit Pass").GetDstOffsets().front() = offsets;

    m_FrameGraph->GetCustomGraphicsPassDynamicConfig("UI Pass").GetRenderArea().extent = swapchain->GetExtent();
}

void RobotApplicationState::OnUpdate(float timeStep)
{
    m_UserInterface->OnUpdate(timeStep);
    m_Renderer->OnUpdate(timeStep);

    m_Scene.OnUpdate(timeStep);

    {
        auto transforms = m_Scene.GetTransforms();
        auto bufferId = m_FrameGraph->GetCurrentBuffer("Transform Buffer");
        m_ResourceAllocator->UploadToBuffer(bufferId, transforms.data(), transforms.size_bytes());
    }

    {
        CameraConstants camera = {
            .Projection = m_Scene.GetCameraProjection(),
            .View = m_Scene.GetCameraView(),
            .Origin = m_Scene.GetCameraOrigin(),
        };
        auto bufferId = m_FrameGraph->GetCurrentBuffer("Camera Uniform Buffer");
        m_ResourceAllocator->UploadToBuffer(bufferId, &camera, sizeof(CameraConstants));
    }
}

void RobotApplicationState::OnRender()
{
    m_Renderer->OnRender();
}
