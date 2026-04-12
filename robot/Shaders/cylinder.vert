#version 460

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
};

layout(set = 0, binding = 1) uniform TransformBuffer {
    mat4x4 u_Transform;
};

layout (location = 0) in vec4 v_Position;
layout (location = 1) in vec4 v_Normal;

layout (location = 0) out vec4 o_Normal;

void main()
{
    gl_Position = u_CameraProjection * u_CameraView * u_Transform * vec4(v_Position.xyz, 1.0f);
    o_Normal = normalize(u_Transform * vec4(v_Normal.xyz, 0.0f));
}