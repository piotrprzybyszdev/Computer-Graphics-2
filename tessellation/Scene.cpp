#define GLM_FORCE_LEFT_HANDED
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <ranges>
#include <vector>

#include "Scene.h"

Scene::Scene()
{
    m_Patches.push_back(CreatePatch0());
    m_Patches.push_back(CreatePatch1());
}

void Scene::OnResize(uint32_t width, uint32_t height)
{
    m_Camera.Projection = glm::perspectiveFov(70.0f, static_cast<float>(width), static_cast<float>(height), 0.01f, 1000.0f);
}

void Scene::OnUpdate(float /* timeStep */)
{
    // camera
    {
        const glm::vec2 delta = m_Camera.CurrentMousePosition - m_Camera.PrevMousePosition;
        if (m_Camera.IsLeftMouseButtonPressed)
        {
            m_Camera.Pitch -= delta.x * 0.005f;
            m_Camera.Yaw += delta.y * 0.005f;
            m_Camera.Yaw = glm::clamp(m_Camera.Yaw, glm::radians(-89.0f), glm::radians(89.0f));
        }
        if (m_Camera.IsRightMouseButtonPressed)
        {
            m_Camera.Distance += delta.y * 0.005f;
        }
        m_Camera.PrevMousePosition = m_Camera.CurrentMousePosition;

        const glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f);
        const glm::vec3 at = glm::vec3(0.0f);
        const glm::vec3 direction = glm::rotate(glm::mat4x4(1.0f), m_Camera.Pitch, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::mat4x4(1.0f), m_Camera.Yaw, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

        m_Camera.Origin = glm::vec4(at - direction * m_Camera.Distance, 1.0f);
        m_Camera.View = glm::lookAt(glm::vec3(m_Camera.Origin), at, up);
    }
}

void Scene::OnKeyEvent(ref::Key key, ref::KeyAction action, ref::Mods /* mods */)
{
    if (action == ref::KeyAction::Release)
    {
        switch (key)
        {
        case ref::Key::A:
            m_TessellationControls.InsideTessFactor--;
            break;
        case ref::Key::D:
            m_TessellationControls.InsideTessFactor++;
            break;
        case ref::Key::Q:
            m_TessellationControls.OutsideTessFactor--;
            break;
        case ref::Key::E:
            m_TessellationControls.OutsideTessFactor++;
            break;
        case ref::Key::Space:
            m_CurrentPatchIndex = m_CurrentPatchIndex == 0 ? 1 : 0;
            break;
        case ref::Key::C:
            m_TessellationControls.ShowControlLines = !m_TessellationControls.ShowControlLines;
            break;
        case ref::Key::P:
            m_TessellationControls.ShadePhong = !m_TessellationControls.ShadePhong;
            break;
        }
    }
}

void Scene::OnMouseButtonEvent(ref::Button button, ref::ButtonAction action, ref::Mods /* mods */)
{
    if (button == ref::Button::Left)
        m_Camera.IsLeftMouseButtonPressed = action == ref::ButtonAction::Press;
    if (button == ref::Button::Right)
        m_Camera.IsRightMouseButtonPressed = action == ref::ButtonAction::Press;
}

void Scene::OnCursorMoveEvent(double xpos, double ypos)
{
    m_Camera.CurrentMousePosition = glm::vec2(xpos, ypos);
}

const glm::mat4x4& Scene::GetCameraProjection() const
{
    return m_Camera.Projection;
}

const glm::mat4x4& Scene::GetCameraView() const
{
    return m_Camera.View;
}

const glm::vec4& Scene::GetCameraOrigin() const
{
    return m_Camera.Origin;
}

const TessellationControls& Scene::GetTessellationControls() const
{
    return m_TessellationControls;
}

std::span<const Vertex> Scene::GetVertices() const
{
    return m_Vertices;
}

std::span<const uint32_t> Scene::GetIndices() const
{
    return m_Indices;
}

std::span<const Patch> Scene::GetPatches() const
{
    return m_Patches;
}

uint32_t Scene::GetCurrentPatchIndex() const
{
    return m_CurrentPatchIndex;
}

Patch Scene::CreatePatch0()
{
    Patch mesh = {
        .VertexOffset = static_cast<uint32_t>(m_Vertices.size()),
        .IndexOffset = static_cast<uint32_t>(m_Indices.size()),
    };

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
        {
            const float x = -1.0f + 2.0f * i / 3.0f;
            const float y = i > 0 && i < 3 && j > 0 && j < 3 ? -1.0f : 0.0f;
            const float z = -1.0f + 2.0f * j / 3.0f;
            m_Vertices.emplace_back(glm::vec4(x, y, z, 1.0f));
        }

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 3; j++)
        {
            m_Indices.push_back(i * 4 + j);
            m_Indices.push_back(i * 4 + j + 1);

            m_Indices.push_back(j * 4 + i);
            m_Indices.push_back((j + 1) * 4 + i);
        }

    mesh.VertexCount = static_cast<uint32_t>(m_Vertices.size()) - mesh.VertexOffset;
    mesh.IndexCount = static_cast<uint32_t>(m_Indices.size()) - mesh.IndexOffset;

    return mesh;
}

Patch Scene::CreatePatch1()
{
    Patch mesh = {
        .VertexOffset = static_cast<uint32_t>(m_Vertices.size()),
        .IndexOffset = static_cast<uint32_t>(m_Indices.size()),
    };

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
        {
            const float x = -1.0f + 2.0f * i / 3.0f;
            const float y = i > 0 && i < 3 ? 1.0f : 0.0f;
            const float z = -1.0f + 2.0f * j / 3.0f;
            m_Vertices.emplace_back(glm::vec4(x, y, z, 1.0f));
        }

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 3; j++)
        {
            m_Indices.push_back(i * 4 + j);
            m_Indices.push_back(i * 4 + j + 1);

            m_Indices.push_back(j * 4 + i);
            m_Indices.push_back((j + 1) * 4 + i);
        }

    mesh.VertexCount = static_cast<uint32_t>(m_Vertices.size()) - mesh.VertexOffset;
    mesh.IndexCount = static_cast<uint32_t>(m_Indices.size()) - mesh.IndexOffset;

    return mesh;
}

Texture Scene::LoadTexture(const std::filesystem::path& path)
{
    int x, y, channels;
    stbi_uc *data = stbi_load(path.string().c_str(), &x, &y, &channels, STBI_rgb_alpha);
    const size_t size = 4ull * x * y;

    Texture texture = { static_cast<uint32_t>(x), static_cast<uint32_t>(y) };
    std::byte* content = reinterpret_cast<std::byte*>(data);
    texture.Content.assign(content, content + size);

    stbi_image_free(data);

    return texture;
}
