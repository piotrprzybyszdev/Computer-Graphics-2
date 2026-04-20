#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cassert>
#include <format>
#include <fstream>
#include <numbers>
#include <ranges>
#include <vector>

#include "Scene.h"

Scene::Scene()
{
    for (int i = 0; i < 6; i++)
        m_Meshes.Meshes.push_back(LoadRobotMesh(std::format("assets/puma/mesh{}.txt", i + 1)));
    m_Meshes.Transforms.resize(6, glm::mat4x4(1.0f));

    m_Meshes.Meshes.push_back(CreateCylinderMesh(1.0f, 10.0f, 10, 3));
    
    m_Meshes.Transforms.push_back(
        glm::scale(glm::rotate(glm::translate(glm::mat4x4(1.0f), glm::vec3(-1.5f, -1.0f, -3.0f)), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.4f, 0.4f, 0.5f))
    );

    m_Meshes.Meshes.push_back(CreateUnitCubeMesh());
    m_Meshes.Transforms.push_back(glm::scale(glm::mat4x4(1.0f), glm::vec3(3.0f, 2.0f, 3.0f)));

    // TODO: mirror transform matrix
    m_Meshes.Meshes.push_back(CreateUnitSquareMesh());
    m_Meshes.Transforms.push_back(glm::scale(glm::rotate(glm::translate(glm::mat4x4(1.0f), glm::vec3(2.0f, -0.3f, -1.5f)), glm::radians(-40.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.4f)));

    m_Lights.push_back(Light(glm::vec4(0.5f, 1.0f, 0.0f, 1.0f)));

    m_SparkTexture = LoadTexture("assets/spark.png");

    const size_t particleCount = 10;
    m_Particles.resize(particleCount, Particle(glm::mat4x4(1.0f), 0.25f));
}

void Scene::OnResize(uint32_t width, uint32_t height)
{
    m_Camera.Projection = glm::perspectiveFov(45.0f, static_cast<float>(width), static_cast<float>(height), 0.1f, 1000.0f);
}

void Scene::OnUpdate(float /* timeStep */)
{
    // TODO: camera controls
    m_Camera.Origin = glm::vec4(0.0f, 0.0f, 2.0f, 1.0f);
    m_Camera.View = glm::lookAt(glm::vec3(m_Camera.Origin), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));

    // TODO: inverse kinematics
    for (int i = 0; i < 6; i++)
        m_Meshes.Transforms[i] = glm::mat4x4(1.0f);

    // TODO: particle simulation
    for (int i = 0; i < m_Particles.size(); i++)
        m_Particles[i] = {
            .Transform = glm::rotate(glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f)), glm::radians(360.0f * (i / static_cast<float>(m_Particles.size()))), glm::vec3(0.0f, 0.0f, 1.0f)),
            .Alpha = 0.25f,
        };
}

void Scene::OnKeyRelease(ref::Key /* key */)
{
    // TODO: camera controls
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

std::span<const glm::vec4> Scene::GetPositions() const
{
    return m_Meshes.Positions;
}

std::span<const uint32_t> Scene::GetVertexIndices() const
{
    return m_Meshes.VertexIndices;
}

std::span<const glm::vec4> Scene::GetVertexNormals() const
{
    return m_Meshes.VertexNormals;
}

std::span<const glm::uvec3> Scene::GetTriangles() const
{
    return m_Meshes.Triangles;
}

std::span<const glm::uvec4> Scene::GetEdges() const
{
    return m_Meshes.Edges;
}

std::span<const glm::mat4x4> Scene::GetTransforms() const
{
    return m_Meshes.Transforms;
}

std::span<const Mesh> Scene::GetMeshes() const
{
    return m_Meshes.Meshes;
}

std::span<const Mesh> Scene::GetRobotMeshes() const
{
    return std::span(m_Meshes.Meshes.data(), 6);
}

std::span<const Mesh> Scene::GetStaticMeshes() const
{
    return std::span(m_Meshes.Meshes.data(), 8);
}

const Mesh& Scene::GetMirrorMesh() const
{
    return m_Meshes.Meshes[GetMirrorMeshIndex()];
}

uint32_t Scene::GetMirrorMeshIndex() const
{
    return 8;
}

glm::mat4x4 Scene::GetMirrorViewMatrix() const
{
    // TODO: mirror camera view matrix
    const glm::mat4x4 &transform = m_Meshes.Transforms[GetMirrorMeshIndex()];
    return m_Camera.View * transform * glm::scale(glm::mat4x4(1.0f), glm::vec3(1.0f, 1.0f, -1.0f)) * glm::inverse(transform);
}

glm::vec4 Scene::GetMirrorCameraOrigin() const
{
    // TODO: mirror camera origin
    const glm::mat4x4& transform = m_Meshes.Transforms[GetMirrorMeshIndex()];
    return transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

std::span<const Light> Scene::GetLights() const
{
    return m_Lights;
}

const Texture& Scene::GetSparkTexture() const
{
    return m_SparkTexture;
}

std::span<const Particle> Scene::GetParticles() const
{
    return m_Particles;
}

Mesh Scene::LoadRobotMesh(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::in);
    assert(file.is_open());

    Mesh mesh = {
        .PositionOffset = static_cast<uint32_t>(m_Meshes.Positions.size()),
        .VertexOffset = static_cast<uint32_t>(m_Meshes.VertexIndices.size()),
        .TriangleOffset = static_cast<uint32_t>(m_Meshes.Triangles.size()),
        .EdgeOffset = static_cast<uint32_t>(m_Meshes.Edges.size()),
    };

    file >> mesh.PositionCount;
    for (uint32_t i = 0; i < mesh.PositionCount; i++)
    {
        auto& pos = m_Meshes.Positions.emplace_back();
        file >> pos.x >> pos.y >> pos.z;
    }

    file >> mesh.VertexCount;
    for (uint32_t i = 0; i < mesh.VertexCount; i++)
    {
        auto& idx = m_Meshes.VertexIndices.emplace_back();
        auto& normal = m_Meshes.VertexNormals.emplace_back();
        file >> idx >> normal.x >> normal.y >> normal.z;
    }

    file >> mesh.TriangleCount;
    for (uint32_t i = 0; i < mesh.TriangleCount; i++)
    {
        auto& triangle = m_Meshes.Triangles.emplace_back();
        file >> triangle.x >> triangle.y >> triangle.z;
    }

    file >> mesh.EdgeCount;
    assert(mesh.TriangleCount % 2 == 0 && mesh.EdgeCount == mesh.TriangleCount * 3 / 2);
    for (uint32_t i = 0; i < mesh.EdgeCount; i++)
    {
        auto& edge = m_Meshes.Edges.emplace_back();
        file >> edge.x >> edge.y >> edge.z >> edge.w;
    }

    return mesh;
}

Mesh Scene::CreateCylinderMesh(float radius, float height, uint32_t divr, uint32_t divh)
{
    Mesh mesh = {
        .PositionOffset = static_cast<uint32_t>(m_Meshes.Positions.size()),
        .VertexOffset = static_cast<uint32_t>(m_Meshes.VertexIndices.size()),
        .TriangleOffset = static_cast<uint32_t>(m_Meshes.Triangles.size()),
        .EdgeOffset = static_cast<uint32_t>(m_Meshes.Edges.size()),
    };

    auto createCircle = [&](float z) {
        const uint32_t vertexOffset = static_cast<uint32_t>(m_Meshes.VertexIndices.size()) - mesh.VertexOffset;
        for (uint32_t i = 0; i < divr; i++)
        {
            const float t = 2.0f * static_cast<float>(std::numbers::pi) * static_cast<float>(i) / static_cast<float>(divr);
            const float x = std::cos(t) * radius;
            const float y = std::sin(t) * radius;
            m_Meshes.Positions.emplace_back(x, y, z, 1.0f);
            m_Meshes.VertexNormals.emplace_back(glm::vec4(glm::normalize(glm::vec3(x, y, 0.0f)), 0.0f));
            m_Meshes.VertexIndices.push_back(vertexOffset + i);
        }
    };

    for (uint32_t j = 0; j <= divh; j++)
    {
        const uint32_t vertexOffset = static_cast<uint32_t>(m_Meshes.VertexIndices.size()) - mesh.VertexOffset;

        const float z = -height / 2.0f + height * static_cast<float>(j) / static_cast<float>(divh);
        createCircle(z);

        if (j != 0)
        {
            const uint32_t prevVertexOffset = static_cast<uint32_t>(vertexOffset - divr);
            for (uint32_t i = 0; i <= divr; i++)
            {
                m_Meshes.Triangles.emplace_back(prevVertexOffset + i, prevVertexOffset + (i + 1) % divr, vertexOffset + i);
                m_Meshes.Triangles.emplace_back(vertexOffset + i, vertexOffset + (i + 1) % divr, prevVertexOffset + (i + 1) % divr);
            }
        }
    }

    assert(m_Meshes.Positions.size() - mesh.PositionOffset == divr * divh + divr);
    assert(m_Meshes.VertexNormals.size() - mesh.VertexOffset == divr * divh + divr);

    auto createLid = [&](float sign, uint32_t vertexOffset) {
        createCircle(-height / 2.0f);

        m_Meshes.Positions.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
        m_Meshes.VertexNormals.emplace_back(0.0f, 0.0f, sign, 0.0f);
        m_Meshes.VertexIndices.push_back(vertexOffset);

        for (uint32_t i = 0; i <= divr; i++)
        {
            m_Meshes.Triangles.push_back(vertexOffset + glm::uvec3(0u, i, (i + 1) % divr));
        }
    };

    createLid(-1.0f, 0);
    createLid(1.0f, divr * divh);

    mesh.PositionCount = static_cast<uint32_t>(m_Meshes.Positions.size()) - mesh.PositionOffset;
    mesh.VertexCount = static_cast<uint32_t>(m_Meshes.VertexIndices.size()) - mesh.VertexOffset;
    mesh.TriangleCount = static_cast<uint32_t>(m_Meshes.Triangles.size()) - mesh.TriangleOffset;
    mesh.EdgeCount = static_cast<uint32_t>(m_Meshes.Edges.size()) - mesh.EdgeOffset;

    return mesh;
}

Mesh Scene::CreateUnitCubeMesh()
{
    Mesh mesh = {
        .PositionOffset = static_cast<uint32_t>(m_Meshes.Positions.size()),
        .VertexOffset = static_cast<uint32_t>(m_Meshes.VertexIndices.size()),
        .TriangleOffset = static_cast<uint32_t>(m_Meshes.Triangles.size()),
        .EdgeOffset = static_cast<uint32_t>(m_Meshes.Edges.size()),
    };

    m_Meshes.Positions.emplace_back(-1, -1, 1, 1);
    m_Meshes.Positions.emplace_back(1, -1, 1, 1);
    m_Meshes.Positions.emplace_back(1, 1, 1, 1);
    m_Meshes.Positions.emplace_back(-1, 1, 1, 1);
    m_Meshes.VertexNormals.emplace_back(0, 0, -1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, -1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, -1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, -1, 0);
    m_Meshes.VertexIndices.push_back(0);
    m_Meshes.VertexIndices.push_back(1);
    m_Meshes.VertexIndices.push_back(2);
    m_Meshes.VertexIndices.push_back(3);

    m_Meshes.Positions.emplace_back(1, -1, -1, 1);
    m_Meshes.Positions.emplace_back(-1, -1, -1, 1);
    m_Meshes.Positions.emplace_back(-1, 1, -1, 1);
    m_Meshes.Positions.emplace_back(1, 1, -1, 1);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexIndices.push_back(4);
    m_Meshes.VertexIndices.push_back(5);
    m_Meshes.VertexIndices.push_back(6);
    m_Meshes.VertexIndices.push_back(7);

    m_Meshes.Positions.emplace_back(-1, -1, -1, 1);
    m_Meshes.Positions.emplace_back(-1, -1, 1, 1);
    m_Meshes.Positions.emplace_back(-1, 1, 1, 1);
    m_Meshes.Positions.emplace_back(-1, 1, -1, 1);
    m_Meshes.VertexNormals.emplace_back(1, 0, 0, 0);
    m_Meshes.VertexNormals.emplace_back(1, 0, 0, 0);
    m_Meshes.VertexNormals.emplace_back(1, 0, 0, 0);
    m_Meshes.VertexNormals.emplace_back(1, 0, 0, 0);
    m_Meshes.VertexIndices.push_back(8);
    m_Meshes.VertexIndices.push_back(9);
    m_Meshes.VertexIndices.push_back(10);
    m_Meshes.VertexIndices.push_back(11);

    m_Meshes.Positions.emplace_back(1, -1, 1, 1);
    m_Meshes.Positions.emplace_back(1, -1, -1, 1);
    m_Meshes.Positions.emplace_back(1, 1, -1, 1);
    m_Meshes.Positions.emplace_back(1, 1, 1, 1);
    m_Meshes.VertexNormals.emplace_back(-1, 0, 0, 0);
    m_Meshes.VertexNormals.emplace_back(-1, 0, 0, 0);
    m_Meshes.VertexNormals.emplace_back(-1, 0, 0, 0);
    m_Meshes.VertexNormals.emplace_back(-1, 0, 0, 0);
    m_Meshes.VertexIndices.push_back(12);
    m_Meshes.VertexIndices.push_back(13);
    m_Meshes.VertexIndices.push_back(14);
    m_Meshes.VertexIndices.push_back(15);

    m_Meshes.Positions.emplace_back(-1, 1, 1, 1);
    m_Meshes.Positions.emplace_back(1, 1, 1, 1);
    m_Meshes.Positions.emplace_back(1, 1, -1, 1);
    m_Meshes.Positions.emplace_back(-1, 1, -1, 1);
    m_Meshes.VertexNormals.emplace_back(0, -1, 0, 0);
    m_Meshes.VertexNormals.emplace_back(0, -1, 0, 0);
    m_Meshes.VertexNormals.emplace_back(0, -1, 0, 0);
    m_Meshes.VertexNormals.emplace_back(0, -1, 0, 0);
    m_Meshes.VertexIndices.push_back(16);
    m_Meshes.VertexIndices.push_back(17);
    m_Meshes.VertexIndices.push_back(18);
    m_Meshes.VertexIndices.push_back(19);

    m_Meshes.Positions.emplace_back(-1, -1, -1, 1);
    m_Meshes.Positions.emplace_back(1, -1, -1, 1);
    m_Meshes.Positions.emplace_back(1, -1, 1, 1);
    m_Meshes.Positions.emplace_back(-1, -1, 1, 1);
    m_Meshes.VertexNormals.emplace_back(0, 1, 0, 0);
    m_Meshes.VertexNormals.emplace_back(0, 1, 0, 0);
    m_Meshes.VertexNormals.emplace_back(0, 1, 0, 0);
    m_Meshes.VertexNormals.emplace_back(0, 1, 0, 0);
    m_Meshes.VertexIndices.push_back(20);
    m_Meshes.VertexIndices.push_back(21);
    m_Meshes.VertexIndices.push_back(22);
    m_Meshes.VertexIndices.push_back(23);

    for (int i = 0; i < 6; i++)
    {
        m_Meshes.Triangles.push_back(i * 4u + glm::uvec3(0, 1, 2));
        m_Meshes.Triangles.push_back(i * 4u + glm::uvec3(2, 3, 0));
    }

    mesh.PositionCount = static_cast<uint32_t>(m_Meshes.Positions.size()) - mesh.PositionOffset;
    mesh.VertexCount = static_cast<uint32_t>(m_Meshes.VertexIndices.size()) - mesh.VertexOffset;
    mesh.TriangleCount = static_cast<uint32_t>(m_Meshes.Triangles.size()) - mesh.TriangleOffset;
    mesh.EdgeCount = static_cast<uint32_t>(m_Meshes.Edges.size()) - mesh.EdgeOffset;

    return mesh;
}

Mesh Scene::CreateUnitSquareMesh()
{
    Mesh mesh = {
        .PositionOffset = static_cast<uint32_t>(m_Meshes.Positions.size()),
        .VertexOffset = static_cast<uint32_t>(m_Meshes.VertexIndices.size()),
        .TriangleOffset = static_cast<uint32_t>(m_Meshes.Triangles.size()),
        .EdgeOffset = static_cast<uint32_t>(m_Meshes.Edges.size()),
    };

    m_Meshes.Positions.emplace_back(-1, -1, 0, 1);
    m_Meshes.Positions.emplace_back(1, -1, 0, 1);
    m_Meshes.Positions.emplace_back(1, 1, 0, 1);
    m_Meshes.Positions.emplace_back(-1, 1, 0, 1);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexIndices.push_back(0);
    m_Meshes.VertexIndices.push_back(1);
    m_Meshes.VertexIndices.push_back(2);
    m_Meshes.VertexIndices.push_back(3);        
    m_Meshes.Triangles.push_back(glm::uvec3(0, 1, 2));
    m_Meshes.Triangles.push_back(glm::uvec3(2, 3, 0));

    mesh.PositionCount = static_cast<uint32_t>(m_Meshes.Positions.size()) - mesh.PositionOffset;
    mesh.VertexCount = static_cast<uint32_t>(m_Meshes.VertexIndices.size()) - mesh.VertexOffset;
    mesh.TriangleCount = static_cast<uint32_t>(m_Meshes.Triangles.size()) - mesh.TriangleOffset;
    mesh.EdgeCount = static_cast<uint32_t>(m_Meshes.Edges.size()) - mesh.EdgeOffset;

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
