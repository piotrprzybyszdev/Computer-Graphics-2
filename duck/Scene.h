#include <glm/glm.hpp>

#include <Core/UserInterface.h>

#include <filesystem>
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
    
private:
    Mesh CreateUnitSquareMesh();
    Texture LoadTexture(const std::filesystem::path& path);
};