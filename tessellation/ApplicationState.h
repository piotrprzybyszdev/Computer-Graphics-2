#include <glm/glm.hpp>

#include <Core/UserInterface.h>

#include <Vulkan/ApplicationState.h>

#include <memory>

#include "Scene.h"

class TessellationUserInterface final : public ref::UserInterface
{
public:
    TessellationUserInterface(const ref::UserInterfaceVulkanSpec& spec, Scene &scene);
    ~TessellationUserInterface() override = default;

    void OnDefineUI(float timeStep) override;
    
    void OnKeyEvent(ref::Key key, ref::KeyAction action, ref::Mods mods) override;
    void OnMouseButtonEvent(ref::Button button, ref::ButtonAction action, ref::Mods mods) override;
    void OnCursorMoveEvent(double xpos, double ypos) override;

private:
    Scene &m_Scene;
};

class TessellationApplicationState final : public ref::vulkan::ApplicationState
{
public:
    TessellationApplicationState(const ref::vulkan::ApplicationStateSpec& spec);
    ~TessellationApplicationState() override;

    void OnEnter(ref::vulkan::ApplicationState* previous) override;
    void OnExit(ref::vulkan::ApplicationState* next) override;

    void OnResize(const ref::vulkan::Swapchain* swapchain) override;

    void OnUpdate(float timeStep) override;
    void OnRender() override;

private:
    ref::vulkan::Queue m_MainQueue;

    ref::vulkan::GraphicsPipelineInstanceId m_LinePipeline;
    ref::vulkan::GraphicsPipelineInstanceId m_PatchPipeline;

    std::unique_ptr<TessellationUserInterface> m_UserInterface;
    std::unique_ptr<ref::vulkan::FrameGraph> m_FrameGraph;
    std::unique_ptr<ref::vulkan::Renderer> m_Renderer;
    std::unique_ptr<ref::vulkan::ResourceAllocator> m_ResourceAllocator;

    Scene m_Scene;

    uint32_t m_LineColorIndex = 0;
    uint32_t m_PatchColorIndex = 1;

    vk::Sampler m_TextureSampler;
};
