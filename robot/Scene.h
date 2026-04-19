#include <glm/glm.hpp>

#include <Core/UserInterface.h>

#include <filesystem>
#include <span>

struct StaticMesh
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

struct Light
{
    glm::vec4 Position;
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
    const glm::vec4& GetCameraOrigin() const;

    std::span<const glm::vec4> GetPositions() const;
    std::span<const uint32_t> GetVertexIndices() const;
    std::span<const glm::vec4> GetVertexNormals() const;
    std::span<const glm::uvec3> GetTriangles() const;
    std::span<const glm::uvec4> GetEdges() const;

    std::span<const StaticMesh> GetMeshes() const;
    std::span<const StaticMesh> GetRobotMeshes() const;
    std::span<const glm::mat4x4> GetTransforms() const;

    std::span<const Light> GetLights() const;

private:
    struct Camera
    {
        glm::mat4x4 View;
        glm::mat4x4 Projection;
        glm::vec4 Origin;
    } m_Camera;

    struct StaticMeshes
    {
        std::vector<glm::vec4> Positions;
        std::vector<uint32_t> VertexIndices;
        std::vector<glm::vec4> VertexNormals;
        std::vector<glm::uvec3> Triangles;
        std::vector<glm::uvec4> Edges;
        std::vector<glm::mat4x4> Transforms;

        std::vector<StaticMesh> Meshes;
    } m_StaticMeshes;

    std::vector<Light> m_Lights;

private:
    StaticMesh LoadRobotMesh(const std::filesystem::path& path);
    StaticMesh CreateCylinderMesh(float radius, float height, uint32_t divr, uint32_t divh);
    StaticMesh CreateUnitCubeMesh();
};