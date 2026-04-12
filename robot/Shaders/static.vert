#version 460

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
};

layout(set = 0, binding = 1) readonly buffer TransformBuffer {
    mat4x4 u_Transforms[];
};

layout(push_constant, std430) uniform PushConstants {
    uint pc_MeshIndex;
};

layout (location = 0) in vec4 v_Position;
layout (location = 1) in vec4 v_Normal;

layout (location = 0) out vec4 o_Normal;

void main()
{
    const mat4x4 transform = u_Transforms[pc_MeshIndex];
    gl_Position = u_CameraProjection * u_CameraView * transform * vec4(v_Position.xyz, 1.0f);
    o_Normal = normalize(transform * vec4(v_Normal.xyz, 0.0f));
}