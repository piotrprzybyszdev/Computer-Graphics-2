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
    builder.AddHostBuffer("Robot Mesh Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetRobotMeshes().size_bytes()), false);
    builder.AddHostBuffer("Robot Vertex Position Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetRobotPositions().size_bytes()), false);
    builder.AddHostBuffer("Robot Vertex Position Index Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetRobotVertexIndices().size_bytes()), false);
    builder.AddHostBuffer("Robot Vertex Normal Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eVertexBuffer).setSize(m_Scene.GetRobotVertexNormals().size_bytes()), false);
    builder.AddHostBuffer("Robot Index Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetRobotTriangles().size_bytes()), false);
    builder.AddHostBuffer("Robot Edge Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eVertexBuffer).setSize(m_Scene.GetRobotEdges().size_bytes()), false);
    builder.AddHostBuffer("Robot Transform Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eUniformBuffer).setSize(6 * sizeof(glm::mat4x4)), true);
    builder.AddHostBuffer("Camera Uniform Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eUniformBuffer).setSize(sizeof(CameraConstants)), true);

    builder.AddHostBuffer("Static Vertex Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eVertexBuffer).setSize(m_Scene.GetStaticVertices().size_bytes()), false);
    builder.AddHostBuffer("Static Index Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eIndexBuffer).setSize(m_Scene.GetStaticIndices().size_bytes()), false);
    builder.AddHostBuffer("Static Transform Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetStaticTransforms().size_bytes()), false);

    builder.AddHostBuffer("Light Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetLights().size_bytes()), false);

    ShaderId robotVertexShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/robot.vert", "main", vk::ShaderStageFlagBits::eVertex));
    ShaderId staticVertexShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/static.vert", "main", vk::ShaderStageFlagBits::eVertex));
    ShaderId phongFragmentShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/phong.frag", "main", vk::ShaderStageFlagBits::eFragment));
    ShaderId shadowVertexShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/shadow.vert", "main", vk::ShaderStageFlagBits::eVertex));
    ShaderId shadowGeometryShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/shadow.geom", "main", vk::ShaderStageFlagBits::eGeometry));
    ShaderId shadowFragmentShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/shadow.frag", "main", vk::ShaderStageFlagBits::eFragment));
    ShaderId ambientFragmentShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/ambient.frag", "main", vk::ShaderStageFlagBits::eFragment));

    spec.ShaderLibrary->LoadShader(robotVertexShader);
    spec.ShaderLibrary->LoadShader(staticVertexShader);
    spec.ShaderLibrary->LoadShader(phongFragmentShader);
    spec.ShaderLibrary->LoadShader(shadowVertexShader);
    spec.ShaderLibrary->LoadShader(shadowGeometryShader);
    spec.ShaderLibrary->LoadShader(shadowFragmentShader);
    spec.ShaderLibrary->LoadShader(ambientFragmentShader);

    GraphicsPipelineId robotPipelineId, shadowPipelineId, staticMeshPipelineId, ambientRobotPipelineId;
    {
        GraphicsPipelineInfo pipelineInfo = {
            .Name = "Static Pipeline",
            .VertexShaderId = staticVertexShader,
            .FragmentShaderId = phongFragmentShader,
            .BindingDescriptions = { vk::VertexInputBindingDescription(0, sizeof(StaticVertex)) },
            .VertexInputs = { { 0, offsetof(StaticVertex, Position) }, { 0, offsetof(StaticVertex, Normal) } },
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
        pipelineInfo.VertexShaderId = staticVertexShader;
        pipelineInfo.FragmentShaderId = phongFragmentShader;
        staticMeshPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        pipelineInfo.Name = "Robot Pipeline";
        pipelineInfo.BindingDescriptions = { vk::VertexInputBindingDescription(0, sizeof(uint32_t)), vk::VertexInputBindingDescription(1, sizeof(glm::vec4)) };
        pipelineInfo.VertexInputs = { { 0, 0 }, { 1, 0 } };
        pipelineInfo.VertexShaderId = robotVertexShader;
        robotPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        pipelineInfo.Name = "Ambient Robot Pipeline";
        pipelineInfo.FragmentShaderId = ambientFragmentShader;
        pipelineInfo.DepthStencilState.setStencilTestEnable(vk::True);
        pipelineInfo.DepthStencilState.setDepthCompareOp(vk::CompareOp::eEqual);
        pipelineInfo.DepthStencilState.setBack(vk::StencilOpState().setCompareMask(0xff).setCompareOp(vk::CompareOp::eNotEqual).setReference(0));
        pipelineInfo.DepthStencilState.setFront(vk::StencilOpState().setCompareMask(0xff).setCompareOp(vk::CompareOp::eNotEqual).setReference(0));
        ambientRobotPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

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

    [[maybe_unused]] bool success = spec.PipelineLibrary->CompilePipeline(robotPipelineId);
    assert(success == true);
    success = spec.PipelineLibrary->CompilePipeline(shadowPipelineId);
    assert(success == true);
    success = spec.PipelineLibrary->CompilePipeline(staticMeshPipelineId);
    assert(success == true);
    success = spec.PipelineLibrary->CompilePipeline(ambientRobotPipelineId);
    assert(success == true);

    m_MeshIndices.resize(std::max(m_Scene.GetRobotMeshes().size(), m_Scene.GetStaticMeshes().size()));
    std::ranges::iota(m_MeshIndices, 0);
    {
        std::vector<IndexedGraphicsPassSpec::DrawSpec> draws;
        for (int i = 0; i < m_Scene.GetStaticMeshes().size(); i++)
        {
            const auto& mesh = m_Scene.GetStaticMeshes()[i];
            draws.push_back({
                .Command = {
                    .IndexCount = mesh.IndexCount,
                    .InstanceCount = 1,
                    .FirstIndex = mesh.IndexOffset,
                    .VertexOffset = mesh.VertexOffset,
                    .FirstInstance = 0,
                },
                .PushConstantData = std::as_bytes(std::span(m_MeshIndices).subspan(i, 1)),
            });
        }
        IndexedGraphicsPassSpec passSpec = {
            .Pipeline = staticMeshPipelineId,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
                { "Static Transform Buffer", 1, true, false },
                { "Light Buffer", 4, true, false },
            },
            .VertexBuffers = {
                .VertexBuffers = { { "Static Vertex Buffer" } },
            },
            .IndexBuffer = { "Static Index Buffer", 0, vk::IndexType::eUint32 },
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
            .Draws = std::move(draws),
        };
        builder.AddIndexedGraphicsPass("Static Mesh Pass", passSpec);
    }

    {
        std::vector<IndexedGraphicsPassSpec::DrawSpec> draws;
        for (int i = 0; i < m_Scene.GetRobotMeshes().size(); i++)
        {
            const auto& mesh = m_Scene.GetRobotMeshes()[i];
            draws.push_back({
                .Command = {
                    .IndexCount = mesh.TriangleCount * 3,
                    .InstanceCount = 1,
                    .FirstIndex = mesh.TriangleOffset * 3,
                    .VertexOffset = mesh.VertexOffset,
                    .FirstInstance = 0,
                },
                .PushConstantData = std::as_bytes(std::span(m_MeshIndices).subspan(i, 1)),
            });
        }

        IndexedGraphicsPassSpec passSpec = {
            .Pipeline = robotPipelineId,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
                { "Robot Vertex Position Buffer", 1, true, false },
                { "Robot Mesh Buffer", 2, true, false },
                { "Robot Transform Buffer", 3, true, false },
                { "Light Buffer", 4, true, false },
            },
            .VertexBuffers = {
                .VertexBuffers = { { "Robot Vertex Position Index Buffer" }, { "Robot Vertex Normal Buffer" } },
            },
            .IndexBuffer = { "Robot Index Buffer", 0, vk::IndexType::eUint32 },
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
        builder.AddIndexedGraphicsPass("Robot Pass", passSpec);
    }

    {
        std::vector<GraphicsPassSpec::DrawSpec> draws;
        for (int i = 0; i < m_Scene.GetRobotMeshes().size(); i++)
        {
            const auto& mesh = m_Scene.GetRobotMeshes()[i];
            draws.push_back({
                .Command = {
                    .VertexCount = mesh.EdgeCount,
                    .InstanceCount = 1,
                    .FirstVertex = mesh.EdgeOffset,
                    .FirstInstance = 0,
                },
                .PushConstantData = std::as_bytes(std::span(m_MeshIndices).subspan(i, 1)),
            });
        }

        GraphicsPassSpec passSpec = {
            .Pipeline = shadowPipelineId,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
                { "Robot Vertex Position Buffer", 1, true, false },
                { "Robot Mesh Buffer", 2, true, false },
                { "Robot Transform Buffer", 3, true, false },
                { "Light Buffer", 4, true, false },
                { "Robot Index Buffer", 5, true, false },
                { "Robot Vertex Position Index Buffer", 6, true, false },
            },
            .VertexBuffers = {
                .VertexBuffers = { { "Robot Edge Buffer" } },
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
        std::vector<IndexedGraphicsPassSpec::DrawSpec> draws;
        for (int i = 0; i < m_Scene.GetRobotMeshes().size(); i++)
        {
            const auto& mesh = m_Scene.GetRobotMeshes()[i];
            draws.push_back({
                .Command = {
                    .IndexCount = mesh.TriangleCount * 3,
                    .InstanceCount = 1,
                    .FirstIndex = mesh.TriangleOffset * 3,
                    .VertexOffset = mesh.VertexOffset,
                    .FirstInstance = 0,
                },
                .PushConstantData = std::as_bytes(std::span(m_MeshIndices).subspan(i, 1)),
            });
        }

        IndexedGraphicsPassSpec passSpec = {
            .Pipeline = ambientRobotPipelineId,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
                { "Robot Vertex Position Buffer", 1, true, false },
                { "Robot Mesh Buffer", 2, true, false },
                { "Robot Transform Buffer", 3, true, false },
            },
            .VertexBuffers = {
                .VertexBuffers = { { "Robot Vertex Position Index Buffer" }, { "Robot Vertex Normal Buffer" } },
            },
            .IndexBuffer = { "Robot Index Buffer", 0, vk::IndexType::eUint32 },
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
        builder.AddIndexedGraphicsPass("Ambient Robot Pass", passSpec);
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

    uploadBuffer("Robot Mesh Buffer", std::as_bytes(m_Scene.GetRobotMeshes()));
    uploadBuffer("Robot Vertex Position Buffer", std::as_bytes(m_Scene.GetRobotPositions()));
    uploadBuffer("Robot Vertex Position Index Buffer", std::as_bytes(m_Scene.GetRobotVertexIndices()));
    uploadBuffer("Robot Vertex Normal Buffer", std::as_bytes(m_Scene.GetRobotVertexNormals()));
    uploadBuffer("Robot Index Buffer", std::as_bytes(m_Scene.GetRobotTriangles()));
    uploadBuffer("Robot Edge Buffer", std::as_bytes(m_Scene.GetRobotEdges()));
    uploadBuffer("Static Vertex Buffer", std::as_bytes(m_Scene.GetStaticVertices()));
    uploadBuffer("Static Index Buffer", std::as_bytes(m_Scene.GetStaticIndices()));
    uploadBuffer("Static Transform Buffer", std::as_bytes(m_Scene.GetStaticTransforms()));
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

    resizeGraphicsPass(m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Robot Pass"));
    resizeGraphicsPass(m_FrameGraph->GetGraphicsPassDynamicConfig("Shadow Pass"));
    resizeGraphicsPass(m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Static Mesh Pass"));
    resizeGraphicsPass(m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Ambient Robot Pass"));

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
        auto transforms = m_Scene.GetRobotTransforms();
        auto bufferId = m_FrameGraph->GetCurrentBuffer("Robot Transform Buffer");
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
