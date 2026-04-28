#version 460

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
    uint u_IsMirror;
    vec4 u_CameraPosition;
    vec4 u_MirrorNormal;
};

layout(set = 0, binding = 4) uniform sampler2D u_Texture;

layout (location = 0) in vec4 v_Position;
layout (location = 1) in vec2 v_UV;
layout (location = 2) in float v_Alpha;

layout (location = 0) out vec4 o_FragColor;

void main()
{
    if (u_IsMirror == 1)
    {
        const vec3 fragDir = v_Position.xyz - u_CameraOrigin.xyz;
        const vec3 cameraDir = u_CameraPosition.xyz - u_CameraOrigin.xyz;
        const vec3 mirrorNormal = dot(u_MirrorNormal.xyz, cameraDir) * u_MirrorNormal.xyz;

        if (dot(fragDir, mirrorNormal) < 0)
            discard;
    }

    const vec4 color = texture(u_Texture, v_UV);
    o_FragColor = vec4(color.rgb, v_Alpha * color.a);
}
