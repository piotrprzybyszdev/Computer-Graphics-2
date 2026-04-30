#define GLM_FORCE_LEFT_HANDED
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cassert>
#include <format>
#include <fstream>
#include <numbers>
#include <ranges>
#include <random>
#include <vector>

#include "Scene.h"

Scene::Scene()
{
    for (int i = 0; i < 6; i++)
        m_Meshes.Meshes.push_back(LoadRobotMesh(std::format("assets/puma/mesh{}.txt", i + 1)));
    m_Meshes.Transforms.resize(6, glm::mat4x4(1.0f));

    m_Meshes.Meshes.push_back(CreateCylinderMesh(1.0f, 10.0f, 10, 3));
    
    m_Meshes.Transforms.push_back(
        glm::scale(glm::rotate(glm::translate(glm::mat4x4(1.0f), glm::vec3(-0.5f, -1.0f, -2.0f)), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.4f, 0.4f, 0.5f))
    );

    m_Meshes.Meshes.push_back(CreateUnitCubeMesh());
    m_Meshes.Transforms.push_back(glm::scale(glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(4.0f, 2.0f, 3.0f)));

    m_Meshes.Meshes.push_back(CreateUnitSquareMesh());
    m_Meshes.Transforms.push_back(glm::scale(glm::rotate(glm::rotate(glm::translate(glm::mat4x4(1.0f), glm::vec3(-1.8f, 0.3f, -0.25f)), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(0.4f)));

    m_Lights.push_back(Light(glm::vec4(-1.5f, 1.0f, 1.0f, 1.0f)));

    m_SparkTexture = LoadTexture("assets/spark.png");
    m_MirrorTexture = LoadTexture("assets/mirror.png");

    const size_t particleCount = 500;

    m_Particles.resize(particleCount, Particle {
        .Position = glm::vec3(0.0f, 0.0f, 0.0f),
        .Alpha = 0.0f,
        .Velocity = glm::vec3(0.0f, 0.0f, 0.0f),
        .Age = 0.0f,
	});

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    for (size_t i = 0; i < particleCount; i++)
    {
        m_Particles[i].Age = dis(gen);
    }   

    m_CameraPosition = glm::vec3(0.0f, 0.0f, 2.0f);
    m_CameraForward = glm::vec3(0.0f, 0.0f, -1.0f);
}

void Scene::OnResize(uint32_t width, uint32_t height)
{
    m_Camera.Projection = glm::perspectiveFov(45.0f, static_cast<float>(width), static_cast<float>(height), 0.1f, 1000.0f);
}

static void InverseKinematics(glm::vec3 pos, glm::vec3 normal, float& a1, float& a2, float& a3, float& a4, float& a5)
{
    float l1 = .91f, l2 = .81f, l3 = .33f, dy = .27f, dz = .26f;
    normal = glm::normalize(normal);
    glm::vec3 pos1 = pos + normal * l3;
    float e = sqrtf(pos1.z * pos1.z + pos1.x * pos1.x - dz * dz);
    a1 = atan2(pos1.z, -pos1.x) + atan2(dz, e);
    glm::vec3 pos2(e, pos1.y - dy, .0f);
    a3 = -acosf(glm::min(1.0f, (pos2.x * pos2.x + pos2.y * pos2.y - l1 * l1 - l2 * l2) / (2.0f * l1 * l2)));
    float k = l1 + l2 * cosf(a3), l = l2 * sinf(a3);
    a2 = -atan2(pos2.y, sqrtf(pos2.x * pos2.x + pos2.z * pos2.z)) - atan2(l, k);
    glm::vec3 normal1;
    normal1 = glm::vec3(glm::rotate(glm::mat4x4(1.0f), -a1, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(normal.x, normal.y, normal.z, .0f));
    normal1 = glm::vec3(glm::rotate(glm::mat4x4(1.0f), -(a2 + a3), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(normal1.x, normal1.y, normal1.z, .0f));
    a5 = acosf(normal1.x);
    a4 = atan2(normal1.z, normal1.y);
}

void Scene::OnUpdate(float timeStep)
{
    // camera
    {
        const float speed = 0.002f;

        const glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f);
        glm::vec3 cameraRight = glm::normalize(glm::cross(m_CameraForward, glm::vec3(0.0f, 1.0f, 0.0f)));

        if (m_PressedKeys.contains(ref::Key::W)) m_CameraPosition += timeStep * speed * m_CameraForward;
        if (m_PressedKeys.contains(ref::Key::S)) m_CameraPosition -= timeStep * speed * m_CameraForward;
        if (m_PressedKeys.contains(ref::Key::A)) m_CameraPosition -= timeStep * speed * cameraRight;
        if (m_PressedKeys.contains(ref::Key::D)) m_CameraPosition += timeStep * speed * cameraRight;
        if (m_PressedKeys.contains(ref::Key::Q)) m_CameraPosition += timeStep * speed * up;
        if (m_PressedKeys.contains(ref::Key::E)) m_CameraPosition -= timeStep * speed * up;

        m_Camera.Origin = glm::vec4(m_CameraPosition, 1.0f);
        m_Camera.View = glm::lookAt(
            m_CameraPosition,
            m_CameraPosition + m_CameraForward,
            up
        );
    }

    // inverse kinematics
    if (m_AnimationEnabled)
    {
        const glm::mat4x4 &transform = m_Meshes.Transforms[GetMirrorMeshIndex()];

        const glm::vec3 xaxis = transform * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        const glm::vec3 yaxis = transform * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        const glm::vec3 zaxis = transform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);

        const glm::vec3 center = transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        static float time = 0.0f;
        const float radius = 0.2f;
        time += timeStep;
        const glm::vec3 point = center + radius * normalize(xaxis) * glm::cos(time / 1000.0f) + radius * normalize(yaxis) * glm::sin(time / 1000.0f);

        std::array<float, 5> angles;
        InverseKinematics(point, zaxis, angles[0], angles[1], angles[2], angles[3], angles[4]);

        m_Meshes.Transforms[0] = glm::mat4x4(1.0f);
        m_Meshes.Transforms[1] = m_Meshes.Transforms[0] * glm::rotate(glm::mat4x4(1.0f), angles[0], glm::vec3(0.0f, 1.0f, 0.0f));
        m_Meshes.Transforms[2] = m_Meshes.Transforms[1] * glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.27f, 0.0f)) * glm::rotate(glm::mat4x4(1.0f), angles[1], glm::vec3(0.0f, 0.0f, 1.0f)) * glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, -0.27f, 0.0f));
        m_Meshes.Transforms[3] = m_Meshes.Transforms[2] * glm::translate(glm::mat4x4(1.0f), glm::vec3(-0.91f, 0.27f, 0.0f)) * glm::rotate(glm::mat4x4(1.0f), angles[2], glm::vec3(0.0f, 0.0f, 1.0f)) * glm::translate(glm::mat4x4(1.0f), glm::vec3(0.91f, -0.27f, 0.0f));
        m_Meshes.Transforms[4] = m_Meshes.Transforms[3] * glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.27f, -0.26f)) * glm::rotate(glm::mat4x4(1.0f), angles[3], glm::vec3(1.0f, 0.0f, 0.0f)) * glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, -0.27f, 0.26f));
        m_Meshes.Transforms[5] = m_Meshes.Transforms[4] * glm::translate(glm::mat4x4(1.0f), glm::vec3(-1.72f, 0.27f, 0.0f)) * glm::rotate(glm::mat4x4(1.0f), angles[4], glm::vec3(0.0f, 0.0f, 1.0f)) * glm::translate(glm::mat4x4(1.0f), glm::vec3(1.72f, -0.27f, 0.0f));

        auto uniformSampleHemisphere = [](const glm::vec2 &u) -> glm::vec4
            {
                const float z = u.x;
                const float r = glm::sqrt(glm::max(0.0f, 1.0f - z * z));
                const float phi = 2.0f * glm::pi<float>() * u.y;

                return glm::vec4(
                    r * glm::cos(phi),
                    r * glm::sin(phi),
                    z,
                    1.0f
                );
            };

        // Setup uniform random number generator
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(0.0f, 1.0f);

        // TODO: particle simulation
        for (int i = 0; i < m_Particles.size(); i++)
        {
            if (m_Particles[i].Age > 1.0f)
            {
                float rand1 = dis(gen);
                float rand2 = dis(gen);

                Particle particle {
                    .PrevPosition = point,
                    .Position = point,
                    .Alpha = 0.5f,
                    .Velocity = glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f)), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * uniformSampleHemisphere(glm::vec2(rand1, rand2)),
                    .Age = 0.0f,
                };
                m_Particles[i] = particle;
            }
            else
            {
                m_Particles[i].Velocity += glm::vec3(0.0f, -1.0f, 0.0f) * timeStep / 1000.0f;
                m_Particles[i].PrevPosition = m_Particles[i].Position;
                m_Particles[i].Position += m_Particles[i].Velocity * timeStep / 1000.0f;
                m_Particles[i].Alpha = glm::max(0.0f, m_Particles[i].Alpha - timeStep * 0.5f / 1000.0f);
                m_Particles[i].Age += timeStep / 1000.0f;
            }
        }
    }
}

void Scene::OnKeyEvent(ref::Key key, ref::KeyAction action, ref::Mods /* mods */)
{
    if (action == ref::KeyAction::Press || action == ref::KeyAction::Repeat)
    {
        m_PressedKeys.insert(key);
        if (key == ref::Key::Space && action == ref::KeyAction::Press)
            m_AnimationEnabled = !m_AnimationEnabled;
    }
    else if (action == ref::KeyAction::Release)
        m_PressedKeys.erase(key);
}

void Scene::OnMouseButtonEvent(ref::Button button, ref::ButtonAction action, ref::Mods /* mods */)
{
    if (button == ref::Button::Right)
    {
        m_IsRightMouseDown = action == ref::ButtonAction::Press;
        m_HasLastMouse = false;
    }
}

void Scene::OnCursorMoveEvent(double xpos, double ypos)
{
    if (!m_IsRightMouseDown)
        return;

    if (!m_HasLastMouse)
    {
        m_LastMouseX = xpos;
        m_LastMouseY = ypos;
        m_HasLastMouse = true;
        return;
    }

    const float dx = static_cast<float>(xpos - m_LastMouseX);
    const float dy = static_cast<float>(ypos - m_LastMouseY);

    m_LastMouseX = xpos;
    m_LastMouseY = ypos;

    m_Yaw += dx * m_MouseSensitivity;
    m_Pitch -= dy * m_MouseSensitivity;
    m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

    const float yawRad = glm::radians(m_Yaw);
    const float pitchRad = glm::radians(m_Pitch);

    m_CameraForward = glm::vec3(
        std::cos(yawRad) * std::cos(pitchRad),
        std::sin(pitchRad),
        std::sin(yawRad) * std::cos(pitchRad)
    );
}

const glm::mat4x4& Scene::GetCameraProjection() const
{
    return m_Camera.Projection;
}

const glm::mat4x4& Scene::GetCameraView() const
{
    return m_Camera.View;
}

const glm::vec4& Scene::GetCameraOrigin() const
{
    return m_Camera.Origin;
}

std::span<const glm::vec4> Scene::GetPositions() const
{
    return m_Meshes.Positions;
}

std::span<const uint32_t> Scene::GetVertexIndices() const
{
    return m_Meshes.VertexIndices;
}

std::span<const glm::vec4> Scene::GetVertexNormals() const
{
    return m_Meshes.VertexNormals;
}

std::span<const glm::uvec3> Scene::GetTriangles() const
{
    return m_Meshes.Triangles;
}

std::span<const glm::uvec4> Scene::GetEdges() const
{
    return m_Meshes.Edges;
}

std::span<const glm::mat4x4> Scene::GetTransforms() const
{
    return m_Meshes.Transforms;
}

std::span<const Mesh> Scene::GetMeshes() const
{
    return m_Meshes.Meshes;
}

std::span<const Mesh> Scene::GetRobotMeshes() const
{
    return std::span(m_Meshes.Meshes.data(), 6);
}

std::span<const Mesh> Scene::GetStaticMeshes() const
{
    return std::span(m_Meshes.Meshes.data(), 8);
}

const Mesh& Scene::GetMirrorMesh() const
{
    return m_Meshes.Meshes[GetMirrorMeshIndex()];
}

uint32_t Scene::GetMirrorMeshIndex() const
{
    return 8;
}

glm::mat4x4 Scene::GetMirrorViewMatrix() const
{
    // TODO: mirror camera view matrix
    const glm::mat4x4 &transform = m_Meshes.Transforms[GetMirrorMeshIndex()];
    return m_Camera.View * transform * glm::scale(glm::mat4x4(1.0f), glm::vec3(1.0f, 1.0f, -1.0f)) * glm::inverse(transform);
}

glm::vec4 Scene::GetMirrorCameraOrigin() const
{
    // TODO: mirror camera origin
    const glm::mat4x4& transform = m_Meshes.Transforms[GetMirrorMeshIndex()];
    return transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

std::span<const Light> Scene::GetLights() const
{
    return m_Lights;
}

const Texture& Scene::GetSparkTexture() const
{
    return m_SparkTexture;
}

const Texture& Scene::GetMirrorTexture() const
{
    return m_MirrorTexture;
}

std::span<const Particle> Scene::GetParticles() const
{
    return m_Particles;
}

Mesh Scene::LoadRobotMesh(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::in);
    assert(file.is_open());

    Mesh mesh = {
        .PositionOffset = static_cast<uint32_t>(m_Meshes.Positions.size()),
        .VertexOffset = static_cast<uint32_t>(m_Meshes.VertexIndices.size()),
        .TriangleOffset = static_cast<uint32_t>(m_Meshes.Triangles.size()),
        .EdgeOffset = static_cast<uint32_t>(m_Meshes.Edges.size()),
        .Color = glm::vec4(0.0, 1.0f, 0.0f, 1.0f),
    };

    file >> mesh.PositionCount;
    for (uint32_t i = 0; i < mesh.PositionCount; i++)
    {
        auto& pos = m_Meshes.Positions.emplace_back();
        file >> pos.x >> pos.y >> pos.z;
    }

    file >> mesh.VertexCount;
    for (uint32_t i = 0; i < mesh.VertexCount; i++)
    {
        auto& idx = m_Meshes.VertexIndices.emplace_back();
        auto& normal = m_Meshes.VertexNormals.emplace_back();
        file >> idx >> normal.x >> normal.y >> normal.z;
    }

    file >> mesh.TriangleCount;
    for (uint32_t i = 0; i < mesh.TriangleCount; i++)
    {
        auto& triangle = m_Meshes.Triangles.emplace_back();
        file >> triangle.x >> triangle.y >> triangle.z;
    }

    file >> mesh.EdgeCount;
    assert(mesh.TriangleCount % 2 == 0 && mesh.EdgeCount == mesh.TriangleCount * 3 / 2);
    for (uint32_t i = 0; i < mesh.EdgeCount; i++)
    {
        auto& edge = m_Meshes.Edges.emplace_back();
        file >> edge.x >> edge.y >> edge.z >> edge.w;
    }

    return mesh;
}

Mesh Scene::CreateCylinderMesh(float radius, float height, uint32_t divr, uint32_t divh)
{
    Mesh mesh = {
        .PositionOffset = static_cast<uint32_t>(m_Meshes.Positions.size()),
        .VertexOffset = static_cast<uint32_t>(m_Meshes.VertexIndices.size()),
        .TriangleOffset = static_cast<uint32_t>(m_Meshes.Triangles.size()),
        .EdgeOffset = static_cast<uint32_t>(m_Meshes.Edges.size()),
        .Color = glm::vec4(0.0, 1.0f, 0.0f, 1.0f),
    };

    auto createCircle = [&](float z) {
        const uint32_t vertexOffset = static_cast<uint32_t>(m_Meshes.VertexIndices.size()) - mesh.VertexOffset;
        for (uint32_t i = 0; i < divr; i++)
        {
            const float t = 2.0f * static_cast<float>(std::numbers::pi) * static_cast<float>(i) / static_cast<float>(divr);
            const float x = std::cos(t) * radius;
            const float y = std::sin(t) * radius;
            m_Meshes.Positions.emplace_back(x, y, z, 1.0f);
            m_Meshes.VertexNormals.emplace_back(glm::vec4(glm::normalize(glm::vec3(x, y, 0.0f)), 0.0f));
            m_Meshes.VertexIndices.push_back(vertexOffset + i);
        }
    };

    for (uint32_t j = 0; j <= divh; j++)
    {
        const uint32_t vertexOffset = static_cast<uint32_t>(m_Meshes.VertexIndices.size()) - mesh.VertexOffset;

        const float z = -height / 2.0f + height * static_cast<float>(j) / static_cast<float>(divh);
        createCircle(z);

        if (j != 0)
        {
            const uint32_t prevVertexOffset = static_cast<uint32_t>(vertexOffset - divr);
            for (uint32_t i = 0; i <= divr; i++)
            {
                m_Meshes.Triangles.emplace_back(prevVertexOffset + i, prevVertexOffset + (i + 1) % divr, vertexOffset + i);
                m_Meshes.Triangles.emplace_back(vertexOffset + i, prevVertexOffset + (i + 1) % divr, vertexOffset + (i + 1) % divr);
            }
        }
    }

    assert(m_Meshes.Positions.size() - mesh.PositionOffset == divr * divh + divr);
    assert(m_Meshes.VertexNormals.size() - mesh.VertexOffset == divr * divh + divr);

    auto createLid = [&](float sign, uint32_t vertexOffset, bool back) {
        createCircle(-height / 2.0f);

        m_Meshes.Positions.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
        m_Meshes.VertexNormals.emplace_back(0.0f, 0.0f, sign, 0.0f);
        m_Meshes.VertexIndices.push_back(vertexOffset);

        for (uint32_t i = 0; i <= divr; i++)
        {
            glm::uvec3 triangle = back ? glm::uvec3(0u, i, (i + 1) % divr) : glm::uvec3(0u, (i + 1) % divr, i);
            m_Meshes.Triangles.push_back(vertexOffset + triangle);
        }
    };

    createLid(-1.0f, 0, false);
    createLid(1.0f, divr * divh, true);

    mesh.PositionCount = static_cast<uint32_t>(m_Meshes.Positions.size()) - mesh.PositionOffset;
    mesh.VertexCount = static_cast<uint32_t>(m_Meshes.VertexIndices.size()) - mesh.VertexOffset;
    mesh.TriangleCount = static_cast<uint32_t>(m_Meshes.Triangles.size()) - mesh.TriangleOffset;
    mesh.EdgeCount = static_cast<uint32_t>(m_Meshes.Edges.size()) - mesh.EdgeOffset;

    return mesh;
}

Mesh Scene::CreateUnitCubeMesh()
{
    Mesh mesh = {
        .PositionOffset = static_cast<uint32_t>(m_Meshes.Positions.size()),
        .VertexOffset = static_cast<uint32_t>(m_Meshes.VertexIndices.size()),
        .TriangleOffset = static_cast<uint32_t>(m_Meshes.Triangles.size()),
        .EdgeOffset = static_cast<uint32_t>(m_Meshes.Edges.size()),
        .Color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
    };

    m_Meshes.Positions.emplace_back(-1, -1, 1, 1);
    m_Meshes.Positions.emplace_back(1, -1, 1, 1);
    m_Meshes.Positions.emplace_back(1, 1, 1, 1);
    m_Meshes.Positions.emplace_back(-1, 1, 1, 1);
    m_Meshes.VertexNormals.emplace_back(0, 0, -1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, -1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, -1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, -1, 0);
    m_Meshes.VertexIndices.push_back(0);
    m_Meshes.VertexIndices.push_back(1);
    m_Meshes.VertexIndices.push_back(2);
    m_Meshes.VertexIndices.push_back(3);

    m_Meshes.Positions.emplace_back(1, -1, -1, 1);
    m_Meshes.Positions.emplace_back(-1, -1, -1, 1);
    m_Meshes.Positions.emplace_back(-1, 1, -1, 1);
    m_Meshes.Positions.emplace_back(1, 1, -1, 1);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexIndices.push_back(4);
    m_Meshes.VertexIndices.push_back(5);
    m_Meshes.VertexIndices.push_back(6);
    m_Meshes.VertexIndices.push_back(7);

    m_Meshes.Positions.emplace_back(-1, -1, -1, 1);
    m_Meshes.Positions.emplace_back(-1, -1, 1, 1);
    m_Meshes.Positions.emplace_back(-1, 1, 1, 1);
    m_Meshes.Positions.emplace_back(-1, 1, -1, 1);
    m_Meshes.VertexNormals.emplace_back(1, 0, 0, 0);
    m_Meshes.VertexNormals.emplace_back(1, 0, 0, 0);
    m_Meshes.VertexNormals.emplace_back(1, 0, 0, 0);
    m_Meshes.VertexNormals.emplace_back(1, 0, 0, 0);
    m_Meshes.VertexIndices.push_back(8);
    m_Meshes.VertexIndices.push_back(9);
    m_Meshes.VertexIndices.push_back(10);
    m_Meshes.VertexIndices.push_back(11);

    m_Meshes.Positions.emplace_back(1, -1, 1, 1);
    m_Meshes.Positions.emplace_back(1, -1, -1, 1);
    m_Meshes.Positions.emplace_back(1, 1, -1, 1);
    m_Meshes.Positions.emplace_back(1, 1, 1, 1);
    m_Meshes.VertexNormals.emplace_back(-1, 0, 0, 0);
    m_Meshes.VertexNormals.emplace_back(-1, 0, 0, 0);
    m_Meshes.VertexNormals.emplace_back(-1, 0, 0, 0);
    m_Meshes.VertexNormals.emplace_back(-1, 0, 0, 0);
    m_Meshes.VertexIndices.push_back(12);
    m_Meshes.VertexIndices.push_back(13);
    m_Meshes.VertexIndices.push_back(14);
    m_Meshes.VertexIndices.push_back(15);

    m_Meshes.Positions.emplace_back(-1, 1, 1, 1);
    m_Meshes.Positions.emplace_back(1, 1, 1, 1);
    m_Meshes.Positions.emplace_back(1, 1, -1, 1);
    m_Meshes.Positions.emplace_back(-1, 1, -1, 1);
    m_Meshes.VertexNormals.emplace_back(0, -1, 0, 0);
    m_Meshes.VertexNormals.emplace_back(0, -1, 0, 0);
    m_Meshes.VertexNormals.emplace_back(0, -1, 0, 0);
    m_Meshes.VertexNormals.emplace_back(0, -1, 0, 0);
    m_Meshes.VertexIndices.push_back(16);
    m_Meshes.VertexIndices.push_back(17);
    m_Meshes.VertexIndices.push_back(18);
    m_Meshes.VertexIndices.push_back(19);

    m_Meshes.Positions.emplace_back(-1, -1, -1, 1);
    m_Meshes.Positions.emplace_back(1, -1, -1, 1);
    m_Meshes.Positions.emplace_back(1, -1, 1, 1);
    m_Meshes.Positions.emplace_back(-1, -1, 1, 1);
    m_Meshes.VertexNormals.emplace_back(0, 1, 0, 0);
    m_Meshes.VertexNormals.emplace_back(0, 1, 0, 0);
    m_Meshes.VertexNormals.emplace_back(0, 1, 0, 0);
    m_Meshes.VertexNormals.emplace_back(0, 1, 0, 0);
    m_Meshes.VertexIndices.push_back(20);
    m_Meshes.VertexIndices.push_back(21);
    m_Meshes.VertexIndices.push_back(22);
    m_Meshes.VertexIndices.push_back(23);

    for (int i = 0; i < 6; i++)
    {
        m_Meshes.Triangles.push_back(i * 4u + glm::uvec3(0, 2, 1));
        m_Meshes.Triangles.push_back(i * 4u + glm::uvec3(0, 3, 2));
    }

    mesh.PositionCount = static_cast<uint32_t>(m_Meshes.Positions.size()) - mesh.PositionOffset;
    mesh.VertexCount = static_cast<uint32_t>(m_Meshes.VertexIndices.size()) - mesh.VertexOffset;
    mesh.TriangleCount = static_cast<uint32_t>(m_Meshes.Triangles.size()) - mesh.TriangleOffset;
    mesh.EdgeCount = static_cast<uint32_t>(m_Meshes.Edges.size()) - mesh.EdgeOffset;

    return mesh;
}

Mesh Scene::CreateUnitSquareMesh()
{
    Mesh mesh = {
        .PositionOffset = static_cast<uint32_t>(m_Meshes.Positions.size()),
        .VertexOffset = static_cast<uint32_t>(m_Meshes.VertexIndices.size()),
        .TriangleOffset = static_cast<uint32_t>(m_Meshes.Triangles.size()),
        .EdgeOffset = static_cast<uint32_t>(m_Meshes.Edges.size()),
        .Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
    };

    m_Meshes.Positions.emplace_back(-1, -1, 0, 1);
    m_Meshes.Positions.emplace_back(1, -1, 0, 1);
    m_Meshes.Positions.emplace_back(1, 1, 0, 1);
    m_Meshes.Positions.emplace_back(-1, 1, 0, 1);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexNormals.emplace_back(0, 0, 1, 0);
    m_Meshes.VertexIndices.push_back(0);
    m_Meshes.VertexIndices.push_back(1);
    m_Meshes.VertexIndices.push_back(2);
    m_Meshes.VertexIndices.push_back(3);
    m_Meshes.Triangles.push_back(glm::uvec3(0, 1, 2));
    m_Meshes.Triangles.push_back(glm::uvec3(2, 3, 0));

    mesh.PositionCount = static_cast<uint32_t>(m_Meshes.Positions.size()) - mesh.PositionOffset;
    mesh.VertexCount = static_cast<uint32_t>(m_Meshes.VertexIndices.size()) - mesh.VertexOffset;
    mesh.TriangleCount = static_cast<uint32_t>(m_Meshes.Triangles.size()) - mesh.TriangleOffset;
    mesh.EdgeCount = static_cast<uint32_t>(m_Meshes.Edges.size()) - mesh.EdgeOffset;

    return mesh;
}

Texture Scene::LoadTexture(const std::filesystem::path& path)
{
    int x, y, channels;
    stbi_uc *data = stbi_load(path.string().c_str(), &x, &y, &channels, STBI_rgb_alpha);
    const size_t size = 4ull * x * y;

    Texture texture = { static_cast<uint32_t>(x), static_cast<uint32_t>(y) };
    std::byte* content = reinterpret_cast<std::byte*>(data);
    texture.Content.assign(content, content + size);

    stbi_image_free(data);

    return texture;
}
