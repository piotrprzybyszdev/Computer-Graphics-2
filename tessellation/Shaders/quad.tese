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

layout (quads, fractional_odd_spacing, cw) in;

layout (location = 0) in vec4 v_Position[];

layout (location = 0) out vec4 o_Position;

vec3 evalBezierCurve(vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t)
{
    const vec3 p10 = mix(p0, p1, t), p11 = mix(p1, p2, t), p12 = mix(p2, p3, t);
    const vec3 p20 = mix(p10, p11, t), p21 = mix(p11, p12, t);
    return mix(p20, p21, t);
}

void main()
{
    const vec3 p0 = evalBezierCurve(v_Position[0].xyz, v_Position[1].xyz, v_Position[2].xyz, v_Position[3].xyz, gl_TessCoord.x);
    const vec3 p1 = evalBezierCurve(v_Position[4].xyz, v_Position[5].xyz, v_Position[6].xyz, v_Position[7].xyz, gl_TessCoord.x);
    const vec3 p2 = evalBezierCurve(v_Position[8].xyz, v_Position[9].xyz, v_Position[10].xyz, v_Position[11].xyz, gl_TessCoord.x);
    const vec3 p3 = evalBezierCurve(v_Position[12].xyz, v_Position[13].xyz, v_Position[14].xyz, v_Position[15].xyz, gl_TessCoord.x);
    const vec3 position = evalBezierCurve(p0, p1, p2, p3, gl_TessCoord.y);

    gl_Position = u_CameraProjection * u_CameraView * vec4(position, 1.0f);
    o_Position = vec4(position, 1.0f);
}
