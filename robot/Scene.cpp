#include <cassert>
#include <format>
#include <fstream>
#include <numbers>
#include <vector>

#include "Scene.h"

Scene::Scene()
{
    for (int i = 0; i < 6; i++)
        m_Robot.Meshes.push_back(LoadRobotMesh(std::format("assets/puma/mesh{}.txt", i + 1)));
    m_Cylinder = CreateCylinderMesh(1.0f, 10.0f, 10, 3);
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

std::span<const RobotMesh> Scene::GetRobotMeshes() const
{
    return m_Robot.Meshes;
}

std::span<const CylinderVertex> Scene::GetCylinderVertices() const
{
    return m_Cylinder.Vertices;
}

std::span<const uint32_t> Scene::GetCylinderIndices() const
{
    return m_Cylinder.Indices;
}

RobotMesh Scene::LoadRobotMesh(const std::filesystem::path &path)
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
        auto &pos = m_Robot.Positions.emplace_back();
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

Scene::Cylinder Scene::CreateCylinderMesh(float radius, float height, uint32_t divr, uint32_t divh)
{
    Cylinder cylinder;

    auto createCircle = [&](float z) {
        for (uint32_t i = 0; i < divr; i++)
        {
            const float t = 2.0f * static_cast<float>(std::numbers::pi) * static_cast<float>(i) / static_cast<float>(divr);
            const float x = std::cos(t) * radius;
            const float y = std::sin(t) * radius;
            cylinder.Vertices.emplace_back(glm::vec4(x, y, z, 1.0f), glm::vec4(glm::normalize(glm::vec3(x, y, 0.0f)), 0.0f));
        }
    };

    for (uint32_t j = 0; j <= divh; j++)
    {
        const uint32_t vertexOffset = static_cast<uint32_t>(cylinder.Vertices.size());

        const float z = -height / 2.0f + height * static_cast<float>(j) / static_cast<float>(divh);
        createCircle(z);

        if (j != 0)
        {
            const uint32_t prevVertexOffset = static_cast<uint32_t>(vertexOffset - divr);
            for (uint32_t i = 0; i <= divr; i++)
            {
                cylinder.Indices.push_back(prevVertexOffset + i);
                cylinder.Indices.push_back(prevVertexOffset + (i + 1) % divr);
                cylinder.Indices.push_back(vertexOffset + i);
                cylinder.Indices.push_back(vertexOffset + i);
                cylinder.Indices.push_back(vertexOffset + (i + 1) % divr);
                cylinder.Indices.push_back(prevVertexOffset + (i + 1) % divr);
            }
        }
    }

    assert(cylinder.Vertices.size() == divr * divh + divr);

    auto createLid = [&](float sign, uint32_t vertexOffset) {
        createCircle(-height / 2.0f);
        
        cylinder.Vertices.emplace_back(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, sign, 0.0f));
        const uint32_t centerIndex = static_cast<uint32_t>(cylinder.Vertices.size() - 1);

        for (uint32_t i = 0; i <= divr; i++)
        {
            cylinder.Indices.push_back(centerIndex);
            cylinder.Indices.push_back(vertexOffset + i);
            cylinder.Indices.push_back(vertexOffset + (i + 1) % divr);
        }
    };

    createLid(-1.0f, 0);
    createLid(1.0f, divr * divh);

    return cylinder;
}
