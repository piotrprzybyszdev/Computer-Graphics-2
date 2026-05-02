#version 460

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
};

layout (location = 0) in vec4 v_Position;
layout (location = 1) in vec4 v_Normal;
layout (location = 2) in vec4 v_Color;

layout (location = 0) out vec4 o_FragColor;

void main()
{
    const vec3 lightPosition = u_CameraOrigin.xyz;

    const vec3 N = normalize(v_Normal.xyz);
    const vec3 L = normalize(lightPosition - v_Position.xyz);
    const vec3 V = normalize(u_CameraOrigin.xyz - v_Position.xyz);
    const vec3 R = normalize(reflect(-L, N));
    
    const float ka = 0.2f, kd = 0.5f, ks = 0.5f, m = 100.0f;

    const float diffuse = max(dot(N, L), 0.0f);
    const float specular = pow(max(dot(R, V), 0.0f), m);

    o_FragColor = vec4(v_Color.rgb * vec3(ka + kd * diffuse + ks * specular), 1.0f);
}
