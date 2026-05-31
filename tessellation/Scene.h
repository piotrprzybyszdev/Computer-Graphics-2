#include <glm/glm.hpp>

#include <Core/UserInterface.h>

#include <filesystem>
#include <span>

struct Vertex
{
    glm::vec4 Position;
};

struct Patch
{
    uint32_t VertexOffset;
    uint32_t VertexCount;

    uint32_t IndexOffset;
    uint32_t IndexCount;
};

struct Texture
{
    uint32_t Width;
    uint32_t Height;
    std::vector<std::vector<std::byte>> Content;
    enum class PixelFormat { RGBA8, BGRA8 } Format;
};

struct TessellationControls
{
    float InsideTessFactor = 1.0f;
    float OutsideTessFactor = 1.0f;
    bool ShowControlLines = false;
    bool ShadePhong = false;
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

    const TessellationControls& GetTessellationControls() const;

    std::span<const Vertex> GetVertices() const;
    std::span<const uint32_t> GetIndices() const;

    std::span<const Patch> GetPatches() const;

    uint32_t GetCurrentPatchIndex() const;

    const Texture &GetDiffuseTexture() const;
    const Texture &GetHeightTexture() const;
    const Texture &GetNormalTexture() const;

private:
    struct Camera
    {
        bool IsLeftMouseButtonPressed = false, IsRightMouseButtonPressed = false;
        float Yaw = 0.0f, Pitch = 0.0f, Distance = 1.0f;
        glm::vec2 PrevMousePosition;
        glm::vec2 CurrentMousePosition;

        glm::mat4x4 View;
        glm::mat4x4 Projection;
        glm::vec4 Origin;
    } m_Camera;

    TessellationControls m_TessellationControls;

    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    std::vector<Patch> m_Patches;

    uint32_t m_CurrentPatchIndex = 0;

    Texture m_DiffuseTexture;
    Texture m_HeightTexture;
    Texture m_NormalTexture;

private:
    Patch CreatePatch0();
    Patch CreatePatch1();
    Patch CreatePatch2();

    Texture LoadTexture(const std::filesystem::path& path);
};