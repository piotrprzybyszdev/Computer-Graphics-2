#include <glm/glm.hpp>

#include <Core/UserInterface.h>

#include <Vulkan/ApplicationState.h>

#include <memory>

#include "Scene.h"

class DuckUserInterface final : public ref::UserInterface
{
public:
    DuckUserInterface(const ref::UserInterfaceVulkanSpec& spec, Scene &scene);
    ~DuckUserInterface() override = default;

    void OnDefineUI(float timeStep) override;
    
    void OnKeyEvent(ref::Key key, ref::KeyAction action, ref::Mods mods) override;
    void OnMouseButtonEvent(ref::Button button, ref::ButtonAction action, ref::Mods mods) override;
    void OnCursorMoveEvent(double xpos, double ypos) override;

private:
    Scene &m_Scene;
};

class DuckApplicationState final : public ref::vulkan::ApplicationState
{
public:
    DuckApplicationState(const ref::vulkan::ApplicationStateSpec& spec);
    ~DuckApplicationState() override;

    void OnEnter(ref::vulkan::ApplicationState* previous) override;
    void OnExit(ref::vulkan::ApplicationState* next) override;

    void OnResize(const ref::vulkan::Swapchain* swapchain) override;

    void OnUpdate(float timeStep) override;
    void OnRender() override;

private:
    ref::vulkan::Queue m_MainQueue;

    ref::vulkan::ComputePipelineInstanceId m_WaterHeightPipeline, m_WaterNormalPipeline;
    ref::vulkan::GraphicsPipelineInstanceId m_WaterPipeline;

    std::unique_ptr<DuckUserInterface> m_UserInterface;
    std::unique_ptr<ref::vulkan::FrameGraph> m_FrameGraph;
    std::unique_ptr<ref::vulkan::Renderer> m_Renderer;
    std::unique_ptr<ref::vulkan::ResourceAllocator> m_ResourceAllocator;

    Scene m_Scene;

    struct SimulationData
    {
        glm::uint InputBufferIndex = 0;
        glm::uint Disturb = 0;
        glm::ivec2 DisturbCoord = glm::ivec2(0, 0);
    } m_SimulationData;
};
