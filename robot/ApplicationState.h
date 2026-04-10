#include <glm/glm.hpp>

#include <Core/UserInterface.h>

#include <Vulkan/ApplicationState.h>

#include "Scene.h"

class SwapchainUserInterfaceState final : public ref::UserInterfaceState
{
public:
    SwapchainUserInterfaceState();
    ~SwapchainUserInterfaceState() override = default;

    void OnInit() override;
    void OnShutdown() override;

    void OnUpdate(float timeStep) override;
    void OnKeyRelease(ref::Key key) override;
};

class RobotApplicationState final : public ref::vulkan::ApplicationState
{
public:
    RobotApplicationState(const ref::vulkan::ApplicationStateSpec& spec);
    ~RobotApplicationState() override;

    void OnEnter(ApplicationState* previous) override;
    void OnExit(ApplicationState* next) override;

    void OnResize(const ref::vulkan::Swapchain* swapchain) override;

    void OnUpdate(float timeStep) override;
    void OnRender() override;

private:
    ref::vulkan::Queue m_MainQueue;
    SwapchainUserInterfaceState* m_UserInterfaceState;
    std::unique_ptr<ref::UserInterface> m_UserInterface;
    std::unique_ptr<ref::vulkan::FrameGraph> m_FrameGraph;
    std::unique_ptr<ref::vulkan::Renderer> m_Renderer;
    std::unique_ptr<ref::vulkan::ResourceAllocator> m_ResourceAllocator;

    Scene m_Scene;
    uint32_t m_Width = 0, m_Height = 0;
    std::array<uint32_t, 6> m_MeshIndices = { 0,1,2,3,4,5 };
};

