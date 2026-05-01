#include <Core/UserInterface.h>

#include <Vulkan/ApplicationState.h>

#include <memory>
#include <vector>

#include "Scene.h"

class RobotUserInterface final : public ref::UserInterface
{
public:
    RobotUserInterface(const ref::UserInterfaceVulkanSpec& spec, Scene &scene);
    ~RobotUserInterface() override = default;

    void OnDefineUI(float timeStep) override;
    
    void OnKeyEvent(ref::Key key, ref::KeyAction action, ref::Mods mods) override;
    void OnMouseButtonEvent(ref::Button button, ref::ButtonAction action, ref::Mods mods) override;
    void OnCursorMoveEvent(double xpos, double ypos) override;

private:
    Scene &m_Scene;
};

class RobotApplicationState final : public ref::vulkan::ApplicationState
{
public:
    RobotApplicationState(const ref::vulkan::ApplicationStateSpec& spec);
    ~RobotApplicationState() override;

    void OnEnter(ref::vulkan::ApplicationState* previous) override;
    void OnExit(ref::vulkan::ApplicationState* next) override;

    void OnResize(const ref::vulkan::Swapchain* swapchain) override;

    void OnUpdate(float timeStep) override;
    void OnRender() override;

private:
    ref::vulkan::Queue m_MainQueue;

    ref::vulkan::GraphicsPipelineInstanceId m_ParticlePipelineId, m_ParticleReflectionPipelineId;
    ref::vulkan::GraphicsPipelineInstanceId m_MirrorStencilPipelineId, m_ReflectionPipelineId, m_MirrorPipelineId;
    ref::vulkan::GraphicsPipelineInstanceId m_LightingPipelineId, m_ShadowPipelineId, m_AmbientPipelineId;

    std::unique_ptr<RobotUserInterface> m_UserInterface;
    std::unique_ptr<ref::vulkan::FrameGraph> m_FrameGraph;
    std::unique_ptr<ref::vulkan::Renderer> m_Renderer;
    std::unique_ptr<ref::vulkan::ResourceAllocator> m_ResourceAllocator;

    Scene m_Scene;
    std::vector<uint32_t> m_MeshIndices;
    uint32_t m_MirrorMeshIndex;
    vk::Sampler m_TextureSampler;
};
