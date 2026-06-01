#define GLM_FORCE_LEFT_HANDED
#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <ranges>
#include <vector>

#include "Scene.h"

Scene::Scene()
{
    m_Patches.push_back(CreatePatch0());
    m_Patches.push_back(CreatePatch1());
    m_Patches.push_back(CreatePatch2());

    m_DiffuseTexture = LoadTexture("assets/tessellation/diffuse.dds");
    m_HeightTexture = LoadTexture("assets/tessellation/height.dds");
    m_NormalTexture = LoadTexture("assets/tessellation/normals.dds");
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
    auto updateTessFactor = [](float factor, float change) {
        return std::clamp(((factor - 1.0f) * 10.0f + change) / 10.0f + 1.0f, 0.1f, 2.0f);
    };

    if (action == ref::KeyAction::Release)
    {
        switch (key)
        {
        case ref::Key::A:
            m_TessellationControls.InsideTessFactor = updateTessFactor(m_TessellationControls.InsideTessFactor, -1.0f);
            break;
        case ref::Key::D:
            m_TessellationControls.InsideTessFactor = updateTessFactor(m_TessellationControls.InsideTessFactor, 1.0f);
            break;
        case ref::Key::Q:
            m_TessellationControls.OutsideTessFactor = updateTessFactor(m_TessellationControls.OutsideTessFactor, -1.0f);
            break;
        case ref::Key::E:
            m_TessellationControls.OutsideTessFactor = updateTessFactor(m_TessellationControls.OutsideTessFactor, 1.0f);
            break;
        case ref::Key::Space:
            m_CurrentPatchIndex = (++m_CurrentPatchIndex) % m_Patches.size();
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
    m_Camera.IsLeftMouseButtonPressed = false;
    m_Camera.IsRightMouseButtonPressed = false;
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

const Texture& Scene::GetDiffuseTexture() const
{
    return m_DiffuseTexture;
}

const Texture& Scene::GetHeightTexture() const
{
    return m_HeightTexture;
}

const Texture& Scene::GetNormalTexture() const
{
    return m_NormalTexture;
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

Patch Scene::CreatePatch2()
{
    Patch mesh = { 
        .VertexOffset = static_cast<uint32_t>(m_Vertices.size()),
        .IndexOffset = static_cast<uint32_t>(m_Indices.size()),
    };

    for (int patchI = 0; patchI < 4; patchI++)
    {
        for (int patchJ = 0; patchJ < 4; patchJ++)
        {
            const float fromX = std::lerp(-1.0f, 1.0f, patchI / 4.0f);
            const float toX = std::lerp(-1.0f, 1.0f, (patchI + 1) / 4.0f);
            const float fromZ = std::lerp(-1.0f, 1.0f, patchJ / 4.0f);
            const float toZ = std::lerp(-1.0f, 1.0f, (patchJ + 1) / 4.0f);

            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                {
                    const float x = std::lerp(fromX, toX, i / 3.0f);
                    const float y = (i > 0 && i < 3 ? 0.333f : 0.0f) * (patchI % 2 == 0 ? 1 : -1);
                    const float z = std::lerp(fromZ, toZ, j / 3.0f);
                    m_Vertices.emplace_back(glm::vec4(x, y, z, 1.0f));
                }


            const int offset = ((patchI * 4) + patchJ) * 16;
            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 3; j++)
                {
                    m_Indices.push_back(offset + i * 4 + j);
                    m_Indices.push_back(offset + i * 4 + j + 1);

                    m_Indices.push_back(offset + j * 4 + i);
                    m_Indices.push_back(offset + (j + 1) * 4 + i);
                }
        }
    }
    
    mesh.VertexCount = static_cast<uint32_t>(m_Vertices.size()) - mesh.VertexOffset;
    mesh.IndexCount = static_cast<uint32_t>(m_Indices.size()) - mesh.IndexOffset;

    return mesh;
}

Texture Scene::LoadTexture(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    assert(file.is_open());

    struct DDS_PIXELFORMAT {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwFourCC;
        uint32_t dwRGBBitCount;
        uint32_t dwRBitMask;
        uint32_t dwGBitMask;
        uint32_t dwBBitMask;
        uint32_t dwABitMask;
    };

    struct DDS_HEADER {
        uint32_t           dwSize;
        uint32_t           dwFlags;
        uint32_t           dwHeight;
        uint32_t           dwWidth;
        uint32_t           dwPitchOrLinearSize;
        uint32_t           dwDepth;
        uint32_t           dwMipMapCount;
        uint32_t           dwReserved1[11];
        DDS_PIXELFORMAT    ddspf;
        uint32_t           dwCaps;
        uint32_t           dwCaps2;
        uint32_t           dwCaps3;
        uint32_t           dwCaps4;
        uint32_t           dwReserved2;
    };

    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), 4);
    assert(magic == 0x20534444);

    DDS_HEADER header;
    file.read(reinterpret_cast<char *>(&header), sizeof(DDS_HEADER));
    assert((header.ddspf.dwFlags & 0x4) == 0);
    
    assert(std::has_single_bit(header.dwWidth));
    assert(std::has_single_bit(header.dwHeight));
    Texture texture = {
        .Width = header.dwWidth,
        .Height = header.dwHeight,
    };
    texture.Content.resize(header.dwMipMapCount);

    if (header.ddspf.dwRBitMask == 0x00ff0000 && header.ddspf.dwGBitMask == 0x0000ff00 && header.ddspf.dwBBitMask == 0x000000ff)
        texture.Format = Texture::PixelFormat::BGRA8;
    else if (header.ddspf.dwRBitMask == 0x000000ff && header.ddspf.dwGBitMask == 0x0000ff00 && header.ddspf.dwBBitMask == 0x00ff0000)
        texture.Format = Texture::PixelFormat::RGBA8;
    else
        std::terminate();

    size_t size = 4 * texture.Width * texture.Height;
    for (int i = 0; i < texture.Content.size(); i++)
    {
        texture.Content[i].resize(size);
        file.read(reinterpret_cast<char*>(texture.Content[i].data()), texture.Content[i].size());
        size /= 4;
    }
    
    return texture;
}
