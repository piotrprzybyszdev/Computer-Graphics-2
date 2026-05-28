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

layout (vertices = 16) out;

layout (location = 0) in vec4 v_Position[];

layout (location = 0) out vec4 o_Position[16];

void main()
{
    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = u_InsideTessFactor;
        gl_TessLevelInner[1] = u_InsideTessFactor;
        gl_TessLevelOuter[0] = u_OutsideTessFactor;
        gl_TessLevelOuter[1] = u_OutsideTessFactor;
        gl_TessLevelOuter[2] = u_OutsideTessFactor;
        gl_TessLevelOuter[3] = u_OutsideTessFactor;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    o_Position[gl_InvocationID] = v_Position[gl_InvocationID];
}
