#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Core/Core.h>

#include <Vulkan/Application.h>
#include <Vulkan/Renderer/FrameGraphBuilder.h>

#include "ApplicationState.h"

using namespace ref;
using namespace ref::vulkan;

SwapchainUserInterfaceState::SwapchainUserInterfaceState()
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

void SwapchainUserInterfaceState::OnKeyRelease(Key /* key */)
{
}

struct CameraConstants
{
    glm::mat4x4 Projection;
    glm::mat4x4 View;
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
        auto ptr = std::make_unique<SwapchainUserInterfaceState>();
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
    builder.AddHostBuffer("Robot Vertex Position Index Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eVertexBuffer).setSize(m_Scene.GetRobotVertexIndices().size_bytes()), false);
    builder.AddHostBuffer("Robot Vertex Normal Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eVertexBuffer).setSize(m_Scene.GetRobotVertexNormals().size_bytes()), false);
    builder.AddHostBuffer("Robot Index Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eIndexBuffer).setSize(m_Scene.GetRobotTriangles().size_bytes()), false);
    builder.AddHostBuffer("Robot Transform Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eUniformBuffer).setSize(6 * sizeof(glm::mat4x4)), true);
    builder.AddHostBuffer("Camera Uniform Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eUniformBuffer).setSize(sizeof(CameraConstants)), true);

    ShaderId vertexShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/robot.vert", "main", vk::ShaderStageFlagBits::eVertex));
    ShaderId fragmentShader = spec.ShaderLibrary->AddShader(ShaderInfo("Shaders/color.frag", "main", vk::ShaderStageFlagBits::eFragment));

    spec.ShaderLibrary->LoadShader(vertexShader);
    spec.ShaderLibrary->LoadShader(fragmentShader);

    GraphicsPipelineId graphicsPipelineId;
    {
        GraphicsPipelineInfo pipelineInfo = {
            .Name = "Graphics Pipeline",
            .VertexShaderId = vertexShader,
            .FragmentShaderId = fragmentShader,
            .BindingDescriptions = { vk::VertexInputBindingDescription(0, sizeof(uint32_t)), vk::VertexInputBindingDescription(1, sizeof(glm::vec4)) },
            .VertexInputs = { { 0, 0 }, { 1, 0 } },
            .ColorAttachmentFormats = { vk::Format::eR8G8B8A8Unorm },
            .DepthAttachmentFormat = vk::Format::eD24UnormS8Uint,
            .StencilAttachmentFormat = vk::Format::eD24UnormS8Uint,
        };

        pipelineInfo.InputAssemblyState.setTopology(vk::PrimitiveTopology::eTriangleList);
        pipelineInfo.RasterizationState.setLineWidth(1.0f);
        pipelineInfo.AttachmentBlendStates.emplace_back().setColorWriteMask(vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags);

        pipelineInfo.DepthStencilState.setDepthTestEnable(vk::True);
        pipelineInfo.DepthStencilState.setDepthWriteEnable(vk::True);
        pipelineInfo.DepthStencilState.setDepthCompareOp(vk::CompareOp::eLess);

        graphicsPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);
    }

    [[maybe_unused]] bool success = spec.PipelineLibrary->CompilePipeline(graphicsPipelineId);
    assert(success == true);

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
            .Pipeline = graphicsPipelineId,
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
                    .LoadOp = vk::AttachmentLoadOp::eClear,
                    .ClearValue = vk::ClearColorValue(0.2f, 0.2f, 0.2f, 1.0f),
                },
            },
            .DepthAttachment = { {
                .ImageResource = "Depth Stencil Image",
                .LoadOp = vk::AttachmentLoadOp::eClear,
                .ClearValue = vk::ClearDepthStencilValue(1.0f),
            } },
            .Draws = std::move(draws),
        };
        builder.AddIndexedGraphicsPass("Main Pass", passSpec);
    }

    {
        CustomGraphicsPassSpec passSpec = {
            .OnRender = [this](vk::CommandBuffer cmd) { m_UserInterface->OnRenderVulkan(cmd); },
            .ColorAttachments = {
                {
                    .ImageResource = "Image",
                    .LoadOp = vk::AttachmentLoadOp::eLoad,
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
    m_Width = extent.width; m_Height = extent.height;

    m_FrameGraph->ModifyImage("Image").Info.setExtent(vk::Extent3D(extent, 1));
    m_FrameGraph->UpdateImage("Image");

    m_FrameGraph->ModifyImage("Depth Stencil Image").Info.setExtent(vk::Extent3D(extent, 1));
    m_FrameGraph->UpdateImage("Depth Stencil Image");

    m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Main Pass").GetScissors() = {
    vk::Rect2D(vk::Offset2D(0, 0), extent)
    };
    m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Main Pass").GetViewports() = {
        vk::Viewport(0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0, 1)
    };
    m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Main Pass").GetRenderArea().extent = extent;

    std::array<vk::Offset3D, 2> offsets = { vk::Offset3D(), vk::Offset3D(extent.width, extent.height, 1) };

    m_FrameGraph->GetBlitPassDynamicConfig("Blit Pass").GetSrcOffsets().front() = offsets;
    m_FrameGraph->GetBlitPassDynamicConfig("Blit Pass").GetDstOffsets().front() = offsets;

    m_FrameGraph->GetCustomGraphicsPassDynamicConfig("UI Pass").GetRenderArea().extent = swapchain->GetExtent();
}

void RobotApplicationState::OnUpdate(float timeStep)
{
    m_UserInterface->OnUpdate(timeStep);
    m_Renderer->OnUpdate(timeStep);

    {
        // TODO: inverse kinematics
        std::array<glm::mat4x4, 6> RobotMeshTransforms;
        for (int i = 0; i < 6; i++)
            RobotMeshTransforms[i] = glm::identity<glm::mat4x4>();

        auto bufferId = m_FrameGraph->GetCurrentBuffer("Robot Transform Buffer");
        m_ResourceAllocator->UploadToBuffer(bufferId, RobotMeshTransforms.data(), std::span(RobotMeshTransforms).size_bytes());
    }

    {
        // TODO: camera controls
        CameraConstants camera = {
            .Projection = glm::perspectiveFov(45.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.1f, 1000.0f),
            .View = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        };

        auto bufferId = m_FrameGraph->GetCurrentBuffer("Camera Uniform Buffer");
        m_ResourceAllocator->UploadToBuffer(bufferId, &camera, sizeof(CameraConstants));
    }
}

void RobotApplicationState::OnRender()
{
    m_Renderer->OnRender();
}
