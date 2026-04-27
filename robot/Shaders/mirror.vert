#version 460

struct Mesh
{
    uint PositionOffset;
    uint PositionCount;
    uint VertexOffset;
    uint VertexCount;
    uint TriangleOffset;
    uint TriangleCount;
    uint EdgeOffset;
    uint EdgeCount;
    vec4 Color;
};

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
};

layout(set = 0, binding = 1) readonly buffer PositionBuffer {
    vec4 s_Positions[];
};

layout(set = 0, binding = 2) readonly buffer MeshBuffer {
    Mesh s_Meshes[];
};

layout(set = 0, binding = 3) uniform TransformBuffer {
    mat4x4 u_Transforms[6];
};

layout(push_constant) uniform PushConstants {
    uint pc_MeshIndex;
};

layout (location = 0) in uint v_PositionIndex;
layout (location = 1) in vec4 v_Normal;

layout (location = 0) out vec2 o_UV;
layout (location = 1) out float o_Alpha;

void main()
{
    const uint positionOffset = s_Meshes[pc_MeshIndex].PositionOffset;
    const mat4x4 transform = u_Transforms[pc_MeshIndex];

    o_UV = (1.0f + s_Positions[positionOffset + v_PositionIndex].xy) / 2.0f;
    o_Alpha = 0.5f;

    gl_Position = u_CameraProjection * u_CameraView * transform * vec4(s_Positions[positionOffset + v_PositionIndex].xyz, 1.0f);
}