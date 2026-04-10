#include <cassert>
#include <format>
#include <fstream>
#include <vector>

#include "Scene.h"

Scene::Scene()
{
    for (int i = 0; i < 6; i++)
        m_Robot.Meshes.push_back(LoadRobotMesh(std::format("assets/puma/mesh{}.txt", i + 1)));
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