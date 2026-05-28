#version 460

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
};

layout (location = 0) in vec4 v_Position;

layout (location = 0) out vec4 o_Position;

void main()
{
    o_Position = vec4(v_Position.xyz, 1.0f);
    gl_Position = u_CameraProjection * u_CameraView * o_Position;
}