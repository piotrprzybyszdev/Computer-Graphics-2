#include <glm/glm.hpp>

#include <Core/UserInterface.h>

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

struct StaticVertex
{
    glm::vec4 Position;
    glm::vec4 Normal;
};

struct StaticMesh
{
    uint32_t VertexOffset;
    uint32_t IndexOffset;
    uint32_t VertexCount;
    uint32_t IndexCount;
};

class Scene
{
public:
    Scene();

    void OnResize(uint32_t width, uint32_t height);
    void OnUpdate(float timeStep);

    void OnKeyRelease(ref::Key key);

    const glm::mat4x4& GetCameraProjection() const;
    const glm::mat4x4& GetCameraView() const;

    std::span<const glm::vec4> GetRobotPositions() const;
    std::span<const uint32_t> GetRobotVertexIndices() const;
    std::span<const glm::vec4> GetRobotVertexNormals() const;
    std::span<const glm::uvec3> GetRobotTriangles() const;
    std::span<const glm::uvec4> GetRobotEdges() const;

    std::span<const glm::mat4x4> GetRobotTransforms() const;
    std::span<const RobotMesh> GetRobotMeshes() const;

    std::span<const StaticVertex> GetStaticVertices() const;
    std::span<const uint32_t> GetStaticIndices() const;

    std::span<const StaticMesh> GetStaticMeshes() const;
    std::span<const glm::mat4x4> GetStaticTransforms() const;

private:
    struct Camera
    {
        glm::mat4x4 View;
        glm::mat4x4 Projection;
    } m_Camera;

    struct Robot
    {
        std::vector<glm::vec4> Positions;
        std::vector<uint32_t> VertexIndices;
        std::vector<glm::vec4> VertexNormals;
        std::vector<glm::uvec3> Triangles;
        std::vector<glm::uvec4> Edges;
        std::vector<glm::mat4x4> Transforms;

        std::vector<RobotMesh> Meshes;
    } m_Robot;

    struct StaticMeshes
    {
        std::vector<StaticVertex> Vertices;
        std::vector<uint32_t> Indices;
        std::vector<glm::mat4x4> Transforms;

        std::vector<StaticMesh> Meshes;
    } m_StaticMeshes;

private:
    RobotMesh LoadRobotMesh(const std::filesystem::path& path);
    StaticMesh CreateCylinderMesh(float radius, float height, uint32_t divr, uint32_t divh);
    StaticMesh CreateUnitCubeMesh();
};