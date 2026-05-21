#version 460

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
};

layout(set = 0, binding = 2) uniform sampler2D u_WaterNormal;

layout(set = 0, binding = 3) uniform samplerCube u_EnvironmentTexture;

layout (location = 0) in vec4 v_Position;
layout (location = 1) in vec4 v_Normal;
layout (location = 2) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_FragColor;

vec3 intersectRay(vec3 position, vec3 direction)
{
    const float a = 2.0f;
    const float tx1 = (-a / 2.0f - position.x) / direction.x;
    const float tx2 = (a / 2.0f - position.x) / direction.x;
    const float ty1 = (-a / 2.0f - position.y) / direction.y;
    const float ty2 = (a / 2.0f - position.y) / direction.y;
    const float tz1 = (-a / 2.0f - position.z) / direction.z;
    const float tz2 = (a / 2.0f - position.z) / direction.z;

    const float tx = max(tx1, tx2);
    const float ty = max(ty1, ty2);
    const float tz = max(tz1, tz2);

    const float t = min(min(tx, ty), tz);
    return position + t * direction;
}

float fresnel(float n1, float n2, vec3 N, vec3 V)
{
    const float costheta = max(dot(N, V), 0.0f);
    const float f0 = pow((n2 - n1) / (n2 + n1), 2.0f);
    return f0 + (1.0f - f0) * pow(1.0f - costheta, 5.0f);
}

void main()
{
    const vec3 lightPosition = u_CameraOrigin.xyz;

    vec3 normal = 2.0f * texture(u_WaterNormal, v_TexCoord).rgb - 1.0f;

    const bool belowSurface = u_CameraOrigin.y < 0;
    if (belowSurface)
        normal *= -1.0f;

    const vec3 N = normalize(normal.xzy);
    const vec3 L = normalize(lightPosition - v_Position.xyz);
    const vec3 V = normalize(u_CameraOrigin.xyz - v_Position.xyz);
    const vec3 R = normalize(reflect(-L, N));
      
    const float ior = 1.333f;
    const float eta = belowSurface ? ior : 1.0f / ior;
    const vec3 viewReflected = reflect(-V, N);
    const vec3 viewRefracted = refract(-V, N, eta);

    const bool fullRefl = all(equal(viewRefracted, vec3(0.0f)));

    const vec3 reflectIntersection = intersectRay(v_Position.xyz, normalize(viewReflected));
    const vec3 refractIntersection = intersectRay(v_Position.xyz, normalize(viewRefracted));

    const vec3 envReflected = texture(u_EnvironmentTexture, reflectIntersection).rgb;
    const vec3 envRefracted = texture(u_EnvironmentTexture, refractIntersection).rgb;

    const float f = fresnel(ior, 1.0f, N, V);
    const vec3 color = fullRefl ? envReflected : mix(envRefracted, envReflected, f);

    o_FragColor = vec4(color, 1.0f);
}
