#version 460

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
};

layout(set = 0, binding = 2) uniform sampler2D u_WaterNormal;

layout (location = 0) in vec4 v_Position;
layout (location = 1) in vec4 v_Normal;
layout (location = 2) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_FragColor;

void main()
{
    const vec3 lightPosition = u_CameraOrigin.xyz;

    const vec3 normal = 2.0f * texture(u_WaterNormal, v_TexCoord).rgb - 1.0f;

    const vec3 N = normalize(normal);
    const vec3 L = normalize(lightPosition - v_Position.xyz);
    const vec3 V = normalize(u_CameraOrigin.xyz - v_Position.xyz);
    const vec3 R = normalize(reflect(-L, N));
    
    const float ka = 0.2f, kd = 0.8f, ks = 0.5f, m = 100.0f;

    const float diffuse = max(dot(N, L), 0.0f);
    const float specular = pow(max(dot(R, V), 0.0f), m);

    const vec3 color = vec3(0.47f, 0.91f, 0.90f);
    o_FragColor = vec4(color * vec3(ka + kd * diffuse + ks * specular), 1.0f);
}
