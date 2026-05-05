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
    m_Meshes.push_back(CreateUnitCubeMesh());

    // Water
    m_Transforms.push_back(glm::rotate(glm::mat4x4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
    m_Instances.push_back(Instance(0, 0));

    // Duck
    m_Transforms.push_back(glm::scale(glm::mat4x4(1.0f), glm::vec3(0.002f)));
    m_Instances.push_back(Instance(1, 1));
    m_DuckTexture = LoadTexture("assets/duck/ducktex.jpg");

    std::uniform_real_distribution dist(-1.0f, 1.0f);
    for (int i = 0; i < 4; i++)
        m_Curve.Points[i] = glm::vec2(dist(m_Rng), dist(m_Rng));

    // Environment
    m_Transforms.push_back(glm::mat4x4(1.0f));
    m_Instances.push_back(Instance(2, 2));

    m_EnvironmentTextures[0] = LoadTexture("assets/duck/environment/posx.jpg");
    m_EnvironmentTextures[1] = LoadTexture("assets/duck/environment/negx.jpg");
    m_EnvironmentTextures[2] = LoadTexture("assets/duck/environment/posy.jpg");
    m_EnvironmentTextures[3] = LoadTexture("assets/duck/environment/negy.jpg");
    m_EnvironmentTextures[4] = LoadTexture("assets/duck/environment/posz.jpg");
    m_EnvironmentTextures[5] = LoadTexture("assets/duck/environment/negz.jpg");
}

void Scene::OnResize(uint32_t width, uint32_t height)
{
    m_Camera.Projection = glm::perspectiveFov(45.0f, static_cast<float>(width), static_cast<float>(height), 0.1f, 1000.0f);
}

void Scene::OnUpdate(float timeStep)
{
    // camera
    {
        const glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f);
        m_Camera.Origin = glm::vec4(0.0f, 1.0f, 2.0f, 1.0f);
        m_Camera.View = glm::lookAt(glm::vec3(m_Camera.Origin), glm::vec3(0.0f, 0.0f, 0.0f), up);
    }

    // disturbance
    {
        std::uniform_int_distribution intDist(0, 10);
        if (intDist(m_Rng) == 0)
        {
            std::uniform_real_distribution dist(-1.0f, 1.0f);
            m_Disturbance = glm::vec2(dist(m_Rng), dist(m_Rng));
        }
        else
            m_Disturbance = std::nullopt;
    }

    // duck
    {
        m_Curve.ElapsedTime += timeStep / 5000;
        if (m_Curve.ElapsedTime > Curve::s_MaxTime)
        {
            m_Curve.ElapsedTime = 0.0f;
            std::uniform_real_distribution dist(-1.0f, 1.0f);
            for (int i = 0; i < 3; i++)
                m_Curve.Points[i] = m_Curve.Points[i + 1];
            m_Curve.Points[3] = glm::vec2(dist(m_Rng), dist(m_Rng));
        }

        const float t = m_Curve.ElapsedTime / Curve::s_MaxTime;
        const glm::vec2 pos = glm::vec2(1.0f / 6.0f) * (
            (-m_Curve.Points[0] + 3.0f * m_Curve.Points[1] - 3.0f * m_Curve.Points[2] + m_Curve.Points[3]) * t * t * t +
            (3.0f * m_Curve.Points[0] - 6.0f * m_Curve.Points[1] + 3.0f * m_Curve.Points[2]) * t * t +
            (-3.0f * m_Curve.Points[0] + 3.0f * m_Curve.Points[2]) * t +
            (m_Curve.Points[0] + 4.0f * m_Curve.Points[1] + m_Curve.Points[2])
            );
        const glm::vec2 deriv = glm::vec2(1.0f / 6.0f) * (
            (-m_Curve.Points[0] + 3.0f * m_Curve.Points[1] - 3.0f * m_Curve.Points[2] + m_Curve.Points[3]) * 3.0f * t * t +
            (3.0f * m_Curve.Points[0] - 6.0f * m_Curve.Points[1] + 3.0f * m_Curve.Points[2]) * 2.0f * t +
            (-3.0f * m_Curve.Points[0] + 3.0f * m_Curve.Points[2])
            );

        const float alpha = std::atan2(0.0f, -1.0f) - std::atan2(deriv.y, deriv.x);

        m_DuckDisturbance = pos;
        m_Transforms[m_Instances[GetDuckInstanceIndex()].TransformIndex] = glm::scale(glm::rotate(glm::translate(glm::mat4x4(1.0f), glm::vec3(pos.x, 0.0f, pos.y)), alpha, glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.002f));
    }
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

uint32_t Scene::GetEnvironmentInstanceIndex() const
{
    return 2;
}

const Texture& Scene::GetDuckTexture() const
{
    return m_DuckTexture;
}

std::span<const Texture, 6> Scene::GetEnvironmentTextures() const
{
    return m_EnvironmentTextures;
}

std::optional<glm::vec2> Scene::GetDisturbance() const
{
    return m_Disturbance;
}

glm::vec2 Scene::GetDuckDisturbance() const
{
    return m_DuckDisturbance;
}

Mesh Scene::CreateUnitSquareMesh()
{
    Mesh mesh = {
        .VertexOffset = static_cast<uint32_t>(m_Vertices.size()),
        .IndexOffset = static_cast<uint32_t>(m_Indices.size()),
    };

    m_Vertices.emplace_back(glm::vec4(-1, -1, 0, 1), glm::vec4(0, 0, 1, 0), glm::vec2(0, 1));
    m_Vertices.emplace_back(glm::vec4(1, -1, 0, 1), glm::vec4(0, 0, 1, 0), glm::vec2(1, 1));
    m_Vertices.emplace_back(glm::vec4(1, 1, 0, 1), glm::vec4(0, 0, 1, 0), glm::vec2(1, 0));
    m_Vertices.emplace_back(glm::vec4(-1, 1, 0, 1), glm::vec4(0, 0, 1, 0), glm::vec2(0, 0));
    m_Indices.append_range(std::array<uint32_t, 3>{ 0, 1, 2 });
    m_Indices.append_range(std::array<uint32_t, 3>{ 2, 3, 0 });

    mesh.VertexCount = static_cast<uint32_t>(m_Vertices.size()) - mesh.VertexOffset;
    mesh.IndexCount = static_cast<uint32_t>(m_Indices.size()) - mesh.IndexOffset;

    return mesh;
}

Mesh Scene::CreateUnitCubeMesh()
{
    Mesh mesh = {
        .VertexOffset = static_cast<uint32_t>(m_Vertices.size()),
        .IndexOffset = static_cast<uint32_t>(m_Indices.size()),
    };

    m_Vertices.emplace_back(glm::vec4(-1, -1, 1, 1), glm::vec4(0, 0, -1, 0), glm::vec2(0, 1));
    m_Vertices.emplace_back(glm::vec4(1, -1, 1, 1), glm::vec4(0, 0, -1, 0), glm::vec2(1, 1));
    m_Vertices.emplace_back(glm::vec4(1, 1, 1, 1), glm::vec4(0, 0, -1, 0), glm::vec2(1, 0));
    m_Vertices.emplace_back(glm::vec4(-1, 1, 1, 1), glm::vec4(0, 0, -1, 0), glm::vec2(0, 0));
    m_Indices.append_range(std::array<uint32_t, 3>{ 2, 1, 0 });
    m_Indices.append_range(std::array<uint32_t, 3>{ 0, 3, 2 });

    m_Vertices.emplace_back(glm::vec4(-1, -1, -1, 1), glm::vec4(0, 0, 1, 0), glm::vec2(0, 1));
    m_Vertices.emplace_back(glm::vec4(1, -1, -1, 1), glm::vec4(0, 0, 1, 0), glm::vec2(1, 1));
    m_Vertices.emplace_back(glm::vec4(1, 1, -1, 1), glm::vec4(0, 0, 1, 0), glm::vec2(1, 0));
    m_Vertices.emplace_back(glm::vec4(-1, 1, -1, 1), glm::vec4(0, 0, 1, 0), glm::vec2(0, 0));
    m_Indices.append_range(std::array<uint32_t, 3>{ 4, 5, 6 });
    m_Indices.append_range(std::array<uint32_t, 3>{ 6, 7, 4 });

    m_Vertices.emplace_back(glm::vec4(-1, -1, -1, 1), glm::vec4(1, 0, 0, 0), glm::vec2(0, 1));
    m_Vertices.emplace_back(glm::vec4(-1, 1, -1, 1), glm::vec4(1, 0, 0, 0), glm::vec2(1, 1));
    m_Vertices.emplace_back(glm::vec4(-1, 1, 1, 1), glm::vec4(1, 0, 0, 0), glm::vec2(1, 0));
    m_Vertices.emplace_back(glm::vec4(-1, -1, 1, 1), glm::vec4(1, 0, 0, 0), glm::vec2(0, 0));
    m_Indices.append_range(std::array<uint32_t, 3>{ 8, 9, 10 });
    m_Indices.append_range(std::array<uint32_t, 3>{ 10, 11, 8 });

    m_Vertices.emplace_back(glm::vec4(1, -1, -1, 1), glm::vec4(-1, 0, 0, 0), glm::vec2(0, 1));
    m_Vertices.emplace_back(glm::vec4(1, 1, -1, 1), glm::vec4(-1, 0, 0, 0), glm::vec2(1, 1));
    m_Vertices.emplace_back(glm::vec4(1, 1, 1, 1), glm::vec4(-1, 0, 0, 0), glm::vec2(1, 0));
    m_Vertices.emplace_back(glm::vec4(1, -1, 1, 1), glm::vec4(-1, 0, 0, 0), glm::vec2(0, 0));
    m_Indices.append_range(std::array<uint32_t, 3>{ 14, 13, 12 });
    m_Indices.append_range(std::array<uint32_t, 3>{ 12, 15, 14 });

    m_Vertices.emplace_back(glm::vec4(-1, -1, -1, 1), glm::vec4(0, 1, 0, 0), glm::vec2(0, 1));
    m_Vertices.emplace_back(glm::vec4(1, -1, -1, 1), glm::vec4(0, 1, 0, 0), glm::vec2(1, 1));
    m_Vertices.emplace_back(glm::vec4(1, -1, 1, 1), glm::vec4(0, 1, 0, 0), glm::vec2(1, 0));
    m_Vertices.emplace_back(glm::vec4(-1, -1, 1, 1), glm::vec4(0, 1, 0, 0), glm::vec2(0, 0));
    m_Indices.append_range(std::array<uint32_t, 3>{ 18, 17, 16 });
    m_Indices.append_range(std::array<uint32_t, 3>{ 16, 19, 18 });

    m_Vertices.emplace_back(glm::vec4(-1, 1, -1, 1), glm::vec4(0, -1, 0, 0), glm::vec2(0, 1));
    m_Vertices.emplace_back(glm::vec4(1, 1, -1, 1), glm::vec4(0, -1, 0, 0), glm::vec2(1, 1));
    m_Vertices.emplace_back(glm::vec4(1, 1, 1, 1), glm::vec4(0, -1, 0, 0), glm::vec2(1, 0));
    m_Vertices.emplace_back(glm::vec4(-1, 1, 1, 1), glm::vec4(0, -1, 0, 0), glm::vec2(0, 0));
    m_Indices.append_range(std::array<uint32_t, 3>{ 20, 21, 22 });
    m_Indices.append_range(std::array<uint32_t, 3>{ 22, 23, 20 });

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
