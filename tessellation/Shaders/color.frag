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

layout (location = 0) out vec4 o_FragColor;

void main()
{
    o_FragColor = pc_ColorIndex == 0 ? u_Color0 : u_Color1;
}
