#version 460

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
};

layout(set = 0, binding = 1) uniform TransformBuffer {
    mat4x4 u_Transforms[2];
};

layout(push_constant) uniform PushConstants {
    uint pc_TransformIndex;
};

layout (location = 0) in vec4 v_Position;
layout (location = 1) in vec4 v_Normal;
layout (location = 2) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_Position;
layout (location = 1) out vec4 o_Normal;
layout (location = 2) out vec2 o_TexCoord;
layout (location = 3) out vec4 o_Tangent;

void main()
{
    const mat4x4 transform = u_Transforms[pc_TransformIndex];

    o_Position = transform * vec4(v_Position.xyz, 1.0f);
    o_Normal = transform * vec4(v_Normal.xyz, 0.0f);
    o_TexCoord = v_TexCoord;
    o_Tangent = transform * vec4(normalize(cross(v_Normal.xyz, vec3(1.0f, 0.0f, 0.0f))), 0.0f);

    gl_Position = u_CameraProjection * u_CameraView * o_Position;
}