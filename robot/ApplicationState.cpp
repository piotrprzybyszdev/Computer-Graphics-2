#include <imgui.h>
#include <glm/glm.hpp>

#include <Core/Core.h>

#include <Vulkan/Application.h>
#include <Vulkan/Renderer/FrameGraphBuilder.h>

#include "ApplicationState.h"

#include <numeric>

using namespace ref;
using namespace ref::vulkan;

RobotUserInterfaceState::RobotUserInterfaceState(Scene& scene) : m_Scene(scene)
{
}

void RobotUserInterfaceState::OnUpdate(float /* timeStep */)
{
}

void RobotUserInterfaceState::OnKeyEvent(Key key, KeyAction action, Mods mods)
{
    m_Scene.OnKeyEvent(key, action, mods);
}

void RobotUserInterfaceState::OnMouseButtonEvent(ref::Button button, ref::ButtonAction action, ref::Mods mods)
{
    m_Scene.OnMouseButtonEvent(button, action, mods);
}

void RobotUserInterfaceState::OnCursorMoveEvent(double xpos, double ypos)
{
    m_Scene.OnCursorMoveEvent(xpos, ypos);
}

struct CameraConstants
{
    glm::mat4x4 Projection;
    glm::mat4x4 View;
    glm::vec4 Origin;
    glm::uint IsMirror;
    glm::uvec3 pad0;
    glm::vec4 CameraPosition;
    glm::vec4 MirrorNormal;
};

RobotApplicationState::RobotApplicationState(const ApplicationStateSpec& spec)
    : m_MainQueue(spec.Queues.at(Application::MainQueueName))
{
    ShaderId meshVertexShader = spec.ShaderLibrary->GetShaderByPath("Shaders/mesh.vert");
    ShaderId mirrorVertexShader = spec.ShaderLibrary->GetShaderByPath("Shaders/mirror.vert");
    ShaderId shadowVertexShader = spec.ShaderLibrary->GetShaderByPath("Shaders/shadow.vert");
    ShaderId particleVertexShader = spec.ShaderLibrary->GetShaderByPath("Shaders/particle.vert");
    ShaderId shadowGeometryShader = spec.ShaderLibrary->GetShaderByPath("Shaders/shadow.geom");
    ShaderId phongFragmentShader = spec.ShaderLibrary->GetShaderByPath("Shaders/phong.frag");
    ShaderId emptyFragmentShader = spec.ShaderLibrary->GetShaderByPath("Shaders/empty.frag");
    ShaderId ambientFragmentShader = spec.ShaderLibrary->GetShaderByPath("Shaders/ambient.frag");
    ShaderId particleFragmentShader = spec.ShaderLibrary->GetShaderByPath("Shaders/particle.frag");

    {
        GraphicsPipelineInfo pipelineInfo = {
            .Name = "Particle Pipeline",
            .VertexShaderId = particleVertexShader,
            .FragmentShaderId = particleFragmentShader,
        };

        auto pipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        GraphicsPipelineInstanceInfo pipelineInstanceInfo = {
            .Name = "Particle Pipeline Instance",
            .PipelineId = pipelineId,
            .ColorAttachmentFormats = { vk::Format::eR8G8B8A8Unorm },
            .DepthAttachmentFormat = vk::Format::eD24UnormS8Uint,
            .StencilAttachmentFormat = vk::Format::eD24UnormS8Uint,
        };

        pipelineInstanceInfo.InputAssemblyState.setTopology(vk::PrimitiveTopology::eTriangleStrip);
        pipelineInstanceInfo.RasterizationState.setLineWidth(1.0f);
        pipelineInstanceInfo.DepthStencilState.setDepthTestEnable(vk::True);
        pipelineInstanceInfo.DepthStencilState.setDepthCompareOp(vk::CompareOp::eLess);
        pipelineInstanceInfo.AttachmentBlendStates.emplace_back().setColorWriteMask(vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags).setBlendEnable(vk::True)
            .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha).setDstColorBlendFactor(vk::BlendFactor::eOne)
            .setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha).setDstAlphaBlendFactor(vk::BlendFactor::eOne);
        m_ParticlePipelineId = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);

        pipelineInstanceInfo.Name = "Particle Reflection Pipeline";
        pipelineInstanceInfo.DepthStencilState.setStencilTestEnable(vk::True);
        pipelineInstanceInfo.DepthStencilState.setBack(vk::StencilOpState().setCompareMask(0xff).setCompareOp(vk::CompareOp::eEqual).setReference(1));
        pipelineInstanceInfo.DepthStencilState.setFront(vk::StencilOpState().setCompareMask(0xff).setCompareOp(vk::CompareOp::eEqual).setReference(1));
        m_ParticleReflectionPipelineId = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);
    }

    {
        GraphicsPipelineInfo pipelineInfo = {
            .Name = "Mirror Stencil Pipeline",
            .VertexShaderId = meshVertexShader,
            .FragmentShaderId = emptyFragmentShader,
        };
        auto msPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        pipelineInfo.Name = "Reflection Pipeline";
        pipelineInfo.FragmentShaderId = phongFragmentShader;
        auto rPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        pipelineInfo.Name = "Mirror Pipeline";
        pipelineInfo.VertexShaderId = mirrorVertexShader;
        pipelineInfo.FragmentShaderId = particleFragmentShader;
        auto mPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        GraphicsPipelineInstanceInfo pipelineInstanceInfo = {
            .Name = "Mirror Stencil Pipeline Instance",
            .PipelineId = msPipelineId,
            .BindingDescriptions = { vk::VertexInputBindingDescription(0, sizeof(uint32_t)), vk::VertexInputBindingDescription(1, sizeof(glm::vec4)) },
            .VertexInputs = { { 0, 0 }, { 1, 0 } },
            .ColorAttachmentFormats = { vk::Format::eR8G8B8A8Unorm },
            .DepthAttachmentFormat = vk::Format::eD24UnormS8Uint,
            .StencilAttachmentFormat = vk::Format::eD24UnormS8Uint,
        };

        pipelineInstanceInfo.InputAssemblyState.setTopology(vk::PrimitiveTopology::eTriangleList);
        pipelineInstanceInfo.RasterizationState.setLineWidth(1.0f);
        pipelineInstanceInfo.DepthStencilState.setStencilTestEnable(vk::True);
        pipelineInstanceInfo.DepthStencilState.setBack(vk::StencilOpState().setWriteMask(0xff).setCompareOp(vk::CompareOp::eAlways).setPassOp(vk::StencilOp::eReplace).setReference(1));
        pipelineInstanceInfo.DepthStencilState.setFront(vk::StencilOpState().setWriteMask(0xff).setCompareOp(vk::CompareOp::eAlways).setPassOp(vk::StencilOp::eReplace).setReference(1));
        pipelineInstanceInfo.AttachmentBlendStates.emplace_back().setColorWriteMask(vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags);
        m_MirrorStencilPipelineId = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);

        pipelineInstanceInfo.Name = "Reflection Pipeline Instance";
        pipelineInstanceInfo.PipelineId = rPipelineId;
        pipelineInstanceInfo.RasterizationState.setCullMode(vk::CullModeFlagBits::eFront);
        pipelineInstanceInfo.DepthStencilState.setDepthTestEnable(vk::True);
        pipelineInstanceInfo.DepthStencilState.setDepthWriteEnable(vk::True);
        pipelineInstanceInfo.DepthStencilState.setDepthCompareOp(vk::CompareOp::eLess);
        pipelineInstanceInfo.DepthStencilState.setBack(vk::StencilOpState().setCompareMask(0xff).setCompareOp(vk::CompareOp::eEqual).setReference(1));
        pipelineInstanceInfo.DepthStencilState.setFront(vk::StencilOpState().setCompareMask(0xff).setCompareOp(vk::CompareOp::eEqual).setReference(1));
        m_ReflectionPipelineId = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);

        pipelineInstanceInfo.Name = "Mirror Pipeline Instance";
        pipelineInstanceInfo.PipelineId = mPipelineId;
        pipelineInstanceInfo.RasterizationState.setCullMode(vk::CullModeFlagBits::eNone);
        pipelineInstanceInfo.DepthStencilState.setStencilTestEnable(vk::False);
        pipelineInstanceInfo.DepthStencilState.setDepthCompareOp(vk::CompareOp::eAlways);
        pipelineInstanceInfo.AttachmentBlendStates.back().setColorWriteMask(vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags).setBlendEnable(vk::True)
            .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha).setDstColorBlendFactor(vk::BlendFactor::eOne)
            .setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha).setDstAlphaBlendFactor(vk::BlendFactor::eOne);
        m_MirrorPipelineId = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);
    }

    {
        GraphicsPipelineInfo pipelineInfo = {
            .Name = "Ambient Pipeline",
            .VertexShaderId = meshVertexShader,
            .FragmentShaderId = ambientFragmentShader,
        };

        auto aPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        pipelineInfo.Name = "Lighting Pipeline";
        pipelineInfo.FragmentShaderId = phongFragmentShader;

        auto lPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        pipelineInfo.Name = "Shadow Pipeline";
        pipelineInfo.VertexShaderId = shadowVertexShader;
        pipelineInfo.GeometryShaderId = shadowGeometryShader;
        pipelineInfo.FragmentShaderId = emptyFragmentShader;

        auto sPipelineId = spec.PipelineLibrary->AddPipeline(pipelineInfo);

        GraphicsPipelineInstanceInfo pipelineInstanceInfo = {
            .Name = "Ambient Pipeline Instance",
            .PipelineId = aPipelineId,
            .BindingDescriptions = { vk::VertexInputBindingDescription(0, sizeof(uint32_t)), vk::VertexInputBindingDescription(1, sizeof(glm::vec4)) },
            .VertexInputs = { { 0, 0 }, { 1, 0 } },            
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
        m_AmbientPipelineId = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);

        pipelineInstanceInfo.Name = "Lighting Pipeline Instance";
        pipelineInstanceInfo.PipelineId = lPipelineId;
        pipelineInstanceInfo.DepthStencilState.setStencilTestEnable(vk::True);
        pipelineInstanceInfo.DepthStencilState.setDepthCompareOp(vk::CompareOp::eEqual);
        pipelineInstanceInfo.DepthStencilState.setBack(vk::StencilOpState().setCompareMask(0xff).setCompareOp(vk::CompareOp::eEqual).setReference(0));
        pipelineInstanceInfo.DepthStencilState.setFront(vk::StencilOpState().setCompareMask(0xff).setCompareOp(vk::CompareOp::eEqual).setReference(0));
        m_LightingPipelineId = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);

        pipelineInstanceInfo.Name = "Shadow Pipeline Instance";
        pipelineInstanceInfo.PipelineId = sPipelineId;
        pipelineInstanceInfo.InputAssemblyState.setTopology(vk::PrimitiveTopology::ePointList);
        pipelineInstanceInfo.RasterizationState.setCullMode(vk::CullModeFlagBits::eNone);
        pipelineInstanceInfo.DepthStencilState.setDepthCompareOp(vk::CompareOp::eLess);
        pipelineInstanceInfo.DepthStencilState.setDepthWriteEnable(vk::False);
        pipelineInstanceInfo.DepthStencilState.setFront(vk::StencilOpState().setWriteMask(0xff).setCompareOp(vk::CompareOp::eAlways).setDepthFailOp(vk::StencilOp::eDecrementAndWrap));
        pipelineInstanceInfo.DepthStencilState.setBack(vk::StencilOpState().setWriteMask(0xff).setCompareOp(vk::CompareOp::eAlways).setDepthFailOp(vk::StencilOp::eIncrementAndWrap));
        pipelineInstanceInfo.AttachmentBlendStates.front().setColorWriteMask(vk::ColorComponentFlags());
        
        pipelineInstanceInfo.BindingDescriptions = { vk::VertexInputBindingDescription(0, sizeof(glm::uvec4)) };
        pipelineInstanceInfo.VertexInputs = { { 0, 0 } };
        
        m_ShadowPipelineId = spec.PipelineLibrary->AddPipelineInstance(pipelineInstanceInfo);
    }
}

RobotApplicationState::~RobotApplicationState()
{
}

void RobotApplicationState::OnEnter(vulkan::ApplicationState* /* previous */)
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

    m_UserInterfaceState = std::make_unique<RobotUserInterfaceState>(m_Scene);
    m_UserInterface = std::make_unique<UserInterface>(userInterfaceSpec, *m_UserInterfaceState);

    FrameGraphBuilder builder;

    builder.AddDeviceImage(
        "Image", vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(1280, 720, 1), 1, 1)
        .setUsage(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment), true, false
    );
    builder.AddDeviceImage(
        "Depth Stencil Image", vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eD24UnormS8Uint, vk::Extent3D(1280, 720, 1), 1, 1)
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment), true, false
    );
    builder.AddDeviceBuffer("Mesh Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetMeshes().size_bytes()), false, true);
    builder.AddDeviceBuffer("Vertex Position Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetPositions().size_bytes()), false, true);
    builder.AddDeviceBuffer("Vertex Position Index Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetVertexIndices().size_bytes()), false, true);
    builder.AddDeviceBuffer("Vertex Normal Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer).setSize(m_Scene.GetVertexNormals().size_bytes()), false, true);
    builder.AddDeviceBuffer("Index Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetTriangles().size_bytes()), false, true);
    builder.AddDeviceBuffer("Edge Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer).setSize(m_Scene.GetEdges().size_bytes()), false, true);
    builder.AddHostBuffer("Transform Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eUniformBuffer).setSize(m_Scene.GetTransforms().size_bytes()), true, true);
    builder.AddHostBuffer("Particle Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eUniformBuffer).setSize(m_Scene.GetParticles().size_bytes()), true, true);

    builder.AddHostBuffer("Camera Uniform Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eUniformBuffer).setSize(sizeof(CameraConstants)), true, true);
    builder.AddHostBuffer("Mirror Camera Uniform Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eUniformBuffer).setSize(sizeof(CameraConstants)), true, true);
    builder.AddDeviceBuffer("Light Buffer", vk::BufferCreateInfo().setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer).setSize(m_Scene.GetLights().size_bytes()), false, true);

    const Texture& sparkTexture = m_Scene.GetSparkTexture();
    const Texture& mirrorTexture = m_Scene.GetMirrorTexture();
    builder.AddDeviceImage(
        "Spark Texture", vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(sparkTexture.Width, sparkTexture.Height, 1), 1, 1)
        .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled), false, true
    );
    builder.AddDeviceImage(
        "Mirror Texture", vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, vk::Extent3D(mirrorTexture.Width, mirrorTexture.Height, 1), 1, 1)
        .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled), false, true
    );

    m_MeshIndices.resize(m_Scene.GetMeshes().size());
    std::ranges::iota(m_MeshIndices, 0);
    m_MirrorMeshIndex = m_Scene.GetMirrorMeshIndex();

    m_TextureSampler = spec.LogicalDevice.createSampler(vk::SamplerCreateInfo().setMinFilter(vk::Filter::eLinear).setMagFilter(vk::Filter::eLinear));

    std::vector<IndexedGraphicsPassSpec::DrawSpec> meshDraws = std::ranges::iota_view(0ull, m_Scene.GetStaticMeshes().size()) | std::views::transform([this](size_t index) {
        const auto& mesh = m_Scene.GetStaticMeshes()[index];
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

    IndexedGraphicsPassSpec::DrawSpec mirrorDraw = {
        .Command = {
            .IndexCount = m_Scene.GetMirrorMesh().TriangleCount * 3,
            .InstanceCount = 1,
            .FirstIndex = m_Scene.GetMirrorMesh().TriangleOffset * 3,
            .VertexOffset = m_Scene.GetMirrorMesh().VertexOffset,
            .FirstInstance = 0,
        },
        .PushConstantData = std::as_bytes(std::span(&m_MirrorMeshIndex, 1)),
    };

    {
        IndexedGraphicsPassSpec passSpec = {
            .Pipeline = m_MirrorStencilPipelineId,
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
            .Draws = { mirrorDraw },
        };
        builder.AddIndexedGraphicsPass("Mirror Stencil Pass", passSpec);
    }

    {
        IndexedGraphicsPassSpec passSpec = {
            .Pipeline = m_ReflectionPipelineId,
            .BufferBindings = {
                { "Mirror Camera Uniform Buffer", 0, true, false },
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
        builder.AddIndexedGraphicsPass("Reflection Pass", passSpec);
    }

    {
        GraphicsPassSpec passSpec = {
            .Pipeline = m_ParticleReflectionPipelineId,
            .BufferBindings = {
                { "Mirror Camera Uniform Buffer", 0, true, false },
                { "Particle Buffer", 1, true, false },
            },
            .ImageBindings = {
                { "Spark Texture", 4, m_TextureSampler, true, false},
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
            .Draws = {
                {
                    .Command = {
                        .VertexCount = 4,
                        .InstanceCount = static_cast<uint32_t>(m_Scene.GetParticles().size()),
                        .FirstVertex = 0,
                        .FirstInstance = 0,
                    },
                }
            },
        };
        builder.AddGraphicsPass("Particle Reflection Pass", passSpec);
    }

    {
        IndexedGraphicsPassSpec passSpec = {
            .Pipeline = m_MirrorPipelineId,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
                { "Vertex Position Buffer", 1, true, false },
                { "Mesh Buffer", 2, true, false },
                { "Transform Buffer", 3, true, false },
            },
            .ImageBindings = {
                { "Mirror Texture", 4, m_TextureSampler, true, false },
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
            .Draws = { mirrorDraw },
        };
        builder.AddIndexedGraphicsPass("Mirror Pass", passSpec);
    }

    {
        IndexedGraphicsPassSpec passSpec = {
            .Pipeline = m_AmbientPipelineId,
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
                },
            },
            .DepthAttachment = { {
                .ImageResource = "Depth Stencil Image",
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
            .Pipeline = m_ShadowPipelineId,
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
            .Pipeline = m_LightingPipelineId,
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
        GraphicsPassSpec passSpec = {
            .Pipeline = m_ParticlePipelineId,
            .BufferBindings = {
                { "Camera Uniform Buffer", 0, true, false },
                { "Particle Buffer", 1, true, false },
            },
            .ImageBindings = {
                { "Spark Texture", 4, m_TextureSampler, true, false},
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
            .Draws = {
                {
                    .Command = {
                        .VertexCount = 4,
                        .InstanceCount = static_cast<uint32_t>(m_Scene.GetParticles().size()),
                        .FirstVertex = 0,
                        .FirstInstance = 0,
                    },
                }
            },
        };
        builder.AddGraphicsPass("Particle Pass", passSpec);
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

    uploadBuffer("Mesh Buffer", std::as_bytes(m_Scene.GetMeshes()));
    uploadBuffer("Vertex Position Buffer", std::as_bytes(m_Scene.GetPositions()));
    uploadBuffer("Vertex Position Index Buffer", std::as_bytes(m_Scene.GetVertexIndices()));
    uploadBuffer("Vertex Normal Buffer", std::as_bytes(m_Scene.GetVertexNormals()));
    uploadBuffer("Index Buffer", std::as_bytes(m_Scene.GetTriangles()));
    uploadBuffer("Edge Buffer", std::as_bytes(m_Scene.GetEdges()));
    uploadBuffer("Light Buffer", std::as_bytes(m_Scene.GetLights()));

    m_Renderer->UploadWithStaging(
        m_FrameGraph->GetImage("Spark Texture").front().first, sparkTexture.Content, vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1)
    );
    m_Renderer->UploadWithStaging(
        m_FrameGraph->GetImage("Mirror Texture").front().first, mirrorTexture.Content, vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1)
    );
}

void RobotApplicationState::OnExit(vulkan::ApplicationState* /* next */)
{
    m_Renderer.reset();
    m_FrameGraph.reset();
    m_UserInterface.reset();
    m_UserInterfaceState.reset();
    m_ResourceAllocator.reset();
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
    resizeGraphicsPass(m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Mirror Stencil Pass"));
    resizeGraphicsPass(m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Mirror Pass"));
    resizeGraphicsPass(m_FrameGraph->GetIndexedGraphicsPassDynamicConfig("Reflection Pass"));
    resizeGraphicsPass(m_FrameGraph->GetGraphicsPassDynamicConfig("Particle Pass"));
    resizeGraphicsPass(m_FrameGraph->GetGraphicsPassDynamicConfig("Particle Reflection Pass"));

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
        auto transforms = m_Scene.GetParticles();
        auto bufferId = m_FrameGraph->GetCurrentBuffer("Particle Buffer");
        m_ResourceAllocator->UploadToBuffer(bufferId, transforms.data(), transforms.size_bytes());
    }

    {
        CameraConstants camera = {
            .Projection = m_Scene.GetCameraProjection(),
            .View = m_Scene.GetCameraView(),
            .Origin = m_Scene.GetCameraOrigin(),
            .IsMirror = 0,
            .CameraPosition = m_Scene.GetCameraOrigin(),
            .MirrorNormal = glm::vec4(),
        };
        auto bufferId = m_FrameGraph->GetCurrentBuffer("Camera Uniform Buffer");
        m_ResourceAllocator->UploadToBuffer(bufferId, &camera, sizeof(CameraConstants));
    }

    {
        CameraConstants camera = {
            .Projection = m_Scene.GetCameraProjection(),
            .View = m_Scene.GetMirrorViewMatrix(),
            .Origin = m_Scene.GetMirrorCameraOrigin(),
            .IsMirror = 1,
            .CameraPosition = m_Scene.GetCameraOrigin(),
            .MirrorNormal = m_Scene.GetTransforms()[m_Scene.GetMirrorMeshIndex()] * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f),
        };
        auto bufferId = m_FrameGraph->GetCurrentBuffer("Mirror Camera Uniform Buffer");
        m_ResourceAllocator->UploadToBuffer(bufferId, &camera, sizeof(CameraConstants));
    }
}

void RobotApplicationState::OnRender()
{
    m_Renderer->OnRender();
}
