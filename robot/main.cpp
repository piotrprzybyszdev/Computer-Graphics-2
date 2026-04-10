#include <imgui.h>

#include <Core/Core.h>
#include <Core/UserInterface.h>

#include <Vulkan/Application.h>
#include <Vulkan/ApplicationBuilder.h>
#include <Vulkan/ApplicationState.h>
#include <Vulkan/Renderer/FrameGraphBuilder.h>

using namespace ref;
using namespace ref::vulkan;

class SwapchainUserInterfaceState final : public UserInterfaceState
{
public:
    SwapchainUserInterfaceState();
    ~SwapchainUserInterfaceState() override = default;

    void OnInit() override;
    void OnShutdown() override;

    void OnUpdate(float timeStep) override;
    void OnKeyRelease(Key key) override;
};

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

class RobotApplicationState final : public ApplicationState
{
public:
    RobotApplicationState(const ApplicationStateSpec& spec);
    ~RobotApplicationState() override;

    void OnEnter(ApplicationState* previous) override;
    void OnExit(ApplicationState* next) override;

    void OnResize(const Swapchain* swapchain) override;

    void OnUpdate(float timeStep) override;
    void OnRender() override;

private:
    Queue m_MainQueue;
    SwapchainUserInterfaceState* m_UserInterfaceState;
    std::unique_ptr<UserInterface> m_UserInterface;
    std::unique_ptr<FrameGraph> m_FrameGraph;
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<ResourceAllocator> m_ResourceAllocator;

    SwapchainBuilder* m_SwapchainBuilder;
};

RobotApplicationState::RobotApplicationState(const ApplicationStateSpec& spec)
    : m_MainQueue(spec.Queues.at(Application::MainQueueName)), m_SwapchainBuilder(spec.SwapchainBuilder)
{
    ResourceManagerSpec resourceManagerSpec = {
        .ApiVersion = spec.ApiVersion,
        .Instance = spec.Instance,
        .PhysicalDevice = spec.PhysicalDevice,
        .LogicalDevice = spec.LogicalDevice,
    };

    m_ResourceAllocator = std::make_unique<ResourceAllocator>(resourceManagerSpec);

    const uint32_t imageCount = std::clamp(2u, m_SwapchainBuilder->GetMinImageCount(), m_SwapchainBuilder->GetMaxImageCount());
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
        "Image",
        vk::ImageCreateInfo(
            vk::ImageCreateFlagBits::eMutableFormat, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            vk::Extent3D(1280, 720, 1), 1, 1
        )
        .setUsage(
            vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eColorAttachment
        ),
        true
    );

    {
        CustomGraphicsPassSpec passSpec = {
            .OnRender = [this](vk::CommandBuffer cmd) { m_UserInterface->OnRenderVulkan(cmd); },
            .ColorAttachments = {
                {
                    .ImageResource = "Image",
                    .LoadOp = vk::AttachmentLoadOp::eClear,
                    .ClearValue = vk::ClearColorValue(0.0f, 0.0f, 1.0f, 1.0f),
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

    m_FrameGraph->ModifyImage("Image").Info.setExtent(vk::Extent3D(extent, 1));
    m_FrameGraph->UpdateImage("Image");

    std::array<vk::Offset3D, 2> offsets = { vk::Offset3D(), vk::Offset3D(extent.width, extent.height, 1) };

    m_FrameGraph->GetBlitPassDynamicConfig("Blit Pass").GetSrcOffsets().front() = offsets;
    m_FrameGraph->GetBlitPassDynamicConfig("Blit Pass").GetDstOffsets().front() = offsets;

    m_FrameGraph->GetCustomGraphicsPassDynamicConfig("UI Pass").GetRenderArea().extent = swapchain->GetExtent();
}

void RobotApplicationState::OnUpdate(float timeStep)
{
    m_UserInterface->OnUpdate(timeStep);
    m_Renderer->OnUpdate(timeStep);
}

void RobotApplicationState::OnRender()
{
    m_Renderer->OnRender();
}

int main()
{
    logger::set_level(logger::level::debug);

    ApplicationBuilder::InitSystems();

    ApplicationBuilder builder;
    builder.EnableBase();

    {
        vulkan::Application application = builder.CreateApplication("REF");
        vulkan::ErrorApplicationState::AddToApplication(application);
        application.AddAndCreateState<RobotApplicationState>("Robot State");
        application.Run("Robot State");
    }

    ApplicationBuilder::ShutdownSystems();
}