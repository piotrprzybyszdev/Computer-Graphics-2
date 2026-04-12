#include <glm/gtc/matrix_transform.hpp>

#include <cassert>
#include <format>
#include <fstream>
#include <numbers>
#include <ranges>
#include <vector>

#include "Scene.h"

Scene::Scene()
{
    m_Robot.Transforms.resize(6);
    for (int i = 0; i < 6; i++)
        m_Robot.Meshes.push_back(LoadRobotMesh(std::format("assets/puma/mesh{}.txt", i + 1)));

    m_StaticMeshes.Meshes.push_back(CreateCylinderMesh(1.0f, 10.0f, 10, 3));
    
    glm::mat4x4 cylinderTransform = glm::scale(glm::rotate(glm::translate(glm::mat4x4(1.0f), glm::vec3(-1.5f, -1.0f, -3.0f)), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.4f, 0.4f, 0.5f));
    m_StaticMeshes.Transforms.push_back(cylinderTransform);

    m_StaticMeshes.Meshes.push_back(CreateUnitCubeMesh());
    m_StaticMeshes.Transforms.push_back(glm::scale(glm::mat4x4(1.0f), glm::vec3(3.0f, 2.0f, 3.0f)));
}

void Scene::OnResize(uint32_t width, uint32_t height)
{
    m_Camera.Projection = glm::perspectiveFov(45.0f, static_cast<float>(width), static_cast<float>(height), 0.1f, 1000.0f);
}

void Scene::OnUpdate(float /* timeStep */)
{
    // TODO: camera controls
    m_Camera.View = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));

    // TODO: inverse kinematics
    for (int i = 0; i < 6; i++)
        m_Robot.Transforms[i] = glm::mat4x4(1.0f);
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

std::span<const glm::vec4> Scene::GetRobotPositions() const
{
    return m_Robot.Positions;
}

std::span<const uint32_t> Scene::GetRobotVertexIndices() const
{
    return m_Robot.VertexIndices;
}

std::span<const glm::vec4> Scene::GetRobotVertexNormals() const
{
    return m_Robot.VertexNormals;
}

std::span<const glm::uvec3> Scene::GetRobotTriangles() const
{
    return m_Robot.Triangles;
}

std::span<const glm::uvec4> Scene::GetRobotEdges() const
{
    return m_Robot.Edges;
}

std::span<const glm::mat4x4> Scene::GetRobotTransforms() const
{
    return m_Robot.Transforms;
}

std::span<const RobotMesh> Scene::GetRobotMeshes() const
{
    return m_Robot.Meshes;
}

std::span<const StaticVertex> Scene::GetStaticVertices() const
{
    return m_StaticMeshes.Vertices;
}

std::span<const uint32_t> Scene::GetStaticIndices() const
{
    return m_StaticMeshes.Indices;
}

std::span<const StaticMesh> Scene::GetStaticMeshes() const
{
    return m_StaticMeshes.Meshes;
}

std::span<const glm::mat4x4> Scene::GetStaticTransforms() const
{
    return m_StaticMeshes.Transforms;
}

RobotMesh Scene::LoadRobotMesh(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::in);
    assert(file.is_open());

    RobotMesh mesh = {
        .PositionOffset = static_cast<uint32_t>(m_Robot.Positions.size()),
        .VertexOffset = static_cast<uint32_t>(m_Robot.VertexIndices.size()),
        .TriangleOffset = static_cast<uint32_t>(m_Robot.Triangles.size()),
        .EdgeOffset = static_cast<uint32_t>(m_Robot.Edges.size()),
    };

    file >> mesh.PositionCount;
    for (uint32_t i = 0; i < mesh.PositionCount; i++)
    {
        auto& pos = m_Robot.Positions.emplace_back();
        file >> pos.x >> pos.y >> pos.z;
    }

    file >> mesh.VertexCount;
    for (uint32_t i = 0; i < mesh.VertexCount; i++)
    {
        auto& idx = m_Robot.VertexIndices.emplace_back();
        auto& normal = m_Robot.VertexNormals.emplace_back();
        file >> idx >> normal.x >> normal.y >> normal.z;
    }

    file >> mesh.TriangleCount;
    for (uint32_t i = 0; i < mesh.TriangleCount; i++)
    {
        auto& triangle = m_Robot.Triangles.emplace_back();
        file >> triangle.x >> triangle.y >> triangle.z;
    }

    file >> mesh.EdgeCount;
    assert(mesh.TriangleCount % 2 == 0 && mesh.EdgeCount == mesh.TriangleCount * 3 / 2);
    for (uint32_t i = 0; i < mesh.EdgeCount; i++)
    {
        auto& edge = m_Robot.Edges.emplace_back();
        file >> edge.x >> edge.y >> edge.z >> edge.w;
    }

    return mesh;
}

StaticMesh Scene::CreateCylinderMesh(float radius, float height, uint32_t divr, uint32_t divh)
{
    StaticMesh mesh = {
        .VertexOffset = static_cast<uint32_t>(m_StaticMeshes.Vertices.size()),
        .IndexOffset = static_cast<uint32_t>(m_StaticMeshes.Indices.size()),
    };

    auto createCircle = [&](float z) {
        for (uint32_t i = 0; i < divr; i++)
        {
            const float t = 2.0f * static_cast<float>(std::numbers::pi) * static_cast<float>(i) / static_cast<float>(divr);
            const float x = std::cos(t) * radius;
            const float y = std::sin(t) * radius;
            m_StaticMeshes.Vertices.emplace_back(glm::vec4(x, y, z, 1.0f), glm::vec4(glm::normalize(glm::vec3(x, y, 0.0f)), 0.0f));
        }
        };

    for (uint32_t j = 0; j <= divh; j++)
    {
        const uint32_t vertexOffset = static_cast<uint32_t>(m_StaticMeshes.Vertices.size());

        const float z = -height / 2.0f + height * static_cast<float>(j) / static_cast<float>(divh);
        createCircle(z);

        if (j != 0)
        {
            const uint32_t prevVertexOffset = static_cast<uint32_t>(vertexOffset - divr);
            for (uint32_t i = 0; i <= divr; i++)
            {
                m_StaticMeshes.Indices.push_back(prevVertexOffset + i);
                m_StaticMeshes.Indices.push_back(prevVertexOffset + (i + 1) % divr);
                m_StaticMeshes.Indices.push_back(vertexOffset + i);
                m_StaticMeshes.Indices.push_back(vertexOffset + i);
                m_StaticMeshes.Indices.push_back(vertexOffset + (i + 1) % divr);
                m_StaticMeshes.Indices.push_back(prevVertexOffset + (i + 1) % divr);
            }
        }
    }

    assert(m_StaticMeshes.Vertices.size() == divr * divh + divr);

    auto createLid = [&](float sign, uint32_t vertexOffset) {
        createCircle(-height / 2.0f);

        m_StaticMeshes.Vertices.emplace_back(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, sign, 0.0f));
        const uint32_t centerIndex = static_cast<uint32_t>(m_StaticMeshes.Vertices.size() - 1);

        for (uint32_t i = 0; i <= divr; i++)
        {
            m_StaticMeshes.Indices.push_back(centerIndex);
            m_StaticMeshes.Indices.push_back(vertexOffset + i);
            m_StaticMeshes.Indices.push_back(vertexOffset + (i + 1) % divr);
        }
    };

    createLid(-1.0f, 0);
    createLid(1.0f, divr * divh);

    mesh.IndexCount = static_cast<uint32_t>(m_StaticMeshes.Indices.size()) - mesh.IndexOffset;
    mesh.VertexCount = static_cast<uint32_t>(m_StaticMeshes.Vertices.size()) - mesh.VertexOffset;

    return mesh;
}

StaticMesh Scene::CreateUnitCubeMesh()
{
    StaticMesh mesh = {
        .VertexOffset = static_cast<uint32_t>(m_StaticMeshes.Vertices.size()),
        .IndexOffset = static_cast<uint32_t>(m_StaticMeshes.Indices.size()),
    };

    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ -1, -1, 1, 1 }, glm::vec4{ 0, 0, -1, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ 1, -1, 1, 1 }, glm::vec4{ 0, 0, -1, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ 1, 1, 1, 1 }, glm::vec4{ 0, 0, -1, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ -1, 1, 1, 1 }, glm::vec4{ 0, 0, -1, 0 });

    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ 1, -1, -1, 1 }, glm::vec4{ 0, 0, 1, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ -1, -1, -1, 1 }, glm::vec4{ 0, 0, 1, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ -1, 1, -1, 1 }, glm::vec4{ 0, 0, 1, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ 1, 1, -1, 1 }, glm::vec4{ 0, 0, 1, 0 });

    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ -1, -1, -1, 1 }, glm::vec4{ 1, 0, 0, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ -1, -1, 1, 1 }, glm::vec4{ 1, 0, 0, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ -1, 1, 1, 1 }, glm::vec4{ 1, 0, 0, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ -1, 1, -1, 1 }, glm::vec4{ 1, 0, 0, 0 });

    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ 1, -1, 1, 1 }, glm::vec4{ -1, 0, 0, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ 1, -1, -1, 1 }, glm::vec4{ -1, 0, 0, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ 1, 1, -1, 1 }, glm::vec4{ -1, 0, 0, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ 1, 1, 1, 1 }, glm::vec4{ -1, 0, 0, 0 });

    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ -1, 1, 1, 1 }, glm::vec4{ 0, -1, 0, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ 1, 1, 1, 1 }, glm::vec4{ 0, -1, 0, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ 1, 1, -1, 1 }, glm::vec4{ 0, -1, 0, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ -1, 1, -1, 1 }, glm::vec4{ 0, -1, 0, 0 });

    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ -1, -1, -1, 1 }, glm::vec4{ 0, 1, 0, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ 1, -1, -1, 1 }, glm::vec4{ 0, 1, 0, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ 1, -1, 1, 1 }, glm::vec4{ 0, 1, 0, 0 });
    m_StaticMeshes.Vertices.emplace_back(glm::vec4{ -1, -1, 1, 1 }, glm::vec4{ 0, 1, 0, 0 });

    for (int i = 0; i < 6; i++)
    {
        std::array<uint32_t, 6> indices = { 0, 1, 2, 2, 3, 0 };
        for (auto idx : indices)
            m_StaticMeshes.Indices.push_back(i * 4 + idx);
    }

    mesh.IndexCount = static_cast<uint32_t>(m_StaticMeshes.Indices.size()) - mesh.IndexOffset;
    mesh.VertexCount = static_cast<uint32_t>(m_StaticMeshes.Vertices.size()) - mesh.VertexOffset;

    return mesh;
}
