#define GLM_FORCE_LEFT_HANDED
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>
#include <ranges>
#include <vector>

#include "Scene.h"

Scene::Scene()
{
    m_Meshes.push_back(CreateUnitSquareMesh());
    m_Meshes.push_back(LoadMesh("assets/duck/duck.txt"));

    // Water
    m_Transforms.push_back(glm::rotate(glm::mat4x4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
    m_Instances.push_back(Instance(0, 0));

    // Duck
    m_Transforms.push_back(glm::scale(glm::mat4x4(1.0f), glm::vec3(0.002f)));
    m_Instances.push_back(Instance(1, 1));
    m_DuckTexture = LoadTexture("assets/duck/ducktex.jpg");
}

void Scene::OnResize(uint32_t width, uint32_t height)
{
    m_Camera.Projection = glm::perspectiveFov(45.0f, static_cast<float>(width), static_cast<float>(height), 0.1f, 1000.0f);
}

void Scene::OnUpdate(float /* timeStep */)
{
    const glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f);
    m_Camera.Origin = glm::vec4(0.0f, 1.0f, 2.0f, 1.0f);
    m_Camera.View = glm::lookAt(glm::vec3(m_Camera.Origin), glm::vec3(0.0f, 0.0f, 0.0f), up);
}

void Scene::OnKeyEvent(ref::Key /* key */, ref::KeyAction /* action */, ref::Mods /* mods */)
{
}

void Scene::OnMouseButtonEvent(ref::Button /* button */, ref::ButtonAction /* action */, ref::Mods /* mods */)
{
}

void Scene::OnCursorMoveEvent(double /* xpos */, double /* ypos */)
{
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

std::span<const Vertex> Scene::GetVertices() const
{
    return m_Vertices;
}

std::span<const uint32_t> Scene::GetIndices() const
{
    return m_Indices;
}

std::span<const glm::mat4x4> Scene::GetTransforms() const
{
    return m_Transforms;
}

std::span<const Mesh> Scene::GetMeshes() const
{
    return m_Meshes;
}

std::span<const Instance> Scene::GetInstances() const
{
    return m_Instances;
}

uint32_t Scene::GetWaterInstanceIndex() const
{
    return 0;
}

uint32_t Scene::GetDuckInstanceIndex() const
{
    return 1;
}

const Texture& Scene::GetDuckTexture() const
{
    return m_DuckTexture;
}

Mesh Scene::CreateUnitSquareMesh()
{
    Mesh mesh = {
        .VertexOffset = static_cast<uint32_t>(m_Vertices.size()),
        .IndexOffset = static_cast<uint32_t>(m_Indices.size()),
    };

    m_Vertices.emplace_back(glm::vec4(-1, -1, 0, 1), glm::vec4(0, 0, 1, 0), glm::vec2(0, 0));
    m_Vertices.emplace_back(glm::vec4(1, -1, 0, 1), glm::vec4(0, 0, 1, 0), glm::vec2(1, 0));
    m_Vertices.emplace_back(glm::vec4(1, 1, 0, 1), glm::vec4(0, 0, 1, 0), glm::vec2(1, 1));
    m_Vertices.emplace_back(glm::vec4(-1, 1, 0, 1), glm::vec4(0, 0, 1, 0), glm::vec2(0, 1));
    m_Indices.append_range(std::array<uint32_t, 3>{ 0, 1, 2 });
    m_Indices.append_range(std::array<uint32_t, 3>{ 2, 3, 0 });

    mesh.VertexCount = static_cast<uint32_t>(m_Vertices.size()) - mesh.VertexOffset;
    mesh.IndexCount = static_cast<uint32_t>(m_Indices.size()) - mesh.IndexOffset;

    return mesh;
}

Mesh Scene::LoadMesh(const std::filesystem::path& path)
{
    Mesh mesh = {
        .VertexOffset = static_cast<uint32_t>(m_Vertices.size()),
        .IndexOffset = static_cast<uint32_t>(m_Indices.size()),
    };

    std::ifstream file(path, std::ios::in);
    assert(file.is_open());

    uint32_t v;
    file >> v;

    for (uint32_t i = 0; i < v; i++)
    {
        Vertex &vertex = m_Vertices.emplace_back();
        file >> vertex.Position.x >> vertex.Position.y >> vertex.Position.z;
        file >> vertex.Normal.x >> vertex.Normal.y >> vertex.Normal.z;
        file >> vertex.TexCoord.x >> vertex.TexCoord.y;
    }

    uint32_t t;
    file >> t;

    for (uint32_t i = 0; i < t * 3; i++)
        file >> m_Indices.emplace_back();

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
