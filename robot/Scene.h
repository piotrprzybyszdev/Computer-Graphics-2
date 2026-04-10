#include <glm/glm.hpp>

#include <filesystem>
#include <span>

struct RobotMesh
{
    uint32_t PositionOffset;
    uint32_t PositionCount;
    uint32_t VertexOffset;
    uint32_t VertexCount;
    uint32_t TriangleOffset;
    uint32_t TriangleCount;
    uint32_t EdgeOffset;
    uint32_t EdgeCount;
};

class Scene
{
public:
    Scene();

    std::span<const glm::vec4> GetRobotPositions() const;
    std::span<const uint32_t> GetRobotVertexIndices() const;
    std::span<const glm::vec4> GetRobotVertexNormals() const;
    std::span<const glm::uvec3> GetRobotTriangles() const;
    std::span<const glm::uvec4> GetRobotEdges() const;

    std::span<const RobotMesh> GetRobotMeshes() const;

private:
    struct Robot
    {
        std::vector<glm::vec4> Positions;
        std::vector<uint32_t> VertexIndices;
        std::vector<glm::vec4> VertexNormals;
        std::vector<glm::uvec3> Triangles;
        std::vector<glm::uvec4> Edges;

        std::vector<RobotMesh> Meshes;
    } m_Robot;

private:
    RobotMesh LoadRobotMesh(const std::filesystem::path& path);
};