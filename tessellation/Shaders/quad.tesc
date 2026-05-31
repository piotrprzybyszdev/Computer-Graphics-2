#version 460

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
    vec4 u_Color0;
    vec4 u_Color1;
    float u_InsideTessFactor;
    float u_OutsideTessFactor;
    uvec2 pad0;
};

layout (vertices = 16) out;

layout (location = 0) in vec4 v_Position[];

layout (location = 0) out vec4 o_Position[16];

float log10(float x)
{
    return log(x) / log(10.0f);
}

float factor(float dist)
{
    return -16.0f * log10(dist * 0.01f);
}

void main()
{
    if (gl_InvocationID == 0)
    {
        const float distances[4] = {
            distance(u_CameraOrigin.xyz, v_Position[0].xyz), distance(u_CameraOrigin.xyz, v_Position[3].xyz),
            distance(u_CameraOrigin.xyz, v_Position[12].xyz), distance(u_CameraOrigin.xyz, v_Position[15].xyz)
        };

        const float innerFactor = factor((distances[0] + distances[1] + distances[2] + distances[3]) / 4.0f);
        const float upFactor = factor((distances[0] + distances[1]) / 2.0f);
        const float leftFactor = factor((distances[0] + distances[2]) / 2.0f);
        const float rightFactor = factor((distances[1] + distances[3]) / 2.0f);
        const float downFactor = factor((distances[2] + distances[3]) / 2.0f);

        gl_TessLevelInner[0] = innerFactor * u_InsideTessFactor;
        gl_TessLevelInner[1] = innerFactor * u_InsideTessFactor;
        gl_TessLevelOuter[0] = leftFactor * u_OutsideTessFactor;
        gl_TessLevelOuter[1] = upFactor * u_OutsideTessFactor;
        gl_TessLevelOuter[2] = rightFactor * u_OutsideTessFactor;
        gl_TessLevelOuter[3] = downFactor * u_OutsideTessFactor;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    o_Position[gl_InvocationID] = v_Position[gl_InvocationID];
}
