#include <glm/glm.hpp>

#include <Core/UserInterface.h>

#include <filesystem>
#include <span>
#include <unordered_set>

struct Mesh
{
    uint32_t PositionOffset;
    uint32_t PositionCount;
    uint32_t VertexOffset;
    uint32_t VertexCount;
    uint32_t TriangleOffset;
    uint32_t TriangleCount;
    uint32_t EdgeOffset;
    uint32_t EdgeCount;
    glm::vec4 Color;
};

struct Texture
{
    uint32_t Width;
    uint32_t Height;
    std::vector<std::byte> Content;
};

struct Light
{
    glm::vec4 Position;
};

struct Particle
{
    glm::mat4x4 Transform;
    glm::vec3 Position;
    float Alpha;
    glm::vec3 Velocity;
    float Age;
};

class Scene
{
public:
    Scene();

    void OnResize(uint32_t width, uint32_t height);
    void OnUpdate(float timeStep);

    void OnKeyEvent(ref::Key key, ref::KeyAction action, ref::Mods mods);
    void OnMouseButtonEvent(ref::Button button, ref::ButtonAction action, ref::Mods mods);
    void OnCursorMoveEvent(double xpos, double ypos);

    const glm::mat4x4& GetCameraProjection() const;
    const glm::mat4x4& GetCameraView() const;
    const glm::vec4& GetCameraOrigin() const;

    std::span<const glm::vec4> GetPositions() const;
    std::span<const uint32_t> GetVertexIndices() const;
    std::span<const glm::vec4> GetVertexNormals() const;
    std::span<const glm::uvec3> GetTriangles() const;
    std::span<const glm::uvec4> GetEdges() const;

    std::span<const Mesh> GetMeshes() const;
    std::span<const Mesh> GetRobotMeshes() const;
    std::span<const Mesh> GetStaticMeshes() const;

    std::span<const glm::mat4x4> GetTransforms() const;

    const Mesh& GetMirrorMesh() const;
    uint32_t GetMirrorMeshIndex() const;
    glm::mat4x4 GetMirrorViewMatrix() const;
    glm::vec4 GetMirrorCameraOrigin() const;

    std::span<const Light> GetLights() const;

    const Texture& GetSparkTexture() const;
    const Texture& GetMirrorTexture() const;

    std::span<const Particle> GetParticles() const;

private:
    struct Camera
    {
        glm::mat4x4 View;
        glm::mat4x4 Projection;
        glm::vec4 Origin;
    } m_Camera;

    struct Meshes
    {
        std::vector<glm::vec4> Positions;
        std::vector<uint32_t> VertexIndices;
        std::vector<glm::vec4> VertexNormals;
        std::vector<glm::uvec3> Triangles;
        std::vector<glm::uvec4> Edges;
        std::vector<glm::mat4x4> Transforms;

        std::vector<Mesh> Meshes;
    } m_Meshes;

    std::vector<Light> m_Lights;

    Texture m_MirrorTexture;
    Texture m_SparkTexture;

    std::vector<Particle> m_Particles;

	std::unordered_set<ref::Key> m_PressedKeys;
    
    double m_LastMouseX = 0.0;
    double m_LastMouseY = 0.0;
    bool m_HasLastMouse = false;
    bool m_IsRightMouseDown = false;

    float m_Yaw = -90.0f;
    float m_Pitch = 0.0f;
    float m_MouseSensitivity = 0.12f;

	glm::vec3 m_CameraPosition;
    glm::vec3 m_CameraForward;

private:
    Mesh LoadRobotMesh(const std::filesystem::path& path);
    Mesh CreateCylinderMesh(float radius, float height, uint32_t divr, uint32_t divh);
    Mesh CreateUnitCubeMesh();
    Mesh CreateUnitSquareMesh();

    Texture LoadTexture(const std::filesystem::path& path);
};