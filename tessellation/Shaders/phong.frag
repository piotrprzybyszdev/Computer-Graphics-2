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

layout (set = 0, binding = 2) uniform sampler2D u_DiffuseTexture;
layout (set = 0, binding = 3) uniform sampler2D u_NormalTexture;

layout (push_constant) uniform PushConstants {
    uint pc_ColorIndex;
};

layout (location = 0) in vec4 v_Position;
layout (location = 1) in vec4 v_Normal;
layout (location = 2) in vec4 v_Tangent;
layout (location = 3) in vec4 v_Bitangent;
layout (location = 4) in vec2 v_TexCoords;

layout (location = 0) out vec4 o_FragColor;

void main()
{
    o_FragColor = pc_ColorIndex == 0 ? u_Color0 : u_Color1;

    const vec3 color = texture(u_DiffuseTexture, v_TexCoords).rgb;
    const vec3 normal = texture(u_NormalTexture, v_TexCoords).rgb * 2.0f - 1.0f;

    const mat3x3 tbn = mat3x3(normalize(v_Tangent.xyz), normalize(v_Bitangent.xyz), normalize(v_Normal.xyz));
    const vec3 N = normalize(tbn * normal);

    const vec3 L = normalize(vec3(0.0f, 1.0f, 0.0f) - v_Position.xyz);
    const vec3 R = normalize(reflect(-L, N));
    const vec3 V = normalize(u_CameraOrigin.xyz - v_Position.xyz);

    const float ambient = 0.1f;
    const float diffuse = max(dot(N, L), 0.0f);
    const float specular = pow(max(dot(R, V), 0.0f), 100.0f);

    o_FragColor = vec4(color * (ambient + diffuse) + vec3(0.4f) * specular, 1.0f);
}
