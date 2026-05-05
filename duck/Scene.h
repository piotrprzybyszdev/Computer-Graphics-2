#include <glm/glm.hpp>

#include <Core/UserInterface.h>

#include <array>
#include <filesystem>
#include <optional>
#include <random>
#include <span>

struct Vertex
{
    glm::vec4 Position;
    glm::vec4 Normal;
    glm::vec2 TexCoord;
};

struct Mesh
{
    uint32_t VertexOffset;
    uint32_t VertexCount;
    uint32_t IndexOffset;
    uint32_t IndexCount;
};

struct Instance
{
    uint32_t MeshIndex;
    uint32_t TransformIndex;
};

struct Texture
{
    uint32_t Width;
    uint32_t Height;
    std::vector<std::byte> Content;
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

    std::span<const Vertex> GetVertices() const;
    std::span<const uint32_t> GetIndices() const;

    std::span<const glm::mat4x4> GetTransforms() const;
    std::span<const Mesh> GetMeshes() const;

    std::span<const Instance> GetInstances() const;

    uint32_t GetWaterInstanceIndex() const;
    uint32_t GetDuckInstanceIndex() const;
    uint32_t GetEnvironmentInstanceIndex() const;

    const Texture& GetDuckTexture() const;
    std::span<const Texture, 6> GetEnvironmentTextures() const;

    std::optional<glm::vec2> GetDisturbance() const;
    glm::vec2 GetDuckDisturbance() const;

private:
    struct Camera
    {
        glm::mat4x4 View;
        glm::mat4x4 Projection;
        glm::vec4 Origin;
    } m_Camera;

    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    std::vector<Mesh> m_Meshes;
    std::vector<glm::mat4x4> m_Transforms;

    std::vector<Instance> m_Instances;

    Texture m_DuckTexture;

    struct Curve
    {
        inline static float s_MaxTime = 1.0f;

        float ElapsedTime = 0.0f;
        std::array<glm::vec2, 4> Points = {};
    } m_Curve;
    
    std::mt19937 m_Rng;

    std::optional<glm::vec2> m_Disturbance;
    glm::vec2 m_DuckDisturbance;

    std::array<Texture, 6> m_EnvironmentTextures;

private:
    Mesh CreateUnitSquareMesh();
    Mesh CreateUnitCubeMesh();
    Mesh LoadMesh(const std::filesystem::path &path);

    Texture LoadTexture(const std::filesystem::path& path);
};