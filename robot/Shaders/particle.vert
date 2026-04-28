#version 460

float k_XOffsets[4] = float[4](-1.0f, 1.0f, -1.0f, 1.0f);
float k_YOffsets[4] = float[4](1.0f, 1.0f, -1.0f, -1.0f);
vec2 k_UVs[4] = vec2[4](vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(0.0f, 1.0f), vec2(1.0f, 1.0f));

struct Particle
{
    mat4x4 Transform;
    float Alpha;
};

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
    uint u_IsMirror;
    vec4 u_CameraPosition;
};

layout(set = 0, binding = 1) uniform TransformBuffer {
    Particle u_Particles[100];
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

    const mat4x4 invTransform = inverse(u_CameraView * particle.Transform);

    const vec3 center = (particle.Transform * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
    const vec3 xaxis = normalize((invTransform * vec4(1.0f, 0.0f, 0.0f, 0.0f)).xyz);
    const vec3 yaxis = normalize((invTransform * vec4(0.0f, 1.0f, 0.0f, 0.0f)).xyz);

    const float scaleX = 0.032f;
    const float scaleY = 0.216f;
    const vec3 position = center + xaxis * xoffset * scaleX + yaxis * yoffset * scaleY;
    gl_Position = u_CameraProjection * u_CameraView * vec4(position, 1.0f);
    o_Position = vec4(position, 1.0f);
    o_UV = uv;
    o_Alpha = particle.Alpha;
}
