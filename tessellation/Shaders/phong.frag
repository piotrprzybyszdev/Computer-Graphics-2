#version 460

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
    vec4 u_Color0;
    vec4 u_Color1;
    uint u_InsideTessFactor;
    uint u_OutsideTessFactor;
    uvec2 pad0;
};

layout (push_constant) uniform PushConstants {
    uint pc_ColorIndex;
};

layout (location = 0) in vec4 v_Position;
layout (location = 1) in vec4 v_Normal;

layout (location = 0) out vec4 o_FragColor;

void main()
{
    o_FragColor = pc_ColorIndex == 0 ? u_Color0 : u_Color1;

    const vec3 N = normalize(v_Normal.xyz);
    const vec3 L = normalize(vec3(0.0f, 1.0f, 0.0f) - v_Position.xyz);
    const vec3 R = normalize(reflect(-L, N));
    const vec3 V = normalize(u_CameraOrigin.xyz - v_Position.xyz);

    const float ambient = 0.1f;
    const float specular = pow(max(dot(R, V), 0.0f), 100.0f);

    o_FragColor = vec4(vec3(0.0f, 0.0f, 1.0f) * ambient + vec3(1.0f) * specular, 1.0f);
}
