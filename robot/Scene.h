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

struct CylinderVertex
{
    glm::vec4 Position;
    glm::vec4 Normal;
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

    std::span<const CylinderVertex> GetCylinderVertices() const;
    std::span<const uint32_t> GetCylinderIndices() const;

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

    struct Cylinder
    {
        std::vector<CylinderVertex> Vertices;
        std::vector<uint32_t> Indices;
    } m_Cylinder;

private:
    RobotMesh LoadRobotMesh(const std::filesystem::path& path);
    Cylinder CreateCylinderMesh(float radius, float height, uint32_t divr, uint32_t divh);
};