#version 460

float k_XOffsets[4] = float[4](-1.0f, 1.0f, -1.0f, 1.0f);
float k_YOffsets[4] = float[4](1.0f, 1.0f, -1.0f, -1.0f);
vec2 k_UVs[4] = vec2[4](vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(0.0f, 1.0f), vec2(1.0f, 1.0f));

struct Particle
{
    vec3 PrevPosition;
    float pad0;
    vec3 Position;
    float Alpha;
    vec3 Velocity;
    float Age;
};

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
    uint u_IsMirror;
    vec4 u_CameraPosition;
};

layout(set = 0, binding = 1) uniform TransformBuffer {
    Particle u_Particles[500];
};

layout (location = 0) out vec4 o_Position;
layout (location = 1) out vec2 o_UV;
layout (location = 2) out float o_Alpha;

void main()
{
    const Particle particle = u_Particles[gl_InstanceIndex];
    const float xoffset = k_XOffsets[gl_VertexIndex];
    const float yoffset = k_YOffsets[gl_VertexIndex];
    const vec2 uv = k_UVs[gl_VertexIndex];

    const vec3 center = (particle.Position + particle.PrevPosition) / 2.0f;
    const vec3 yaxis = normalize(particle.Position - particle.PrevPosition);
    const vec3 eye = normalize(center - u_CameraPosition.xyz);
    const vec3 xaxis = normalize(cross(eye, yaxis));

    const float scaleX = 0.004f;
    const float scaleY = 0.016f;
    const vec3 position = center + xaxis * xoffset * scaleX + yaxis * yoffset * scaleY;
    gl_Position = u_CameraProjection * u_CameraView * vec4(position, 1.0f);
    o_Position = vec4(position, 1.0f);
    o_UV = uv;
    o_Alpha = particle.Alpha;
}
