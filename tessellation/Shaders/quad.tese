#version 460

layout (set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
    vec4 u_Color0;
    vec4 u_Color1;
    uint u_InsideTessFactor;
    uint u_OutsideTessFactor;
    uvec2 pad0;
};

layout (set = 0, binding = 1) uniform sampler2D u_HeightTexture;

layout (quads, equal_spacing, cw) in;

layout (location = 0) in vec4 v_Position[];

layout (location = 0) out vec4 o_Position;
layout (location = 1) out vec4 o_Normal;
layout (location = 2) out vec4 o_Tangent;
layout (location = 3) out vec4 o_Bitangent;
layout (location = 4) out vec2 o_TexCoords;

vec3 evalBezierCurve(vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t)
{
    const vec3 p10 = mix(p0, p1, t), p11 = mix(p1, p2, t), p12 = mix(p2, p3, t);
    const vec3 p20 = mix(p10, p11, t), p21 = mix(p11, p12, t);
    return mix(p20, p21, t);
}

vec3 evalBezierCurve(vec3 p0, vec3 p1, vec3 p2, float t)
{
    const vec3 p10 = mix(p0, p1, t), p11 = mix(p1, p2, t);
    return mix(p10, p11, t);
}

vec3 computeTangent(vec3 pts[16], float t, float s)
{
    const vec3 d0 = evalBezierCurve(pts[1].xyz - pts[0].xyz, pts[2].xyz - pts[1].xyz, pts[3].xyz - pts[2].xyz, t);
    const vec3 d1 = evalBezierCurve(pts[5].xyz - pts[4].xyz, pts[6].xyz - pts[5].xyz, pts[7].xyz - pts[6].xyz, t);
    const vec3 d2 = evalBezierCurve(pts[9].xyz - pts[8].xyz, pts[10].xyz - pts[9].xyz, pts[11].xyz - pts[10].xyz, t);
    const vec3 d3 = evalBezierCurve(pts[13].xyz - pts[12].xyz, pts[14].xyz - pts[13].xyz, pts[15].xyz - pts[14].xyz, t);
    return evalBezierCurve(d0, d1, d2, d3, s);
}

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
    const vec3 p0 = evalBezierCurve(v_Position[0].xyz, v_Position[1].xyz, v_Position[2].xyz, v_Position[3].xyz, gl_TessCoord.x);
    const vec3 p1 = evalBezierCurve(v_Position[4].xyz, v_Position[5].xyz, v_Position[6].xyz, v_Position[7].xyz, gl_TessCoord.x);
    const vec3 p2 = evalBezierCurve(v_Position[8].xyz, v_Position[9].xyz, v_Position[10].xyz, v_Position[11].xyz, gl_TessCoord.x);
    const vec3 p3 = evalBezierCurve(v_Position[12].xyz, v_Position[13].xyz, v_Position[14].xyz, v_Position[15].xyz, gl_TessCoord.x);
    const vec3 position = evalBezierCurve(p0, p1, p2, p3, gl_TessCoord.y);

    vec3 ptsu[16] = {
        v_Position[0].xyz, v_Position[1].xyz, v_Position[2].xyz, v_Position[3].xyz,
        v_Position[4].xyz, v_Position[5].xyz, v_Position[6].xyz, v_Position[7].xyz,
        v_Position[8].xyz, v_Position[9].xyz, v_Position[10].xyz, v_Position[11].xyz,
        v_Position[12].xyz, v_Position[13].xyz, v_Position[14].xyz, v_Position[15].xyz
    };
    vec3 ptsv[16] = {
        v_Position[0].xyz, v_Position[4].xyz, v_Position[8].xyz, v_Position[12].xyz,
        v_Position[1].xyz, v_Position[5].xyz, v_Position[9].xyz, v_Position[13].xyz,
        v_Position[2].xyz, v_Position[6].xyz, v_Position[10].xyz, v_Position[14].xyz,
        v_Position[3].xyz, v_Position[7].xyz, v_Position[11].xyz, v_Position[15].xyz
    };

    const vec3 tangent = normalize(computeTangent(ptsu, gl_TessCoord.x, gl_TessCoord.y));
    const vec3 bitangent = normalize(computeTangent(ptsv, gl_TessCoord.y, gl_TessCoord.x));
    const vec3 normal = normalize(cross(tangent, bitangent));

    const vec2 texCoords = (position.xz + 1.0f) / 2.0f;
    const float mip = 6.0f - log2(factor(distance(u_CameraOrigin.xyz, position)));
    const float height = textureLod(u_HeightTexture, texCoords, mip).r;
    const vec3 mappedPosition = position + normal * height / 20.0f;

    gl_Position = u_CameraProjection * u_CameraView * vec4(mappedPosition, 1.0f);
    o_Position = vec4(mappedPosition, 1.0f);
    o_Normal = vec4(normal, 0.0f);
    o_Tangent = vec4(tangent, 0.0f);
    o_Bitangent = vec4(bitangent, 0.0f);
    o_TexCoords = texCoords;
}
