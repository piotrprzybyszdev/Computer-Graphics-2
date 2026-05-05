#version 460

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
};

layout(set = 0, binding = 2) uniform samplerCube u_ColorTexture;

layout (location = 0) in vec4 v_Position;
layout (location = 1) in vec4 v_Normal;
layout (location = 2) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_FragColor;

void main()
{
    const vec3 color = texture(u_ColorTexture, v_Position.xyz).rgb;
    o_FragColor = vec4(color, 1.0f);
}
